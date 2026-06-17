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
#include <sbi/sbi_pmu.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_string.h>
#include <sbi/sbi_tlb.h>
#include <sbi_utils/fdt/fdt_helper.h>

struct thead_generic_quirks {
	u64	errata;
};

static int thead_tlb_flush_early_init(bool cold_boot)
{
	thead_register_tlb_flush_trap_handler();

	return generic_early_init(cold_boot);
}

static int thead_pmu_extensions_init(bool cold_boot)
{
	int rc;

	rc = generic_extensions_init(cold_boot);
	if (rc)
		return rc;

	thead_c9xx_register_pmu_device();

	return 0;
}

static void thead_jtlb_local_sfence_vma(struct sbi_tlb_info *tinfo)
{
	sbi_pmu_ctr_incr_fw(SBI_PMU_FW_SFENCE_VMA_RCVD);
	__asm__ __volatile__("sfence.vma");
}

static void thead_jtlb_local_sfence_vma_asid(struct sbi_tlb_info *tinfo)
{
	sbi_pmu_ctr_incr_fw(SBI_PMU_FW_SFENCE_VMA_ASID_RCVD);
	/* Flush entire MM context for a given ASID */
	__asm__ __volatile__("sfence.vma x0, %0"
			     :
			     : "r"(tinfo->asid)
			     : "memory");
}

static int thead_generic_platform_init(const void *fdt, int nodeoff,
				       const struct fdt_match *match)
{
	const struct thead_generic_quirks *quirks = match->data;
	static struct sbi_tlb_local_operations tlb_local_ops;

	if (quirks->errata & THEAD_QUIRK_ERRATA_TLB_FLUSH)
		generic_platform_ops.early_init = thead_tlb_flush_early_init;
	if (quirks->errata & THEAD_QUIRK_ERRATA_THEAD_PMU)
		generic_platform_ops.extensions_init = thead_pmu_extensions_init;
	if (quirks->errata &THEAD_QUIRK_ERRATA_JTLB) {
		sbi_memcpy(&tlb_local_ops, sbi_tlb_get_local_operations(), sizeof(tlb_local_ops));
		tlb_local_ops.local_sfence_vma = thead_jtlb_local_sfence_vma;
		tlb_local_ops.local_sfence_vma_asid = thead_jtlb_local_sfence_vma_asid;
		sbi_tlb_set_local_operations(&tlb_local_ops);
	}

	return 0;
}

static const struct thead_generic_quirks thead_th1520_quirks = {
	.errata = THEAD_QUIRK_ERRATA_TLB_FLUSH | THEAD_QUIRK_ERRATA_THEAD_PMU,
};

static const struct thead_generic_quirks thead_pmu_quirks = {
	.errata = THEAD_QUIRK_ERRATA_THEAD_PMU,
};

static const struct thead_generic_quirks thead_pmu_jtlb_quirks = {
	.errata = THEAD_QUIRK_ERRATA_THEAD_PMU | THEAD_QUIRK_ERRATA_JTLB,
};

static const struct fdt_match thead_generic_match[] = {
	{ .compatible = "canaan,kendryte-k230", .data = &thead_pmu_quirks },
	{ .compatible = "sophgo,cv1800b", .data = &thead_pmu_quirks },
	{ .compatible = "sophgo,cv1812h", .data = &thead_pmu_quirks },
	{ .compatible = "sophgo,sg2000", .data = &thead_pmu_quirks },
	{ .compatible = "sophgo,sg2002", .data = &thead_pmu_quirks },
	{ .compatible = "sophgo,sg2044", .data = &thead_pmu_jtlb_quirks },
	{ .compatible = "thead,th1520", .data = &thead_th1520_quirks },
	{ },
};

const struct fdt_driver thead_generic = {
	.match_table		= thead_generic_match,
	.init			= thead_generic_platform_init,
};
