/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 Ventana Micro Systems Inc.
 */

#include <sbi/sbi_bitmap.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_hart_protection.h>
#include <sbi/sbi_hfence.h>
#include <sbi/sbi_math.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_tlb.h>
#include <sbi/riscv_asm.h>

/*
 * Smepmp enforces access boundaries between M-mode and
 * S/U-mode. When it is enabled, the PMPs are programmed
 * such that M-mode doesn't have access to S/U-mode memory.
 *
 * To give M-mode R/W access to the shared memory between M and
 * S/U-mode, first entry is reserved. It is disabled at boot.
 * When shared memory access is required, the physical address
 * should be programmed into the first PMP entry with R/W
 * permissions to the M-mode. Once the work is done, it should be
 * unmapped. sbi_hart_protection_map_range/sbi_hart_protection_unmap_range
 * function pair should be used to map/unmap the shared memory.
 */
#define SBI_SMEPMP_RESV_ENTRY		0

static DECLARE_BITMAP(fw_smepmp_ids, PMP_COUNT);
static bool fw_smepmp_ids_inited;

unsigned int sbi_hart_pmp_count(struct sbi_scratch *scratch)
{
	struct sbi_hart_features *hfeatures = sbi_hart_features_ptr(scratch);

	return hfeatures->pmp_count;
}

unsigned int sbi_hart_pmp_log2gran(struct sbi_scratch *scratch)
{
	struct sbi_hart_features *hfeatures = sbi_hart_features_ptr(scratch);

	return hfeatures->pmp_log2gran;
}

unsigned int sbi_hart_pmp_addrbits(struct sbi_scratch *scratch)
{
	struct sbi_hart_features *hfeatures = sbi_hart_features_ptr(scratch);

	return hfeatures->pmp_addr_bits;
}

bool sbi_hart_smepmp_is_fw_region(unsigned int pmp_idx)
{
	if (!fw_smepmp_ids_inited)
		return false;

	return bitmap_test(fw_smepmp_ids, pmp_idx) ? true : false;
}

void sbi_hart_pmp_fence(void)
{
	/*
	 * As per section 3.7.2 of privileged specification v1.12,
	 * virtual address translations can be speculatively performed
	 * (even before actual access). These, along with PMP traslations,
	 * can be cached. This can pose a problem with CPU hotplug
	 * and non-retentive suspend scenario because PMP states are
	 * not preserved.
	 * It is advisable to flush the caching structures under such
	 * conditions.
	 */
	if (misa_extension('S')) {
		__sbi_sfence_vma_all();

		/*
		 * If hypervisor mode is supported, flush caching
		 * structures in guest mode too.
		 */
		if (misa_extension('H'))
			__sbi_hfence_gvma_all();
	}
}

static void sbi_hart_smepmp_set(struct sbi_scratch *scratch,
				struct sbi_domain *dom,
				struct sbi_domain_memregion *reg,
				unsigned int pmp_idx,
				unsigned int pmp_flags,
				unsigned int pmp_log2gran,
				unsigned long pmp_addr_max)
{
	unsigned long pmp_addr = reg->base >> PMP_SHIFT;

	if (pmp_log2gran <= reg->order && pmp_addr < pmp_addr_max) {
		sbi_platform_pmp_set(sbi_platform_ptr(scratch),
				     pmp_idx, reg->flags, pmp_flags,
				     reg->base, reg->order);
		pmp_set(pmp_idx, pmp_flags, reg->base, reg->order);
	} else {
		sbi_printf("Can not configure pmp for domain %s because"
			   " memory region address 0x%lx or size 0x%lx "
			   "is not in range.\n", dom->name, reg->base,
			   reg->order);
	}
}

static bool is_valid_pmp_idx(unsigned int pmp_count, unsigned int pmp_idx)
{
	if (pmp_count > pmp_idx)
		return true;

	sbi_printf("error: insufficient PMP entries\n");
	return false;
}

