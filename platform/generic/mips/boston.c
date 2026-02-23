/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 MIPS
 *
 */

#include <platform_override.h>
#include <sbi/riscv_barrier.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_hsm.h>
#include <sbi/sbi_timer.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <mips/p8700.h>
#include <mips/mips-cm.h>

/* Use in nascent init - not have DTB yet */
#define DRAM_ADDRESS		0x80000000
#define DRAM_SIZE		0x80000000
#define DRAM_PMP_ADDR		((DRAM_ADDRESS >> 2) | ((DRAM_SIZE - 1) >> 3))

static const struct sbi_hsm_device mips_hsm = {
	.name		= "mips_hsm",
	.hart_start	= mips_p8700_hart_start,
	.hart_stop	= mips_p8700_hart_stop,
};

static int boston_final_init(bool cold_boot)
{
	if (cold_boot)
		sbi_hsm_set_device(&mips_hsm);

	return generic_final_init(cold_boot);
}

static int boston_early_init(bool cold_boot)
{
	int rc;

	rc = generic_early_init(cold_boot);
	if (rc)
		return rc;

	if (cold_boot) {
		unsigned long cm_base = p8700_cm_info->gcr_base[0];

		/* For the CPC mtime region, the minimum size is 0x10000. */
		rc = sbi_domain_root_add_memrange(cm_base, SIZE_FOR_CPC_MTIME,
						  P8700_ALIGN,
						  (SBI_DOMAIN_MEMREGION_MMIO |
						   SBI_DOMAIN_MEMREGION_M_READABLE |
						   SBI_DOMAIN_MEMREGION_M_WRITABLE));
		if (rc)
			return rc;

		/* For the APLIC and ACLINT m-mode region */
		rc = sbi_domain_root_add_memrange(cm_base + AIA_OFFSET, SIZE_FOR_AIA_M_MODE,
						  P8700_ALIGN,
						  (SBI_DOMAIN_MEMREGION_MMIO |
						   SBI_DOMAIN_MEMREGION_M_READABLE |
						   SBI_DOMAIN_MEMREGION_M_WRITABLE));
		if (rc)
			return rc;

	}

	return 0;
}

static int boston_nascent_init(void)
{
	u64 hartid = current_hartid();
	unsigned long cm_base = p8700_cm_info->gcr_base[0];
	int i;

	/* Coherence enable for every core */
	if (cpu_hart(hartid) == 0) {
		cm_base += (cpu_core(hartid) << CM_BASE_CORE_SHIFT);
		__raw_writeq(GCR_CORE_COH_EN_EN,
			     (void *)(cm_base + GCR_OFF_LOCAL +
				      GCR_CORE_COH_EN));
		mb();
	}

	/* Set up pmp for DRAM */
	csr_write(CSR_PMPADDR14, DRAM_PMP_ADDR);
	/* All from 0x0 */
	csr_write(CSR_PMPADDR15, 0x1fffffffffffffff);
	csr_write(CSR_PMPCFG2, ((PMP_A_NAPOT|PMP_R|PMP_W|PMP_X)<<56)|
		  ((PMP_A_NAPOT|PMP_R|PMP_W|PMP_X)<<48));
	/* Set cacheable for pmp6, uncacheable for pmp7 */
	csr_write(CSR_MIPSPMACFG2, ((u64)CCA_CACHE_DISABLE << 56)|
		  ((u64)CCA_CACHE_ENABLE << 48));
	/* Reset pmpcfg0 */
	csr_write(CSR_PMPCFG0, 0);
	/* Reset pmacfg0 */
	csr_write(CSR_MIPSPMACFG0, 0);
	mb();

	/* Per cluster set up */
	if (cpu_core(hartid) == 0 && cpu_hart(hartid) == 0) {
		/* Enable L2 prefetch */
		__raw_writel(0xfffff110,
			     (void *)(cm_base + L2_PFT_CONTROL_OFFSET));
		__raw_writel(0x15ff,
			     (void *)(cm_base + L2_PFT_CONTROL_B_OFFSET));
	}

	/* Per core set up */
	if (cpu_hart(hartid) == 0) {
		/* Enable load pair, store pair, and HTW */
		csr_clear(CSR_MIPSCONFIG7, (1<<12)|(1<<13)|(1<<7));

		/* Disable noRFO, misaligned load/store */
		csr_set(CSR_MIPSCONFIG7, (1<<25)|(1<<9));

		/* Enable L1-D$ Prefetch */
		csr_write(CSR_MIPSCONFIG11, 0xff);

		for (i = 0; i < 8; i++) {
			csr_set(CSR_MIPSCONFIG8, 4 + 0x100 * i);
			csr_set(CSR_MIPSCONFIG9, 8);
			mb();
			RISCV_FENCE_I;
		}
	}

	/* Per hart set up */
	/* Enable AMO and RDTIME illegal instruction exceptions. */
	csr_set(CSR_MIPSCONFIG6, (1<<2)|(1<<1));

	return 0;
}

static int boston_platform_init(const void *fdt, int nodeoff, const struct fdt_match *match)
{
	int rc = mips_p8700_platform_init(fdt, nodeoff, match);

	if (rc)
		return rc;
	generic_platform_ops.early_init = boston_early_init;
	generic_platform_ops.final_init = boston_final_init;
	generic_platform_ops.nascent_init = boston_nascent_init;
	generic_platform_ops.pmp_set = mips_p8700_pmp_set;

	return 0;
}

static unsigned long boston_gcr_base[] = {
	0x16100000,
};

static struct p8700_cm_info boston_cm_info = {
	.num_cm = array_size(boston_gcr_base),
	.gcr_base = boston_gcr_base,
};

static const struct fdt_match boston_match[] = {
	{ .compatible = "mips,p8700", .data = &boston_cm_info },
	{ },
};

const struct fdt_driver mips_p8700_boston = {
	.match_table = boston_match,
	.init = boston_platform_init,
};
