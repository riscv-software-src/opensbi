/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 Andes Technology Corporation
 *
 */

#include <andes/andes_pma.h>
#include <andes/andes_pmu.h>
#include <andes/andes_sbi.h>
#include <andes/qilai.h>
#include <platform_override.h>
#include <sbi/sbi_domain.h>
#include <sbi_utils/fdt/fdt_driver.h>
#include <sbi_utils/fdt/fdt_helper.h>

static int andes_qilai_final_init(bool cold_boot)
{

	int rc;

	/*
	 * Set the memory attribute for 3 PCIE endpoint regions,
	 * and they are all non-idempotent and non-bufferable.
	 */
	rc = andes_sbi_set_pma((unsigned long)PCIE0_BASE, (unsigned long)PCIE0_SIZE,
			       ANDES_PMACFG_ETYP_NAPOT |
			       ANDES_PMACFG_MTYP_DEV_NOBUF);
	if (rc)
		return rc;

	rc = andes_sbi_set_pma((unsigned long)PCIE1_BASE, (unsigned long)PCIE1_SIZE,
			       ANDES_PMACFG_ETYP_NAPOT |
			       ANDES_PMACFG_MTYP_DEV_NOBUF);
	if (rc)
		return rc;

	rc = andes_sbi_set_pma((unsigned long)PCIE2_BASE, (unsigned long)PCIE2_SIZE,
			       ANDES_PMACFG_ETYP_NAPOT |
			       ANDES_PMACFG_MTYP_DEV_NOBUF);
	if (rc)
		return rc;

	return generic_final_init(cold_boot);
}

static int andes_qilai_platform_init(const void *fdt, int nodeoff,
				     const struct fdt_match *match)
{
	generic_platform_ops.final_init	     = andes_qilai_final_init;
	generic_platform_ops.extensions_init = andes_pmu_extensions_init;
	generic_platform_ops.pmu_init	     = andes_pmu_init;
	generic_platform_ops.vendor_ext_provider =
		andes_sbi_vendor_ext_provider;
	return 0;
}

static const struct fdt_match andes_qilai_match[] = {
	{ .compatible = "andestech,qilai" },
	{},
};

const struct fdt_driver andes_qilai = {
	.match_table = andes_qilai_match,
	.init	     = andes_qilai_platform_init,
};
