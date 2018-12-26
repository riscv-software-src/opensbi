/*
 * Copyright (c) 2018 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef __SBI_TIMER_H__
#define __SBI_TIMER_H__

#include <sbi/sbi_types.h>

struct sbi_scratch;

u64 sbi_timer_value(struct sbi_scratch *scratch);

void sbi_timer_event_stop(struct sbi_scratch *scratch, u32 hartid);

void sbi_timer_event_start(struct sbi_scratch *scratch, u32 hartid,
			   u64 next_event);

void sbi_timer_process(struct sbi_scratch *scratch, u32 hartid);

int sbi_timer_init(struct sbi_scratch *scratch, u32 hartid,
		   bool cold_boot);

#endif
