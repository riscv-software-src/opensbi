/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Nuclei Corporation or its affiliates.
 *
 * Authors:
 *   lujun <lujun@nucleisys.com>
 */

#ifndef __SYS_NUCLEI_TIMER_H__
#define __SYS_NUCLEI_TIMER_H__

#include <sbi/sbi_types.h>

/**
 * @brief send the ipi to target hart
 * @param[in] target_hart  target hart id
 */
void nuclei_timer_ipi_send(u32 target_hart);

/**
 * @brief clear the ipi for target hart
 * @param[in] target_hart  target hart id
 */
void nuclei_timer_ipi_clear(u32 target_hart);

/**
 * set ipi init for warm boot
 * @return  init result
 * 	@retval 0:  init successful
 * 	@retval -1: init failed
 */
int nuclei_timer_warm_ipi_init(void);

/**
 * @brief ini init for cold boot
 * @param[in] base 		  ipi register base
 * @param[in] hart_count  hart count
 * @return  init result
 * 	@retval 0:  init successful
 * 	@retval -1: init failed
 */
int nuclei_timer_cold_ipi_init(unsigned long base, u32 hart_count);

/**
 * @brief get the timer counter value
 * @return timer counter value
 */
u64 nuclei_timer_timer_value(void);

/**
 * @brief clear the timer interrupt event, set the timecmp to all 1
 */
void nuclei_timer_timer_event_stop(void);

/**
 * @brief set the timer timecmp value
 * @param[in] next_event set timercmp value
 */
void nuclei_timer_timer_event_start(u64 next_event);

/**
 * @brief timer init for warm boot
 * @return  init result
 * 	@retval 0:  init successful
 * 	@retval -1: init failed
 */
int nuclei_timer_warm_timer_init(void);

/**
 * @brief timer init for cold boot
 * @return  init result
 * 	@retval 0:  init successful
 * 	@retval -1: init failed
 */
int nuclei_timer_cold_timer_init(unsigned long base, u32 hart_count,
			  bool has_64bit_mmio);

#endif
