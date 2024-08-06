/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Anup Patel <apatel@ventanamicro.com>
 */

#include <sbi/sbi_error.h>
#include <sbi_utils/mailbox/mailbox.h>
#include <sbi_utils/mailbox/rpmi_mailbox.h>

int rpmi_xlate_error(enum rpmi_error error)
{
	switch(error) {
	case RPMI_SUCCESS:
		return SBI_OK;
	case RPMI_ERR_FAILED:
		return SBI_EFAIL;
	case RPMI_ERR_NOTSUPP:
		return SBI_ENOTSUPP;
	case RPMI_ERR_INVALID_PARAM:
		return SBI_EINVAL;
	case RPMI_ERR_DENIED:
		return SBI_EDENIED;
	case RPMI_ERR_INVALID_ADDR:
		return SBI_EINVALID_ADDR;
	case RPMI_ERR_ALREADY:
		return SBI_EALREADY;
	case RPMI_ERR_EXTENSION:
		return SBI_EFAIL;
	case RPMI_ERR_HW_FAULT:
		return SBI_EIO;
	case RPMI_ERR_BUSY:
		return SBI_EFAIL;
	case RPMI_ERR_INVALID_STATE:
		return SBI_EINVALID_STATE;
	case RPMI_ERR_BAD_RANGE:
		return SBI_EBAD_RANGE;
	case RPMI_ERR_TIMEOUT:
		return SBI_ETIMEDOUT;
	case RPMI_ERR_IO:
		return SBI_EIO;
	case RPMI_ERR_NO_DATA:
		return SBI_EFAIL;
	default:
		return SBI_EUNKNOWN;
	}
}

int rpmi_normal_request_with_status(
			struct mbox_chan *chan, u32 service_id,
			void *req, u32 req_words, u32 req_endian_words,
			void *resp, u32 resp_words, u32 resp_endian_words)
{
	int ret;
	struct mbox_xfer xfer;
	struct rpmi_message_args args = { 0 };

	args.type = RPMI_MSG_NORMAL_REQUEST;
	args.service_id = service_id;
	args.tx_endian_words = req_endian_words;
	args.rx_endian_words = resp_endian_words;
	mbox_xfer_init_txrx(&xfer, &args,
			req, sizeof(u32) * req_words, RPMI_DEF_TX_TIMEOUT,
			resp, sizeof(u32) * resp_words, RPMI_DEF_RX_TIMEOUT);

	ret = mbox_chan_xfer(chan, &xfer);
	if (ret)
		return ret;

	return rpmi_xlate_error(((u32 *)resp)[0]);
}

int rpmi_posted_request(
		struct mbox_chan *chan, u32 service_id,
		void *req, u32 req_words, u32 req_endian_words)
{
	struct mbox_xfer xfer;
	struct rpmi_message_args args = { 0 };

	args.type = RPMI_MSG_POSTED_REQUEST;
	args.flags = RPMI_MSG_FLAGS_NO_RX;
	args.service_id = service_id;
	args.tx_endian_words = req_endian_words;
	mbox_xfer_init_tx(&xfer, &args,
			  req, sizeof(u32) * req_words, RPMI_DEF_TX_TIMEOUT);

	return mbox_chan_xfer(chan, &xfer);
}
