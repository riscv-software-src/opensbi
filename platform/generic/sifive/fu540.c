/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <platform_override.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/fdt/fdt_fixup.h>

static u64 sifive_fu540_tlbr_flush_limit(const struct fdt_match *match)
{
	/*
	 * The sfence.vma by virtual address does not work on
	 * SiFive FU540 so we return remote TLB flush limit as zero.
	 */
	return 0;
}

static const struct fdt_match sifive_fu540_match[] = {
	{ .compatible = "sifive,fu540" },
	{ .compatible = "sifive,fu540g" },
	{ .compatible = "sifive,fu540-c000" },
	{ .compatible = "sifive,hifive-unleashed-a00" },
	{ },
};

const struct platform_override sifive_fu540 = {
	.match_table = sifive_fu540_match,
	.tlbr_flush_limit = sifive_fu540_tlbr_flush_limit,
};
