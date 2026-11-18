/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2026 Qualcomm, Inc
 *
 * Authors:
 *   Pawandeep Oza <pawandeep.oza@oss.qualcomm.com>
 */

#include <sbi_utils/mpxy/fdt_mpxy.h>
#include <sbi_utils/mpxy/fdt_mpxy_bridge.h>
#include <sbi_utils/mpxy/fdt_mpxy_rpmi.h>
#include <sbi_utils/mailbox/rpmi_mailbox.h>

static struct mpxy_rpmi_service_data mm_srvcdata[] = {
	[0] = {
		.id = RPMI_MM_SRV_ENABLE_NOTIFICATION,
		.min_tx_len = sizeof(struct rpmi_enable_notification_req),
		.max_tx_len = sizeof(struct rpmi_enable_notification_req),
		.min_rx_len = sizeof(struct rpmi_enable_notification_resp),
		.max_rx_len = sizeof(struct rpmi_enable_notification_resp),
	},
	[1] = {
		.id = RPMI_MM_SRV_GET_ATTRIBUTES,
		.min_tx_len = 0,
		.max_tx_len = 0,
		.min_rx_len = sizeof(struct rpmi_mm_get_attributes_rsp),
		.max_rx_len = sizeof(struct rpmi_mm_get_attributes_rsp),
	},
	[2] = {
		.id = RPMI_MM_SRV_COMMUNICATE,
		.min_tx_len = sizeof(struct rpmi_mm_communicate_req),
		.max_tx_len = sizeof(struct rpmi_mm_communicate_req),
		.min_rx_len = sizeof(struct rpmi_mm_communicate_rsp),
		.max_rx_len = sizeof(struct rpmi_mm_communicate_rsp),
	},
};

static struct rpmi_mm_shmem_info {
	u64 addr;
	u32 size;
} mm_shmem;

static int mpxy_rpmi_build_reqfwd_resp(struct mpxy_bridge_src_ctx *src,
				       struct mpxy_bridge_dst_ctx *dst,
				       const u8 *data, u32 data_len)
{
	struct rpmi_request_forward_resp *resp;
	u32 hdr_len = sizeof(struct rpmi_message_header);
	u32 avail;
	u32 offset;
	u32 returned;
	int ret = SBI_OK;

	do {
		resp   = (struct rpmi_request_forward_resp *)dst->respbuf;
		offset = dst->start_index;
		avail  = dst->resp_bufsize - sizeof(*resp);

		if (offset > data_len) {
			ret = SBI_EINVAL;
			break;
		}

		if (offset == 0) {
			if (avail <= hdr_len) {
				ret = SBI_EINVAL;
				break;
			}
			avail -= hdr_len;
		}

		returned = data_len - offset;
		if (returned > avail)
			returned = avail;

		resp->status = RPMI_SUCCESS;

		if (offset == 0) {
			sbi_memcpy(resp->data, (u8 *)&src->header, hdr_len);
			sbi_memcpy(resp->data + hdr_len, data, returned);
			resp->returned     = hdr_len + returned;
			resp->remaining    = data_len - returned;
			*dst->resp_len_ptr = sizeof(*resp) + hdr_len + returned;
		} else {
			sbi_memcpy(resp->data, data + offset, returned);
			resp->returned     = returned;
			resp->remaining    = data_len - offset - returned;
			*dst->resp_len_ptr = sizeof(*resp) + returned;
		}
	} while (0);

	if (ret) {
		resp->status = RPMI_ERR_INVALID_PARAM;
		resp->returned = 0;
		resp->remaining = 0;
	}

	return ret;
}

static int mpxy_rpmi_mm_xfer(struct mpxy_bridge_src_ctx *src,
			     struct mpxy_bridge_dst_ctx *dst)
{
	if (!src || !src->msgbuf || !dst || !dst->respbuf || !dst->resp_len_ptr)
		return SBI_EINVAL;

	switch (src->header.service_id) {
		case RPMI_MM_SRV_GET_ATTRIBUTES:
			return mpxy_rpmi_build_reqfwd_resp(src, dst, NULL,
			mm_srvcdata[RPMI_MM_SRV_GET_ATTRIBUTES - 1].min_tx_len);

		case RPMI_MM_SRV_COMMUNICATE:
			return mpxy_rpmi_build_reqfwd_resp(src, dst, (const u8 *)src->msgbuf,
				mm_srvcdata[RPMI_MM_SRV_COMMUNICATE - 1].min_tx_len);

		default:
			return SBI_ENOTSUPP;
	}
}

static int mpxy_rpmi_get_payload(void *msgbuf, u32 msg_len,
				 const u8 **payload, u32 *payload_len)
{
	struct rpmi_message_header *hdr;
	u32 len;

	hdr = (struct rpmi_message_header *)msgbuf;
	len = (u32)le16_to_cpu(hdr->datalen);

	*payload = (const u8 *)msgbuf + sizeof(struct rpmi_message_header);
	*payload_len = len;

	return SBI_OK;
}

static int mpxy_rpmi_copy_payload(struct mpxy_bridge_src_ctx *src,
				   const u8 *payload, u32 payload_len)
{
	sbi_memcpy((u8 *)src->respbuf, payload, payload_len);
	src->resp_length = payload_len;

	return SBI_OK;
}

static int mpxy_rpmi_mm_complete(struct mpxy_bridge_src_ctx *src,
				  void *msgbuf, u32 msg_len)
{
	const u8 *payload;
	u32 payload_len;
	int ret;

	ret = mpxy_rpmi_get_payload(msgbuf, msg_len,
				    &payload, &payload_len);
	if (ret)
		return ret;

	switch (src->header.service_id) {
		case RPMI_MM_SRV_GET_ATTRIBUTES:
			struct rpmi_mm_get_attributes_rsp *attr_rsp;
			attr_rsp = 	(struct rpmi_mm_get_attributes_rsp *)payload;

			mm_shmem.addr = ((u64)le32_to_cpu(attr_rsp->mma.shmem_addr_hi) << 32) |
				(u64)le32_to_cpu(attr_rsp->mma.shmem_addr_lo);
			mm_shmem.size = le32_to_cpu(attr_rsp->mma.shmem_size);
			attr_rsp->status = RPMI_SUCCESS;
			break;

		case RPMI_MM_SRV_COMMUNICATE:
			break;

		default:
			return SBI_ENOTSUPP;
	}

	ret = mpxy_rpmi_copy_payload(src, payload, payload_len);
	if (ret)
		return ret;

	return SBI_OK;
}

static const struct mpxy_bridge_source_ops mm_ops = {
	.xfer    = mpxy_rpmi_mm_xfer,
    .complete  = mpxy_rpmi_mm_complete,
};

static const struct fdt_match mm_match[] = {
	{
		.compatible = "opensbi,rpmi-mpxy-reqfwd-mm",
	},
	{ }
};

static int mpxy_rpmi_mm_probe(const void *fdt, int nodeoff,
		   const struct fdt_match *match)
{
	(void)match;

	return mpxy_bridge_register_source(fdt, nodeoff, &mm_ops, mm_srvcdata,
					array_size(mm_srvcdata), RPMI_SRVGRP_MANAGEMENT_MODE);
}

const struct mpxy_bridge_source_driver fdt_mpxy_rpmi_bridge_mm = {
	.match_table = mm_match,
	.probe = mpxy_rpmi_mm_probe,
};
