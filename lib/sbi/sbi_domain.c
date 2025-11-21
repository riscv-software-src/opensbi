/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <sbi/riscv_asm.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_hartmask.h>
#include <sbi/sbi_heap.h>
#include <sbi/sbi_hsm.h>
#include <sbi/sbi_list.h>
#include <sbi/sbi_math.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_string.h>

SBI_LIST_HEAD(domain_list);

static u32 domain_count = 0;
static bool domain_finalized = false;

#define ROOT_REGION_MAX	32

struct sbi_domain root = {
	.name = "root",
	.possible_harts = NULL,
	.regions = NULL,
	.system_reset_allowed = true,
	.system_suspend_allowed = true,
	.fw_region_inited = false,
};

static unsigned long domain_hart_ptr_offset;

struct sbi_domain *sbi_hartindex_to_domain(u32 hartindex)
{
	struct sbi_scratch *scratch;

	scratch = sbi_hartindex_to_scratch(hartindex);
	if (!scratch || !domain_hart_ptr_offset)
		return NULL;

	return sbi_scratch_read_type(scratch, void *, domain_hart_ptr_offset);
}

void sbi_update_hartindex_to_domain(u32 hartindex, struct sbi_domain *dom)
{
	struct sbi_scratch *scratch;

	scratch = sbi_hartindex_to_scratch(hartindex);
	if (!scratch)
		return;

	sbi_scratch_write_type(scratch, void *, domain_hart_ptr_offset, dom);
}

bool sbi_domain_is_assigned_hart(const struct sbi_domain *dom, u32 hartindex)
{
	bool ret;
	struct sbi_domain *tdom = (struct sbi_domain *)dom;

	if (!dom)
		return false;

	spin_lock(&tdom->assigned_harts_lock);
	ret = sbi_hartmask_test_hartindex(hartindex, &tdom->assigned_harts);
	spin_unlock(&tdom->assigned_harts_lock);

	return ret;
}

int sbi_domain_get_assigned_hartmask(const struct sbi_domain *dom,
				     struct sbi_hartmask *mask)
{
	ulong ret = 0;
	struct sbi_domain *tdom = (struct sbi_domain *)dom;

	if (!dom) {
		sbi_hartmask_clear_all(mask);
		return 0;
	}

	spin_lock(&tdom->assigned_harts_lock);
	sbi_hartmask_copy(mask, &tdom->assigned_harts);
	spin_unlock(&tdom->assigned_harts_lock);

	return ret;
}

void sbi_domain_memregion_init(unsigned long addr,
				unsigned long size,
				unsigned long flags,
				struct sbi_domain_memregion *reg)
{
	unsigned long base = 0, order;

	for (order = log2roundup(size) ; order <= __riscv_xlen; order++) {
		if (order < __riscv_xlen) {
			base = addr & ~((1UL << order) - 1UL);
			if ((base <= addr) &&
			    (addr < (base + (1UL << order))) &&
			    (base <= (addr + size - 1UL)) &&
			    ((addr + size - 1UL) < (base + (1UL << order))))
				break;
		} else {
			base = 0;
			break;
		}

	}

	if (reg) {
		reg->base = base;
		reg->order = order;
		reg->flags = flags;
	}
}

