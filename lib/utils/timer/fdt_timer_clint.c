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

static struct clint_data clint_timer;

static int timer_clint_cold_init(void *fdt, int nodeoff,
				  const struct fdt_match *match)
{
	int rc;

	rc = fdt_parse_clint_node(fdt, nodeoff, TRUE, &clint_timer);
	if (rc)
		return rc;

	return clint_cold_timer_init(&clint_timer, NULL);
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
