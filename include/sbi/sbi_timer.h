/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#ifndef __SBI_TIMER_H__
#define __SBI_TIMER_H__

#include <sbi/sbi_types.h>

struct sbi_scratch;

u64 sbi_timer_value(struct sbi_scratch *scratch);

u64 sbi_timer_virt_value(struct sbi_scratch *scratch);

u64 sbi_timer_get_delta(struct sbi_scratch *scratch);

void sbi_timer_set_delta(struct sbi_scratch *scratch, ulong delta);

void sbi_timer_set_delta_upper(struct sbi_scratch *scratch, ulong delta_upper);

void sbi_timer_event_stop(struct sbi_scratch *scratch);

void sbi_timer_event_start(struct sbi_scratch *scratch, u64 next_event);

void sbi_timer_process(struct sbi_scratch *scratch);

int sbi_timer_init(struct sbi_scratch *scratch, bool cold_boot);

#endif