unsigned int sbi_domain_get_smepmp_flags(struct sbi_domain_memregion *reg)
{
	unsigned int pmp_flags = 0;
	unsigned long rstart, rend;

	if ((reg->flags & SBI_DOMAIN_MEMREGION_ACCESS_MASK) == 0) {
		/*
		 * Region is inaccessible in all privilege modes.
		 *
		 * SmePMP allows two encodings for an inaccessible region:
		 *   - pmpcfg.LRWX = 0000 (Inaccessible region)
		 *   - pmpcfg.LRWX = 1000 (Locked inaccessible region)
		 * We use the first encoding here.
		 */
		return 0;
	} else if (SBI_DOMAIN_MEMREGION_IS_SHARED(reg->flags)) {
		/* Read only for both M and SU modes */
		if (SBI_DOMAIN_MEMREGION_IS_SUR_MR(reg->flags))
			pmp_flags = (PMP_L | PMP_R | PMP_W | PMP_X);

		/* Execute for SU but Read/Execute for M mode */
		else if (SBI_DOMAIN_MEMREGION_IS_SUX_MRX(reg->flags))
			/* locked region */
			pmp_flags = (PMP_L | PMP_W | PMP_X);

		/* Execute only for both M and SU modes */
		else if (SBI_DOMAIN_MEMREGION_IS_SUX_MX(reg->flags))
			pmp_flags = (PMP_L | PMP_W);

		/* Read/Write for both M and SU modes */
		else if (SBI_DOMAIN_MEMREGION_IS_SURW_MRW(reg->flags))
			pmp_flags = (PMP_W | PMP_X);

		/* Read only for SU mode but Read/Write for M mode */
		else if (SBI_DOMAIN_MEMREGION_IS_SUR_MRW(reg->flags))
			pmp_flags = (PMP_W);
	} else if (SBI_DOMAIN_MEMREGION_M_ONLY_ACCESS(reg->flags)) {
		/*
		 * When smepmp is supported and used, M region cannot have RWX
		 * permissions on any region.
		 */
		if ((reg->flags & SBI_DOMAIN_MEMREGION_M_ACCESS_MASK)
		    == SBI_DOMAIN_MEMREGION_M_RWX) {
			sbi_printf("%s: M-mode only regions cannot have"
				   "RWX permissions\n", __func__);
			return 0;
		}

		/* M-mode only access regions are always locked */
		pmp_flags |= PMP_L;

		if (reg->flags & SBI_DOMAIN_MEMREGION_M_READABLE)
			pmp_flags |= PMP_R;
		if (reg->flags & SBI_DOMAIN_MEMREGION_M_WRITABLE)
			pmp_flags |= PMP_W;
		if (reg->flags & SBI_DOMAIN_MEMREGION_M_EXECUTABLE)
			pmp_flags |= PMP_X;
	} else if (SBI_DOMAIN_MEMREGION_SU_ONLY_ACCESS(reg->flags)) {
		if (reg->flags & SBI_DOMAIN_MEMREGION_SU_READABLE)
			pmp_flags |= PMP_R;
		if (reg->flags & SBI_DOMAIN_MEMREGION_SU_WRITABLE)
			pmp_flags |= PMP_W;
		if (reg->flags & SBI_DOMAIN_MEMREGION_SU_EXECUTABLE)
			pmp_flags |= PMP_X;
	} else {
		rstart = reg->base;
		rend = (reg->order < __riscv_xlen) ? rstart + ((1UL << reg->order) - 1) : -1UL;
		sbi_printf("%s: Unsupported Smepmp permissions on region 0x%"PRILX"-0x%"PRILX"\n",
			   __func__, rstart, rend);
	}

	return pmp_flags;
}

bool sbi_domain_check_addr(const struct sbi_domain *dom,
			   unsigned long addr, unsigned long mode,
			   unsigned long access_flags)
{
	bool rmmio, mmio = false;
	struct sbi_domain_memregion *reg;
	unsigned long rstart, rend, rflags, rwx = 0, rrwx = 0;

	if (!dom)
		return false;

	/*
	 * Use M_{R/W/X} bits because the SU-bits are at the
	 * same relative offsets. If the mode is not M, the SU
	 * bits will fall at same offsets after the shift.
	 */
	if (access_flags & SBI_DOMAIN_READ)
		rwx |= SBI_DOMAIN_MEMREGION_M_READABLE;

	if (access_flags & SBI_DOMAIN_WRITE)
		rwx |= SBI_DOMAIN_MEMREGION_M_WRITABLE;

	if (access_flags & SBI_DOMAIN_EXECUTE)
		rwx |= SBI_DOMAIN_MEMREGION_M_EXECUTABLE;

	if (access_flags & SBI_DOMAIN_MMIO)
		mmio = true;

	sbi_domain_for_each_memregion(dom, reg) {
		rflags = reg->flags;
		rrwx = (mode == PRV_M ?
			(rflags & SBI_DOMAIN_MEMREGION_M_ACCESS_MASK) :
			(rflags & SBI_DOMAIN_MEMREGION_SU_ACCESS_MASK)
			>> SBI_DOMAIN_MEMREGION_SU_ACCESS_SHIFT);

		rstart = reg->base;
		rend = (reg->order < __riscv_xlen) ?
			rstart + ((1UL << reg->order) - 1) : -1UL;
		if (rstart <= addr && addr <= rend) {
			rmmio = (rflags & SBI_DOMAIN_MEMREGION_MMIO) ? true : false;
			/*
			 * MMIO devices may appear in regions without the flag set (such as the
			 * default region), but MMIO device regions should not be used as memory.
			 */
			if (!mmio && rmmio)
				return false;
			return ((rrwx & rwx) == rwx) ? true : false;
		}
	}

	return (mode == PRV_M) ? true : false;
}

