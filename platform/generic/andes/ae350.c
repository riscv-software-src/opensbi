/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 Andes Technology Corporation
 */

#include <andes/andes.h>
#include <andes/andes_pmu.h>
#include <andes/andes_sbi.h>
#include <platform_override.h>
#include <sbi_utils/fdt/fdt_helper.h>

extern void _start_warm(void);

void ae350_enable_coherency_warmboot(void)
{
	ae350_enable_coherency();
	_start_warm();
}

static int ae350_platform_init(const void *fdt, int nodeoff, const struct fdt_match *match)
{
	generic_platform_ops.extensions_init = andes_pmu_extensions_init;
	generic_platform_ops.pmu_init = andes_pmu_init;
	generic_platform_ops.vendor_ext_provider = andes_sbi_vendor_ext_provider;

	return 0;
}

static const struct fdt_match andes_ae350_match[] = {
	{ .compatible = "andestech,ae350" },
	{ },
};

const struct fdt_driver andes_ae350 = {
	.match_table = andes_ae350_match,
	.init = ae350_platform_init,
};
