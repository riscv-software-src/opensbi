/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2023 Andes Technology Corporation
 */

#ifndef _SYS_ATCSMU_H
#define _SYS_ATCSMU_H

#include <sbi/sbi_types.h>

/* clang-format off */

#define PCS0_WE_OFFSET		0x90
#define PCSm_WE_OFFSET(i)	((i + 3) * 0x20 + PCS0_WE_OFFSET)

#define PCS0_CTL_OFFSET		0x94
#define PCSm_CTL_OFFSET(i)	((i + 3) * 0x20 + PCS0_CTL_OFFSET)
#define PCS_CTL_CMD_SHIFT	0
#define PCS_CTL_PARAM_SHIFT	3
#define SLEEP_CMD		0x3
#define WAKEUP_CMD		(0x0 | (1 << PCS_CTL_PARAM_SHIFT))
#define LIGHTSLEEP_MODE		0
#define DEEPSLEEP_MODE		1
#define LIGHT_SLEEP_CMD		(SLEEP_CMD | (LIGHTSLEEP_MODE << PCS_CTL_PARAM_SHIFT))
#define DEEP_SLEEP_CMD		(SLEEP_CMD | (DEEPSLEEP_MODE << PCS_CTL_PARAM_SHIFT))

#define PCS0_CFG_OFFSET			0x80
#define PCSm_CFG_OFFSET(i)		((i + 3) * 0x20 + PCS0_CFG_OFFSET)
#define PCS_CFG_LIGHT_SLEEP_SHIFT	2
#define PCS_CFG_LIGHT_SLEEP		(1 << PCS_CFG_LIGHT_SLEEP_SHIFT)
#define PCS_CFG_DEEP_SLEEP_SHIFT	3
#define PCS_CFG_DEEP_SLEEP		(1 << PCS_CFG_DEEP_SLEEP_SHIFT)

#define RESET_VEC_LO_OFFSET	0x50
#define RESET_VEC_HI_OFFSET	0x60
#define RESET_VEC_8CORE_OFFSET	0x1a0
#define HARTn_RESET_VEC_LO(n)	(RESET_VEC_LO_OFFSET + \
			        ((n) < 4 ? 0 : RESET_VEC_8CORE_OFFSET) + \
			        ((n) * 0x4))
#define HARTn_RESET_VEC_HI(n)	(RESET_VEC_HI_OFFSET + \
			        ((n) < 4 ? 0 : RESET_VEC_8CORE_OFFSET) + \
			        ((n) * 0x4))

#define PCS_MAX_NR  8
#define FLASH_BASE  0x80000000ULL

/* clang-format on */

struct smu_data {
	unsigned long addr;
};

int smu_set_wakeup_events(struct smu_data *smu, u32 events, u32 hartid);
bool smu_support_sleep_mode(struct smu_data *smu, u32 sleep_mode, u32 hartid);
int smu_set_command(struct smu_data *smu, u32 pcs_ctl, u32 hartid);
int smu_set_reset_vector(struct smu_data *smu, ulong wakeup_addr, u32 hartid);

#endif /* _SYS_ATCSMU_H */