/* Check if region complies with constraints */
static bool is_region_valid(const struct sbi_domain_memregion *reg)
{
	if (reg->order < 3 || __riscv_xlen < reg->order)
		return false;

	if (reg->order == __riscv_xlen && reg->base != 0)
		return false;

	if (reg->order < __riscv_xlen && (reg->base & (BIT(reg->order) - 1)))
		return false;

	return true;
}

/** Check if regionA is sub-region of regionB */
static bool is_region_subset(const struct sbi_domain_memregion *regA,
			     const struct sbi_domain_memregion *regB)
{
	ulong regA_start = regA->base;
	ulong regA_end = regA->base + (BIT(regA->order) - 1);
	ulong regB_start = regB->base;
	ulong regB_end = regB->base + (BIT(regB->order) - 1);

	if ((regB_start <= regA_start) &&
	    (regA_start < regB_end) &&
	    (regB_start < regA_end) &&
	    (regA_end <= regB_end))
		return true;

	return false;
}

/** Check if regionA can be replaced by regionB */
static bool is_region_compatible(const struct sbi_domain_memregion *regA,
				 const struct sbi_domain_memregion *regB)
{
	if (is_region_subset(regA, regB) && regA->flags == regB->flags)
		return true;

	return false;
}

/** Check if regionA should be placed before regionB */
static bool is_region_before(const struct sbi_domain_memregion *regA,
			     const struct sbi_domain_memregion *regB)
{
	/*
	 * Enforce firmware region ordering for memory access
	 * under SmePMP.
	 * Place firmware regions first to ensure consistent
	 * PMP entries during domain context switches.
	 */
	if (SBI_DOMAIN_MEMREGION_IS_FIRMWARE(regA->flags) &&
	   !SBI_DOMAIN_MEMREGION_IS_FIRMWARE(regB->flags))
		return true;
	if (!SBI_DOMAIN_MEMREGION_IS_FIRMWARE(regA->flags) &&
	    SBI_DOMAIN_MEMREGION_IS_FIRMWARE(regB->flags))
		return false;

	if (regA->order < regB->order)
		return true;

	if ((regA->order == regB->order) &&
	    (regA->base < regB->base))
		return true;

	return false;
}

static const struct sbi_domain_memregion *find_region(
						const struct sbi_domain *dom,
						unsigned long addr)
{
	unsigned long rstart, rend;
	struct sbi_domain_memregion *reg;

	sbi_domain_for_each_memregion(dom, reg) {
		rstart = reg->base;
		rend = (reg->order < __riscv_xlen) ?
			rstart + ((1UL << reg->order) - 1) : -1UL;
		if (rstart <= addr && addr <= rend)
			return reg;
	}

	return NULL;
}

static const struct sbi_domain_memregion *find_next_subset_region(
				const struct sbi_domain *dom,
				const struct sbi_domain_memregion *reg,
				unsigned long addr)
{
	struct sbi_domain_memregion *sreg, *ret = NULL;

	sbi_domain_for_each_memregion(dom, sreg) {
		if (sreg == reg || (sreg->base <= addr) ||
		    !is_region_subset(sreg, reg))
			continue;

		if (!ret || (sreg->base < ret->base) ||
		    ((sreg->base == ret->base) && (sreg->order < ret->order)))
			ret = sreg;
	}

	return ret;
}

