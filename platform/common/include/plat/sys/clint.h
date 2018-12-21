/*
 * Copyright (c) 2018 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef __SYS_CLINT_H__
#define __SYS_CLINT_H__

#include <sbi/sbi_types.h>

void clint_ipi_inject(u32 target_hart, u32 source_hart);

void clint_ipi_sync(u32 target_hart, u32 source_hart);

void clint_ipi_clear(u32 target_hart);

int clint_warm_ipi_init(u32 target_hart);

int clint_cold_ipi_init(unsigned long base, u32 hart_count);

u64 clint_timer_value(void);

void clint_timer_event_stop(u32 target_hart);

void clint_timer_event_start(u32 target_hart, u64 next_event);

int clint_warm_timer_init(u32 target_hart);

int clint_cold_timer_init(unsigned long base, u32 hart_count);

#endif
