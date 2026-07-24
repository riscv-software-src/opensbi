// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (C) 2026 Metanoia Communication Inc.
 *
 * Authors:
 *      Jun Chang <jun.chang@metanoia-comm.com>
 */

#include <andes/andes_pmu.h>
#include <andes/andes_sbi.h>
#include <platform_override.h>

static int metanoia_mt2824_platform_init(const void *fdt, int nodeoff,
					const struct fdt_match *match)
{
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