static void swap_region(struct sbi_domain_memregion* reg1,
			struct sbi_domain_memregion* reg2)
{
	struct sbi_domain_memregion treg;

	sbi_memcpy(&treg, reg1, sizeof(treg));
	sbi_memcpy(reg1, reg2, sizeof(treg));
	sbi_memcpy(reg2, &treg, sizeof(treg));
}

static void clear_region(struct sbi_domain_memregion* reg)
{
	sbi_memset(reg, 0x0, sizeof(*reg));
}

static int sbi_domain_used_memregions(const struct sbi_domain *dom)
{
	int count = 0;
	struct sbi_domain_memregion *reg;

	sbi_domain_for_each_memregion(dom, reg)
		count++;

	return count;
}

static int sanitize_domain(struct sbi_domain *dom)
{
	u32 i, j, count;
	bool is_covered;
	struct sbi_domain_memregion *reg, *reg1;

	/* Check possible HARTs */
	if (!dom->possible_harts) {
		sbi_printf("%s: %s possible HART mask is NULL\n",
			   __func__, dom->name);
		return SBI_EINVAL;
	}
	sbi_hartmask_for_each_hartindex(i, dom->possible_harts) {
		if (!sbi_hartindex_valid(i)) {
			sbi_printf("%s: %s possible HART mask has invalid "
				   "hart %d\n", __func__,
				   dom->name, sbi_hartindex_to_hartid(i));
			return SBI_EINVAL;
		}
	}

	/* Check memory regions */
	if (!dom->regions) {
		sbi_printf("%s: %s regions is NULL\n",
			   __func__, dom->name);
		return SBI_EINVAL;
	}
	sbi_domain_for_each_memregion(dom, reg) {
		if (!is_region_valid(reg)) {
			sbi_printf("%s: %s has invalid region base=0x%lx "
				   "order=%lu flags=0x%lx\n", __func__,
				   dom->name, reg->base, reg->order,
				   reg->flags);
			return SBI_EINVAL;
		}
	}

	/* Count memory regions */
	count = sbi_domain_used_memregions(dom);

	/* Check presence of firmware regions */
	if (!dom->fw_region_inited) {
		sbi_printf("%s: %s does not have firmware region\n",
			   __func__, dom->name);
		return SBI_EINVAL;
	}

	/* Sort the memory regions */
	for (i = 0; i < (count - 1); i++) {
		reg = &dom->regions[i];
		for (j = i + 1; j < count; j++) {
			reg1 = &dom->regions[j];

			if (!is_region_before(reg1, reg))
				continue;

			swap_region(reg, reg1);
		}
	}

	/* Remove covered regions */
	for (i = 0; i < (count - 1);) {
		is_covered = false;
		reg = &dom->regions[i];

		for (j = i + 1; j < count; j++) {
			reg1 = &dom->regions[j];

			if (is_region_compatible(reg, reg1)) {
				is_covered = true;
				break;
			}
		}

		/* find a region is superset of reg, remove reg */
		if (is_covered) {
			for (j = i; j < (count - 1); j++)
				swap_region(&dom->regions[j],
					    &dom->regions[j + 1]);
			clear_region(&dom->regions[count - 1]);
			count--;
		} else
			i++;
	}

	/*
	 * We don't need to check boot HART id of domain because if boot
	 * HART id is not possible/assigned to this domain then it won't
	 * be started at boot-time by sbi_domain_finalize().
	 */

	/*
	 * Check next mode
	 *
	 * We only allow next mode to be S-mode or U-mode, so that we can
	 * protect M-mode context and enforce checks on memory accesses.
	 */
	if (dom->next_mode != PRV_S &&
	    dom->next_mode != PRV_U) {
		sbi_printf("%s: %s invalid next booting stage mode 0x%lx\n",
			   __func__, dom->name, dom->next_mode);
		return SBI_EINVAL;
	}

	/* Check next address and next mode */
	if (!sbi_domain_check_addr(dom, dom->next_addr, dom->next_mode,
				   SBI_DOMAIN_EXECUTE)) {
		sbi_printf("%s: %s next booting stage address 0x%lx can't "
			   "execute\n", __func__, dom->name, dom->next_addr);
		return SBI_EINVAL;
	}

	return 0;
}

