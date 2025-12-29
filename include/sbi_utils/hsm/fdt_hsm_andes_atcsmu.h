/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 Andes Technology Corporation
 */

#ifndef __FDT_HSM_ANDES_ATCSMU_H__
#define __FDT_HSM_ANDES_ATCSMU_H__

#include <sbi/sbi_types.h>

/* clang-format off */

#define SCRATCH_PAD_OFFSET		0x40

#define RESET_VEC_LO_OFFSET		0x50
#define RESET_VEC_HI_OFFSET		0x60
#define RESET_VEC_8CORE_OFFSET		0x1a0
#define HARTn_RESET_VEC_LO(n)		(RESET_VEC_LO_OFFSET + \
					((n) < 4 ? 0 : RESET_VEC_8CORE_OFFSET) + \
					((n) * 0x4))
#define HARTn_RESET_VEC_HI(n)		(RESET_VEC_HI_OFFSET + \
					((n) < 4 ? 0 : RESET_VEC_8CORE_OFFSET) + \
					((n) * 0x4))

#define PCS0_CFG_OFFSET			0x80
#define PCSm_CFG_OFFSET(i)		((i + 3) * 0x20 + PCS0_CFG_OFFSET)
#define PCS_CFG_LIGHT_SLEEP		BIT(2)
#define PCS_CFG_DEEP_SLEEP		BIT(3)

#define PCS0_SCRATCH_OFFSET		0x84
#define PCSm_SCRATCH_OFFSET(i)		((i + 3) * 0x20 + PCS0_SCRATCH_OFFSET)

#define PCS0_WE_OFFSET			0x90
#define PCSm_WE_OFFSET(i)		((i + 3) * 0x20 + PCS0_WE_OFFSET)
#define PCS_WAKEUP_RTC_ALARM_MASK	BIT(2)
#define PCS_WAKEUP_UART2_MASK		BIT(9)
#define PCS_WAKEUP_MSIP_MASK		BIT(29)

#define PCS0_CTL_OFFSET			0x94
#define PCSm_CTL_OFFSET(i)		((i + 3) * 0x20 + PCS0_CTL_OFFSET)
#define LIGHT_SLEEP_CMD			0x3
#define WAKEUP_CMD			0x8
#define DEEP_SLEEP_CMD			0xb

#define PCS0_STATUS_OFFSET		0x98
#define PCSm_STATUS_OFFSET(i)		((i + 3) * 0x20 + PCS0_STATUS_OFFSET)
#define PD_TYPE_MASK			GENMASK(2, 0)
#define PD_TYPE_SLEEP			2
#define PD_STATUS_MASK			GENMASK(7, 3)
#define PD_STATUS_LIGHT_SLEEP		0
#define PD_STATUS_DEEP_SLEEP		0x10

/* clang-format on */

void atcsmu_set_wakeup_events(u32 events, u32 hartid);
bool atcsmu_support_sleep_mode(u32 sleep_type, u32 hartid);
void atcsmu_set_command(u32 pcs_ctl, u32 hartid);
int atcsmu_set_reset_vector(u64 wakeup_addr, u32 hartid);
u32 atcsmu_get_sleep_type(u32 hartid);
void atcsmu_write_scratch(u32 value);
u32 atcsmu_read_scratch(void);
bool atcsmu_pcs_is_sleep(u32 hartid, bool deep_sleep);

#endif
