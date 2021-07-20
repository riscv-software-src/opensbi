/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2021 SiFive
 *
 * Authors:
 *   David Abdurachmanov <david.abdurachmanov@sifive.com>
 */

#include <platform_override.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/fdt/fdt_fixup.h>

static u64 sifive_fu740_tlbr_flush_limit(const struct fdt_match *match)
{
	/*
	 * Needed to address CIP-1200 errata on SiFive FU740
	 * Title: Instruction TLB can fail to respect a non-global SFENCE
	 * Workaround: Flush the TLB using SFENCE.VMA x0, x0
	 * See Errata_FU740-C000_20210205 from
	 * https://www.sifive.com/boards/hifive-unmatched
	 */
	return 0;
}

static const struct fdt_match sifive_fu740_match[] = {
	{ .compatible = "sifive,fu740" },
	{ .compatible = "sifive,fu740-c000" },
	{ .compatible = "sifive,hifive-unmatched-a00" },
	{ },
};

const struct platform_override sifive_fu740 = {
	.match_table = sifive_fu740_match,
	.tlbr_flush_limit = sifive_fu740_tlbr_flush_limit,
};