bool sbi_domain_check_addr_range(const struct sbi_domain *dom,
				 unsigned long addr, unsigned long size,
				 unsigned long mode,
				 unsigned long access_flags)
{
	unsigned long max = addr + size;
	const struct sbi_domain_memregion *reg, *sreg;

	if (!dom)
		return false;

	while (addr < max) {
		reg = find_region(dom, addr);
		if (!reg)
			return false;

		if (!sbi_domain_check_addr(dom, addr, mode, access_flags))
			return false;

		sreg = find_next_subset_region(dom, reg, addr);
		if (sreg)
			addr = sreg->base;
		else if (reg->order < __riscv_xlen)
			addr = reg->base + (1UL << reg->order);
		else
			break;
	}

	return true;
}

void sbi_domain_dump(const struct sbi_domain *dom, const char *suffix)
{
	u32 i, j, k;
	unsigned long rstart, rend;
	struct sbi_domain_memregion *reg;

	sbi_printf("Domain%d Name        %s: %s\n",
		   dom->index, suffix, dom->name);

	sbi_printf("Domain%d Boot HART   %s: %d\n",
		   dom->index, suffix, dom->boot_hartid);

	k = 0;
	sbi_printf("Domain%d HARTs       %s: ", dom->index, suffix);
	sbi_hartmask_for_each_hartindex(i, dom->possible_harts) {
		j = sbi_hartindex_to_hartid(i);
		sbi_printf("%s%d%s", (k++) ? "," : "",
			   j, sbi_domain_is_assigned_hart(dom, i) ? "*" : "");
	}
	sbi_printf("\n");

	i = 0;
	sbi_domain_for_each_memregion(dom, reg) {
		rstart = reg->base;
		rend = (reg->order < __riscv_xlen) ?
			rstart + ((1UL << reg->order) - 1) : -1UL;

		sbi_printf("Domain%d Region%02d    %s: 0x%" PRILX "-0x%" PRILX " ",
			   dom->index, i, suffix, rstart, rend);

		k = 0;

		sbi_printf("M: ");
		if (reg->flags & SBI_DOMAIN_MEMREGION_MMIO)
			sbi_printf("%cI", (k++) ? ',' : '(');
		if (reg->flags & SBI_DOMAIN_MEMREGION_FW)
			sbi_printf("%cF", (k++) ? ',' : '(');
		if (reg->flags & SBI_DOMAIN_MEMREGION_M_READABLE)
			sbi_printf("%cR", (k++) ? ',' : '(');
		if (reg->flags & SBI_DOMAIN_MEMREGION_M_WRITABLE)
			sbi_printf("%cW", (k++) ? ',' : '(');
		if (reg->flags & SBI_DOMAIN_MEMREGION_M_EXECUTABLE)
			sbi_printf("%cX", (k++) ? ',' : '(');
		sbi_printf("%s ", (k++) ? ")" : "()");

		k = 0;
		sbi_printf("S/U: ");
		if (reg->flags & SBI_DOMAIN_MEMREGION_SU_READABLE)
			sbi_printf("%cR", (k++) ? ',' : '(');
		if (reg->flags & SBI_DOMAIN_MEMREGION_SU_WRITABLE)
			sbi_printf("%cW", (k++) ? ',' : '(');
		if (reg->flags & SBI_DOMAIN_MEMREGION_SU_EXECUTABLE)
			sbi_printf("%cX", (k++) ? ',' : '(');
		sbi_printf("%s\n", (k++) ? ")" : "()");

		i++;
	}

	sbi_printf("Domain%d Next Address%s: 0x%" PRILX "\n",
		   dom->index, suffix, dom->next_addr);

	sbi_printf("Domain%d Next Arg1   %s: 0x%" PRILX "\n",
		   dom->index, suffix, dom->next_arg1);

	sbi_printf("Domain%d Next Mode   %s: ", dom->index, suffix);
	switch (dom->next_mode) {
	case PRV_M:
		sbi_printf("M-mode\n");
		break;
	case PRV_S:
		sbi_printf("S-mode\n");
		break;
	case PRV_U:
		sbi_printf("U-mode\n");
		break;
	default:
		sbi_printf("Unknown\n");
		break;
	}

	sbi_printf("Domain%d SysReset    %s: %s\n",
		   dom->index, suffix, (dom->system_reset_allowed) ? "yes" : "no");

	sbi_printf("Domain%d SysSuspend  %s: %s\n",
		   dom->index, suffix, (dom->system_suspend_allowed) ? "yes" : "no");
}

