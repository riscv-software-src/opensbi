// SPDX-License-Identifier: GPL-2.0-only OR BSD-2-Clause
/*
 * Copyright (c) 2024 Beijing ESWIN Computing Technology Co., Ltd.
 *
 * Authors:
 *   Lin Min <linmin@eswincomputing.com>
 *   Bo Gan <ganboing@gmail.com>
 *
 * Adapted from Core/Inc/hf_common.h from ESWIN's Hifive Premier P550
 * onboard BMC (MCU) source repo:
 *   https://github.com/eswincomputing/hifive-premier-p550-mcu-patches.git
 *
 */

#ifndef __ESWIN_HFP_H__
#define __ESWIN_HFP_H__

#include <sbi/sbi_types.h>

enum hfp_bmc_msg {
	HFP_MSG_REQUEST = 1,
	HFP_MSG_REPLY,
	HFP_MSG_NOTIFY,
};

enum hfp_bmc_cmd {
	HFP_CMD_POWER_OFF = 1,
	HFP_CMD_REBOOT,
	HFP_CMD_READ_BOARD_INFO,
	HFP_CMD_CONTROL_LED,
	HFP_CMD_PVT_INFO,
	HFP_CMD_BOARD_STATUS,
	HFP_CMD_POWER_INFO,
	HFP_CMD_RESTART, // cold reboot with power off/on
};

#define MAGIC_HEADER	0xA55AAA55
#define MAGIC_TAIL	0xBDBABDBA

struct hfp_bmc_message {
	uint32_t header_magic;
	uint32_t task_id;
	uint8_t type;
	uint8_t cmd;
	uint8_t result;
	uint8_t data_len;
	uint8_t data[250];
	uint8_t checksum;
	uint32_t tail_magic;
} __packed;

static inline void hfp_bmc_checksum_msg(struct hfp_bmc_message *msg)
{
	msg->checksum = 0;
	msg->checksum ^= msg->type;
	msg->checksum ^= msg->cmd;
	msg->checksum ^= msg->data_len;
	for (uint8_t i = 0; i != msg->data_len; i++)
		msg->checksum ^= msg->data[i];
}

extern const struct eic770x_board_override hfp_override;

#endif
