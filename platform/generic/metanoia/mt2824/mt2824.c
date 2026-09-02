// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (C) 2026 Metanoia Communication Inc.
 *
 * Authors:
 *      Jun Chang <jun.chang@metanoia-comm.com>
 */

#include <andes/andes_pma.h>
#include <andes/andes_pmu.h>
#include <andes/andes_sbi.h>
#include <platform_override.h>
#include <sbi_utils/fdt/fdt_helper.h>

#define MT2824_XOR_RAM_BASE		0x00100000ULL
#define MT2824_XOR_RAM_SIZE		0x00040000ULL

#define MT2824_MRAS_MEM_BASE		0x60000000ULL
#define MT2824_MRAS_MEM_SIZE		0x20000000ULL

#define MT2824_RSCTABLE_BASE		0x81000000ULL
#define MT2824_RSCTABLE_SIZE		0x00010000ULL

static const struct andes_pma_region metanoia_mt2824_pma_regions[] = {
	{
		.pa = MT2824_XOR_RAM_BASE,
		.size = MT2824_XOR_RAM_SIZE,
		.flags = ANDES_PMACFG_ETYP_NAPOT |
			 ANDES_PMACFG_MTYP_MEM_NON_CACHE_BUF,
	},
	{
		.pa = MT2824_MRAS_MEM_BASE,
		.size = MT2824_MRAS_MEM_SIZE,
		.flags = ANDES_PMACFG_ETYP_NAPOT |
			 ANDES_PMACFG_MTYP_MEM_NON_CACHE_BUF,
	},
	{
		.pa = MT2824_RSCTABLE_BASE,
		.size = MT2824_RSCTABLE_SIZE,
		.flags = ANDES_PMACFG_ETYP_NAPOT |
			 ANDES_PMACFG_MTYP_MEM_NON_CACHE_BUF,
	},
};

static int metanoia_mt2824_final_init(bool cold_boot)
{
	void *fdt = fdt_get_address_rw();
	int rc;

	/* Set the DSP memory regions attributes */
	rc = andes_pma_setup_regions(fdt, metanoia_mt2824_pma_regions,
				     array_size(metanoia_mt2824_pma_regions));
	if (rc)
		return rc;

	return generic_final_init(cold_boot);
}

static int metanoia_mt2824_platform_init(const void *fdt, int nodeoff,
					const struct fdt_match *match)
{
	generic_platform_ops.final_init = metanoia_mt2824_final_init;
	generic_platform_ops.extensions_init = andes_pmu_extensions_init;
	generic_platform_ops.pmu_init = andes_pmu_init;
	generic_platform_ops.vendor_ext_provider = andes_sbi_vendor_ext_provider;

	return 0;
}

static const struct fdt_match metanoia_mt2824_match[] = {
	{ .compatible = "metanoia,mt2824" },
	{ /* sentinel */ }
};

const struct fdt_driver metanoia_mt2824 = {
	.match_table = metanoia_mt2824_match,
	.init = metanoia_mt2824_platform_init,
};