void sbi_domain_dump_all(const char *suffix)
{
	const struct sbi_domain *dom;

	sbi_domain_for_each(dom) {
		sbi_domain_dump(dom, suffix);
		sbi_printf("\n");
	}
}

int sbi_domain_register(struct sbi_domain *dom,
			const struct sbi_hartmask *assign_mask)
{
	u32 i;
	int rc;
	struct sbi_domain *tdom;
	u32 cold_hartid = current_hartid();

	/* Sanity checks */
	if (!dom || !assign_mask || domain_finalized)
		return SBI_EINVAL;

	/* Check if domain already discovered */
	sbi_domain_for_each(tdom) {
		if (tdom == dom)
			return SBI_EALREADY;
	}

	/* Sanitize discovered domain */
	rc = sanitize_domain(dom);
	if (rc) {
		sbi_printf("%s: sanity checks failed for"
			   " %s (error %d)\n", __func__,
			   dom->name, rc);
		return rc;
	}

	sbi_list_add_tail(&dom->node, &domain_list);

	/* Assign index to domain */
	dom->index = domain_count++;

	/* Initialize spinlock for dom->assigned_harts */
	SPIN_LOCK_INIT(dom->assigned_harts_lock);

	/* Clear assigned HARTs of domain */
	sbi_hartmask_clear_all(&dom->assigned_harts);

	/* Assign domain to HART if HART is a possible HART */
	sbi_hartmask_for_each_hartindex(i, assign_mask) {
		if (!sbi_hartmask_test_hartindex(i, dom->possible_harts))
			continue;

		tdom = sbi_hartindex_to_domain(i);
		if (tdom)
			sbi_hartmask_clear_hartindex(i,
					&tdom->assigned_harts);
		sbi_update_hartindex_to_domain(i, dom);
		sbi_hartmask_set_hartindex(i, &dom->assigned_harts);

		/*
		 * If cold boot HART is assigned to this domain then
		 * override boot HART of this domain.
		 */
		if (sbi_hartindex_to_hartid(i) == cold_hartid &&
		    dom->boot_hartid != cold_hartid) {
			sbi_printf("Domain%d Boot HARTID forced to"
				   " %d\n", dom->index, cold_hartid);
			dom->boot_hartid = cold_hartid;
		}
	}

	/* Setup data for the discovered domain */
	rc = sbi_domain_setup_data(dom);
	if (rc) {
		sbi_printf("%s: domain data setup failed for %s (error %d)\n",
			   __func__, dom->name, rc);
		sbi_list_del(&dom->node);
		return rc;
	}

	return 0;
}

static int root_add_memregion(const struct sbi_domain_memregion *reg)
{
	int rc;
	bool reg_merged;
	struct sbi_domain_memregion *nreg, *nreg1, *nreg2;
	int root_memregs_count = sbi_domain_used_memregions(&root);

	/* Sanity checks */
	if (!reg || domain_finalized || !root.regions ||
	    (ROOT_REGION_MAX <= root_memregs_count))
		return SBI_EINVAL;

	/* Check whether compatible region exists for the new one */
	sbi_domain_for_each_memregion(&root, nreg) {
		if (is_region_compatible(reg, nreg))
			return 0;
	}

	/* Append the memregion to root memregions */
	nreg = &root.regions[root_memregs_count];
	sbi_memcpy(nreg, reg, sizeof(*reg));
	root_memregs_count++;
	root.regions[root_memregs_count].order = 0;

	/* Sort and optimize root regions */
	do {
		/* Sanitize the root domain so that memregions are sorted */
		rc = sanitize_domain(&root);
		if (rc) {
			sbi_printf("%s: sanity checks failed for"
				   " %s (error %d)\n", __func__,
				   root.name, rc);
			return rc;
		}

		/* Merge consecutive memregions with same order and flags */
		reg_merged = false;
		sbi_domain_for_each_memregion(&root, nreg) {
			nreg1 = nreg + 1;
			if (!nreg1->order)
				continue;

			if (!(nreg->base & (BIT(nreg->order + 1) - 1)) &&
			    (nreg->base + BIT(nreg->order)) == nreg1->base &&
			    nreg->order == nreg1->order &&
			    nreg->flags == nreg1->flags) {
				nreg->order++;
				while (nreg1->order) {
					nreg2 = nreg1 + 1;
					sbi_memcpy(nreg1, nreg2, sizeof(*nreg1));
					nreg1++;
				}
				reg_merged = true;
				root_memregs_count--;
			}
		}
	} while (reg_merged);

	return 0;
}

