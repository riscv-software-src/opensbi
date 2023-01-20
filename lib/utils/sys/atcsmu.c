/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2023 Andes Technology Corporation
 *
 * Authors:
 *   Yu Chien Peter Lin <peterlin@andestech.com>
 */

#include <sbi_utils/sys/atcsmu.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_bitops.h>

inline int smu_set_wakeup_events(struct smu_data *smu, u32 events, u32 hartid)
{
	if (smu) {
		writel(events, (void *)(smu->addr + PCSm_WE_OFFSET(hartid)));
		return 0;
	} else
		return SBI_EINVAL;
}

inline bool smu_support_sleep_mode(struct smu_data *smu, u32 sleep_mode,
				   u32 hartid)
{
	u32 pcs_cfg;

	if (!smu) {
		sbi_printf("%s(): Failed to access smu_data\n", __func__);
		return false;
	}

	pcs_cfg = readl((void *)(smu->addr + PCSm_CFG_OFFSET(hartid)));

	switch (sleep_mode) {
	case LIGHTSLEEP_MODE:
		if (EXTRACT_FIELD(pcs_cfg, PCS_CFG_LIGHT_SLEEP) == 0) {
			sbi_printf(
				"SMU: hart%d (PCS%d) does not support light sleep mode\n",
				hartid, hartid + 3);
			return false;
		}
		break;
	case DEEPSLEEP_MODE:
		if (EXTRACT_FIELD(pcs_cfg, PCS_CFG_DEEP_SLEEP) == 0) {
			sbi_printf(
				"SMU: hart%d (PCS%d) does not support deep sleep mode\n",
				hartid, hartid + 3);
			return false;
		}
		break;
	}

	return true;
}

inline int smu_set_command(struct smu_data *smu, u32 pcs_ctl, u32 hartid)
{
	if (smu) {
		writel(pcs_ctl, (void *)(smu->addr + PCSm_CTL_OFFSET(hartid)));
		return 0;
	} else
		return SBI_EINVAL;
}

inline int smu_set_reset_vector(struct smu_data *smu, ulong wakeup_addr,
				u32 hartid)
{
	u32 vec_lo, vec_hi;
	u64 reset_vector;

	if (!smu)
		return SBI_EINVAL;

	writel(wakeup_addr, (void *)(smu->addr + HARTn_RESET_VEC_LO(hartid)));
	writel((u64)wakeup_addr >> 32,
	       (void *)(smu->addr + HARTn_RESET_VEC_HI(hartid)));

	vec_lo = readl((void *)(smu->addr + HARTn_RESET_VEC_LO(hartid)));
	vec_hi = readl((void *)(smu->addr + HARTn_RESET_VEC_HI(hartid)));
	reset_vector = ((u64)vec_hi << 32) | vec_lo;

	if (reset_vector != (u64)wakeup_addr) {
		sbi_printf(
			"hard%d (PCS%d): Failed to program the reset vector.\n",
			hartid, hartid + 3);
		return SBI_EFAIL;
	} else
		return 0;
}
