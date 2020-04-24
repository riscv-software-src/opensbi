/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/timer/fdt_timer.h>
#include <sbi_utils/sys/clint.h>

static int timer_clint_cold_init(void *fdt, int nodeoff,
				  const struct fdt_match *match)
{
	int rc;
	u32 max_hartid;
	unsigned long addr;

	rc = fdt_parse_max_hart_id(fdt, &max_hartid);
	if (rc)
		return rc;

	rc = fdt_get_node_addr_size(fdt, nodeoff, &addr, NULL);
	if (rc)
		return rc;

	/* TODO: We should figure-out CLINT has_64bit_mmio from DT node */
	return clint_cold_timer_init(addr, max_hartid + 1, TRUE);
}

static const struct fdt_match timer_clint_match[] = {
	{ .compatible = "riscv,clint0" },
	{ },
};

struct fdt_timer fdt_timer_clint = {
	.match_table = timer_clint_match,
	.cold_init = timer_clint_cold_init,
	.warm_init = clint_warm_timer_init,
	.exit = NULL,
	.value = clint_timer_value,
	.event_stop = clint_timer_event_stop,
	.event_start = clint_timer_event_start,
};