int sbi_domain_root_add_memrange(unsigned long addr, unsigned long size,
			   unsigned long align, unsigned long region_flags)
{
	int rc;
	unsigned long pos, end, rsize;
	struct sbi_domain_memregion reg;

	pos = addr;
	end = addr + size;
	while (pos < end) {
		rsize = pos & (align - 1);
		if (rsize)
			rsize = 1UL << sbi_ffs(pos);
		else
			rsize = ((end - pos) < align) ?
				(end - pos) : align;

		sbi_domain_memregion_init(pos, rsize, region_flags, &reg);
		rc = root_add_memregion(&reg);
		if (rc)
			return rc;
		pos += rsize;
	}

	return 0;
}

int sbi_domain_startup(struct sbi_scratch *scratch, u32 cold_hartid)
{
	int rc;
	u32 dhart;
	struct sbi_domain *dom;

	/* Sanity checks */
	if (!domain_finalized)
		return SBI_EINVAL;

	/* Startup boot HART of domains */
	sbi_domain_for_each(dom) {
		/* Domain boot HART index */
		dhart = sbi_hartid_to_hartindex(dom->boot_hartid);

		/* Ignore of boot HART is off limits */
		if (!sbi_hartindex_valid(dhart))
			continue;

		/* Ignore if boot HART not possible for this domain */
		if (!sbi_hartmask_test_hartindex(dhart, dom->possible_harts))
			continue;

		/* Ignore if boot HART assigned different domain */
		if (sbi_hartindex_to_domain(dhart) != dom)
			continue;

		/* Ignore if boot HART is not part of the assigned HARTs */
		spin_lock(&dom->assigned_harts_lock);
		rc = sbi_hartmask_test_hartindex(dhart, &dom->assigned_harts);
		spin_unlock(&dom->assigned_harts_lock);
		if (!rc)
			continue;

		/* Startup boot HART of domain */
		if (dom->boot_hartid == cold_hartid) {
			scratch->next_addr = dom->next_addr;
			scratch->next_mode = dom->next_mode;
			scratch->next_arg1 = dom->next_arg1;
		} else {
			rc = sbi_hsm_hart_start(scratch, NULL,
						dom->boot_hartid,
						dom->next_addr,
						dom->next_mode,
						dom->next_arg1);
			if (rc) {
				sbi_printf("%s: failed to start boot HART %d"
					   " for %s (error %d)\n", __func__,
					   dom->boot_hartid, dom->name, rc);
				return rc;
			}
		}
	}

	return 0;
}

int sbi_domain_finalize(struct sbi_scratch *scratch)
{
	int rc;
	const struct sbi_platform *plat = sbi_platform_ptr(scratch);

	/* Sanity checks */
	if (domain_finalized)
		return SBI_EINVAL;

	/* Initialize and populate domains for the platform */
	rc = sbi_platform_domains_init(plat);
	if (rc) {
		sbi_printf("%s: platform domains_init() failed (error %d)\n",
			   __func__, rc);
		return rc;
	}

	/*
	 * Set the finalized flag so that the root domain
	 * regions can't be changed.
	 */
	domain_finalized = true;

	return 0;
}