static int sbi_hart_smepmp_configure(struct sbi_scratch *scratch)
{
	struct sbi_domain_memregion *reg;
	struct sbi_domain *dom = sbi_domain_thishart_ptr();
	unsigned int pmp_log2gran, pmp_bits;
	unsigned int pmp_idx, pmp_count;
	unsigned long pmp_addr_max;
	unsigned int pmp_flags;

	pmp_count = sbi_hart_pmp_count(scratch);
	pmp_log2gran = sbi_hart_pmp_log2gran(scratch);
	pmp_bits = sbi_hart_pmp_addrbits(scratch) - 1;
	pmp_addr_max = (1UL << pmp_bits) | ((1UL << pmp_bits) - 1);

	/*
	 * Set the RLB so that, we can write to PMP entries without
	 * enforcement even if some entries are locked.
	 */
	csr_set(CSR_MSECCFG, MSECCFG_RLB);

	/* Disable the reserved entry */
	pmp_disable(SBI_SMEPMP_RESV_ENTRY);

	/* Program M-only regions when MML is not set. */
	pmp_idx = 0;
	sbi_domain_for_each_memregion(dom, reg) {
		/* Skip reserved entry */
		if (pmp_idx == SBI_SMEPMP_RESV_ENTRY)
			pmp_idx++;
		if (!is_valid_pmp_idx(pmp_count, pmp_idx))
			return SBI_EFAIL;

		/* Skip shared and SU-only regions */
		if (!SBI_DOMAIN_MEMREGION_M_ONLY_ACCESS(reg->flags)) {
			pmp_idx++;
			continue;
		}

		/*
		 * Track firmware PMP entries to preserve them during
		 * domain switches. Under SmePMP, M-mode requires
		 * explicit PMP entries to access firmware code/data.
		 * These entries must remain enabled across domain
		 * context switches to prevent M-mode access faults.
		 */
		if (SBI_DOMAIN_MEMREGION_IS_FIRMWARE(reg->flags)) {
			if (fw_smepmp_ids_inited) {
				/* Check inconsistent firmware region */
				if (!sbi_hart_smepmp_is_fw_region(pmp_idx))
					return SBI_EINVAL;
			} else {
				bitmap_set(fw_smepmp_ids, pmp_idx, 1);
			}
		}

		pmp_flags = sbi_domain_get_smepmp_flags(reg);

		sbi_hart_smepmp_set(scratch, dom, reg, pmp_idx++, pmp_flags,
				    pmp_log2gran, pmp_addr_max);
	}

	fw_smepmp_ids_inited = true;

	/* Set the MML to enforce new encoding */
	csr_set(CSR_MSECCFG, MSECCFG_MML);

	/* Program shared and SU-only regions */
	pmp_idx = 0;
	sbi_domain_for_each_memregion(dom, reg) {
		/* Skip reserved entry */
		if (pmp_idx == SBI_SMEPMP_RESV_ENTRY)
			pmp_idx++;
		if (!is_valid_pmp_idx(pmp_count, pmp_idx))
			return SBI_EFAIL;

		/* Skip M-only regions */
		if (SBI_DOMAIN_MEMREGION_M_ONLY_ACCESS(reg->flags)) {
			pmp_idx++;
			continue;
		}

		pmp_flags = sbi_domain_get_smepmp_flags(reg);

		sbi_hart_smepmp_set(scratch, dom, reg, pmp_idx++, pmp_flags,
				    pmp_log2gran, pmp_addr_max);
	}

	/*
	 * All entries are programmed.
	 * Keep the RLB bit so that dynamic mappings can be done.
	 */

	sbi_hart_pmp_fence();
	return 0;
}

static int sbi_hart_smepmp_map_range(struct sbi_scratch *scratch,
				     unsigned long addr, unsigned long size)
{
	/* shared R/W access for M and S/U mode */
	unsigned int pmp_flags = (PMP_W | PMP_X);
	unsigned long order, base = 0;

	if (is_pmp_entry_mapped(SBI_SMEPMP_RESV_ENTRY))
		return SBI_ENOSPC;

	for (order = MAX(sbi_hart_pmp_log2gran(scratch), log2roundup(size));
	     order <= __riscv_xlen; order++) {
		if (order < __riscv_xlen) {
			base = addr & ~((1UL << order) - 1UL);
			if ((base <= addr) &&
			    (addr < (base + (1UL << order))) &&
			    (base <= (addr + size - 1UL)) &&
			    ((addr + size - 1UL) < (base + (1UL << order))))
				break;
		} else {
			return SBI_EFAIL;
		}
	}

	sbi_platform_pmp_set(sbi_platform_ptr(scratch), SBI_SMEPMP_RESV_ENTRY,
			     SBI_DOMAIN_MEMREGION_SHARED_SURW_MRW,
			     pmp_flags, base, order);
	pmp_set(SBI_SMEPMP_RESV_ENTRY, pmp_flags, base, order);

	return SBI_OK;
}

