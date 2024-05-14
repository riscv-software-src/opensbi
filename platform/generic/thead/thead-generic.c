/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Authors:
 *   Inochi Amaoto <inochiama@outlook.com>
 *
 */

#include <platform_override.h>
#include <thead/c9xx_errata.h>
#include <thead/c9xx_pmu.h>
#include <sbi/sbi_const.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_string.h>
#include <sbi_utils/fdt/fdt_helper.h>

struct thead_generic_quirks {
	u64	errata;
};

static int thead_generic_early_init(bool cold_boot,
				    const struct fdt_match *match)
{
	struct thead_generic_quirks *quirks = (void *)match->data;

	if (quirks->errata & THEAD_QUIRK_ERRATA_TLB_FLUSH)
		thead_register_tlb_flush_trap_handler();

	return 0;
}

static int thead_generic_extensions_init(const struct fdt_match *match,
					 struct sbi_hart_features *hfeatures)
{
	struct thead_generic_quirks *quirks = (void *)match->data;

	if (quirks->errata & THEAD_QUIRK_ERRATA_THEAD_PMU)
		thead_c9xx_register_pmu_device();

	return 0;
}

static struct thead_generic_quirks thead_th1520_quirks = {
	.errata = THEAD_QUIRK_ERRATA_TLB_FLUSH | THEAD_QUIRK_ERRATA_THEAD_PMU,
};

static struct thead_generic_quirks canaan_k230_quirks = {
	.errata = THEAD_QUIRK_ERRATA_THEAD_PMU,
};

static struct thead_generic_quirks sophgo_cv1800_quirks = {
	.errata = THEAD_QUIRK_ERRATA_THEAD_PMU,
};

static const struct fdt_match thead_generic_match[] = {
	{ .compatible = "sophgo,cv1800b", .data = &sophgo_cv1800_quirks },
	{ .compatible = "sophgo,cv1812h", .data = &sophgo_cv1800_quirks },
	{ .compatible = "sophgo,sg2000", .data = &sophgo_cv1800_quirks },
	{ .compatible = "sophgo,sg2002", .data = &sophgo_cv1800_quirks },
	{ .compatible = "thead,th1520", .data = &thead_th1520_quirks },
	{ .compatible = "canaan,kendryte-k230", .data = &canaan_k230_quirks },
	{ },
};

const struct platform_override thead_generic = {
	.match_table		= thead_generic_match,
	.early_init		= thead_generic_early_init,
	.extensions_init	= thead_generic_extensions_init,
};