int sbi_domain_init(struct sbi_scratch *scratch, u32 cold_hartid)
{
	int rc;
	struct sbi_hartmask *root_hmask;
	struct sbi_domain_memregion *root_memregs;
	int root_memregs_count = 0;

	SBI_INIT_LIST_HEAD(&domain_list);

	if (scratch->fw_rw_offset == 0 ||
	    (scratch->fw_rw_offset & (scratch->fw_rw_offset - 1)) != 0) {
		sbi_printf("%s: fw_rw_offset is not a power of 2 (0x%lx)\n",
			   __func__, scratch->fw_rw_offset);
		return SBI_EINVAL;
	}

	if ((scratch->fw_start & (scratch->fw_rw_offset - 1)) != 0) {
		sbi_printf("%s: fw_start and fw_rw_offset not aligned\n",
			   __func__);
		return SBI_EINVAL;
	}

	domain_hart_ptr_offset = sbi_scratch_alloc_type_offset(void *);
	if (!domain_hart_ptr_offset)
		return SBI_ENOMEM;

	/* Initialize domain context support */
	rc = sbi_domain_context_init();
	if (rc)
		goto fail_free_domain_hart_ptr_offset;

	root_memregs = sbi_calloc(sizeof(*root_memregs), ROOT_REGION_MAX + 1);
	if (!root_memregs) {
		sbi_printf("%s: no memory for root regions\n", __func__);
		rc = SBI_ENOMEM;
		goto fail_deinit_context;
	}
	root.regions = root_memregs;

	root_hmask = sbi_zalloc(sizeof(*root_hmask));
	if (!root_hmask) {
		sbi_printf("%s: no memory for root hartmask\n", __func__);
		rc = SBI_ENOMEM;
		goto fail_free_root_memregs;
	}
	root.possible_harts = root_hmask;

	/* Root domain firmware memory region */
	sbi_domain_memregion_init(scratch->fw_start, scratch->fw_rw_offset,
				  (SBI_DOMAIN_MEMREGION_M_READABLE |
				   SBI_DOMAIN_MEMREGION_M_EXECUTABLE |
				   SBI_DOMAIN_MEMREGION_FW),
				  &root_memregs[root_memregs_count++]);

	sbi_domain_memregion_init((scratch->fw_start + scratch->fw_rw_offset),
				  (scratch->fw_size - scratch->fw_rw_offset),
				  (SBI_DOMAIN_MEMREGION_M_READABLE |
				   SBI_DOMAIN_MEMREGION_M_WRITABLE |
				   SBI_DOMAIN_MEMREGION_FW),
				  &root_memregs[root_memregs_count++]);

	root.fw_region_inited = true;

	/*
	 * Allow SU RWX on rest of the memory region. Since pmp entries
	 * have implicit priority on index, previous entries will
	 * deny access to SU on M-mode region. Also, M-mode will not
	 * have access to SU region while previous entries will allow
	 * access to M-mode regions.
	 */
	sbi_domain_memregion_init(0, ~0UL,
				  (SBI_DOMAIN_MEMREGION_SU_READABLE |
				   SBI_DOMAIN_MEMREGION_SU_WRITABLE |
				   SBI_DOMAIN_MEMREGION_SU_EXECUTABLE),
				  &root_memregs[root_memregs_count++]);

	/* Root domain memory region end */
	root_memregs[root_memregs_count].order = 0;

	/* Root domain boot HART id is same as coldboot HART id */
	root.boot_hartid = cold_hartid;

	/* Root domain next booting stage details */
	root.next_arg1 = scratch->next_arg1;
	root.next_addr = scratch->next_addr;
	root.next_mode = scratch->next_mode;

	/* Root domain possible and assigned HARTs */
	sbi_for_each_hartindex(i)
		sbi_hartmask_set_hartindex(i, root_hmask);

	/* Finally register the root domain */
	rc = sbi_domain_register(&root, root_hmask);
	if (rc)
		goto fail_free_root_hmask;

	return 0;

fail_free_root_hmask:
	sbi_free(root_hmask);
fail_free_root_memregs:
	sbi_free(root_memregs);
fail_deinit_context:
	sbi_domain_context_deinit();
fail_free_domain_hart_ptr_offset:
	sbi_scratch_free_offset(domain_hart_ptr_offset);
	return rc;
}
