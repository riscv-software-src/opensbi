/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2026 SiFive Inc.
 */

#include <platform_override.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_scratch.h>

/*
 * Instead of allocating individual PMP entries for each M-mode
 * MMIO driver, configure SMEPMP regions to grant R/W permission
 * to peripheral and system ports for all privilege modes.
 * Finer-grained access control is enforced by wgChecker rules.
 */
static int sifive_smepmp_setup(void)
{
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();
	int rc;

	if (!sbi_hart_has_extension(scratch, SBI_HART_EXT_SMEPMP))
		return 0;

	/* Peripheral port region: 0 to 2GiB */
	rc = sbi_domain_root_add_memrange(0x0, 0x80000000, 0x80000000,
					  SBI_DOMAIN_MEMREGION_MMIO |
					  SBI_DOMAIN_MEMREGION_SHARED_SURW_MRW);
	if (rc)
		return rc;

#if __riscv_xlen > 32
	/* System port region: 512GiB to 1TiB */
	rc = sbi_domain_root_add_memrange(0x8000000000ULL, 0x8000000000ULL, 0x8000000000ULL,
					  SBI_DOMAIN_MEMREGION_MMIO |
					  SBI_DOMAIN_MEMREGION_SHARED_SURW_MRW);
	if (rc)
		return rc;
#endif

	return 0;
}

static int sifive_early_init(bool cold_boot)
{
	int rc;

	if (cold_boot) {
		rc = sifive_smepmp_setup();
		if (rc)
			return rc;
	}

	return generic_early_init(cold_boot);
}

static int sifive_platform_init(const void *fdt, int nodeoff,
				const struct fdt_match *match)
{
	generic_platform_ops.early_init = sifive_early_init;

	return 0;
}

static const struct fdt_match sifive_dev_platform_match[] = {
	{ .compatible = "sifive-dev" },
	{ },
};

const struct fdt_driver sifive_dev_platform = {
	.match_table = sifive_dev_platform_match,
	.init = sifive_platform_init,
};
