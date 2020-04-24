/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#ifndef __FDT_TIMER_H__
#define __FDT_TIMER_H__

#include <sbi/sbi_types.h>

struct fdt_timer {
	const struct fdt_match *match_table;
	int (*cold_init)(void *fdt, int nodeoff, const struct fdt_match *match);
	int (*warm_init)(void);
	void (*exit)(void);
	u64 (*value)(void);
	void (*event_stop)(void);
	void (*event_start)(u64 next_event);
};

u64 fdt_timer_value(void);

void fdt_timer_event_stop(void);

void fdt_timer_event_start(u64 next_event);

void fdt_timer_exit(void);

int fdt_timer_init(bool cold_boot);

#endif