static int sbi_hart_smepmp_unmap_range(struct sbi_scratch *scratch,
				       unsigned long addr, unsigned long size)
{
	sbi_platform_pmp_disable(sbi_platform_ptr(scratch), SBI_SMEPMP_RESV_ENTRY);
	return pmp_disable(SBI_SMEPMP_RESV_ENTRY);
}

static int sbi_hart_oldpmp_configure(struct sbi_scratch *scratch)
{
	struct sbi_domain_memregion *reg;
	struct sbi_domain *dom = sbi_domain_thishart_ptr();
	unsigned long pmp_addr, pmp_addr_max;
	unsigned int pmp_log2gran, pmp_bits;
	unsigned int pmp_idx, pmp_count;
	unsigned int pmp_flags;

	pmp_count = sbi_hart_pmp_count(scratch);
	pmp_log2gran = sbi_hart_pmp_log2gran(scratch);
	pmp_bits = sbi_hart_pmp_addrbits(scratch) - 1;
	pmp_addr_max = (1UL << pmp_bits) | ((1UL << pmp_bits) - 1);

	pmp_idx = 0;
	sbi_domain_for_each_memregion(dom, reg) {
		if (!is_valid_pmp_idx(pmp_count, pmp_idx))
			return SBI_EFAIL;

		pmp_flags = sbi_domain_get_oldpmp_flags(reg);
		pmp_addr = reg->base >> PMP_SHIFT;
		if (pmp_log2gran <= reg->order && pmp_addr < pmp_addr_max) {
			sbi_platform_pmp_set(sbi_platform_ptr(scratch),
					     pmp_idx, reg->flags, pmp_flags,
					     reg->base, reg->order);
			pmp_set(pmp_idx++, pmp_flags, reg->base, reg->order);
		} else {
			sbi_printf("Can not configure pmp for domain %s because"
				   " memory region address 0x%lx or size 0x%lx "
				   "is not in range.\n", dom->name, reg->base,
				   reg->order);
		}
	}

	sbi_hart_pmp_fence();
	return 0;
}

static void sbi_hart_pmp_unconfigure(struct sbi_scratch *scratch)
{
	int i, pmp_count = sbi_hart_pmp_count(scratch);

	for (i = 0; i < pmp_count; i++) {
		/* Don't revoke firmware access permissions */
		if (sbi_hart_smepmp_is_fw_region(i))
			continue;

		sbi_platform_pmp_disable(sbi_platform_ptr(scratch), i);
		pmp_disable(i);
	}
}

static struct sbi_hart_protection pmp_protection = {
	.name = "pmp",
	.rating = 100,
	.configure = sbi_hart_oldpmp_configure,
	.unconfigure = sbi_hart_pmp_unconfigure,
};

static struct sbi_hart_protection epmp_protection = {
	.name = "epmp",
	.rating = 200,
	.configure = sbi_hart_smepmp_configure,
	.unconfigure = sbi_hart_pmp_unconfigure,
	.map_range = sbi_hart_smepmp_map_range,
	.unmap_range = sbi_hart_smepmp_unmap_range,
};

int sbi_hart_pmp_init(struct sbi_scratch *scratch)
{
	int rc;

	if (sbi_hart_pmp_count(scratch)) {
		if (sbi_hart_has_extension(scratch, SBI_HART_EXT_SMEPMP)) {
			rc = sbi_hart_protection_register(&epmp_protection);
			if (rc)
				return rc;
		} else {
			rc = sbi_hart_protection_register(&pmp_protection);
			if (rc)
				return rc;
		}
	}

	return 0;
}
