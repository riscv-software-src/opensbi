/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Andes Technology Corporation
 *
 * Authors:
 *   Zong Li <zong@andestech.com>
 */

#ifndef _AE350_PLMT_H_
#define _AE350_PLMT_H_

u64 plmt_timer_value(void);

void plmt_timer_event_stop(void);

void plmt_timer_event_start(u64 next_event);

int plmt_warm_timer_init(void);

int plmt_cold_timer_init(unsigned long base, u32 hart_count);

#endif /* _AE350_PLMT_H_ */
