/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Rahul Pathak <rpathak@ventanamicro.com>
 *   Anup Patel <apatel@ventanamicro.com>
 */

#include <libfdt.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_heap.h>
#include <sbi/sbi_mpxy.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/mpxy/fdt_mpxy.h>
#include <sbi_utils/mailbox/fdt_mailbox.h>
#include <sbi_utils/mailbox/rpmi_msgprot.h>
#include <sbi/sbi_console.h>

#define RPMI_MAJOR_VER		(1)
#define RPMI_MINOR_VER		(0)

/** Convert the mpxy attribute ID to attribute array index */
#define attr_id2index(attr_id)	(attr_id - SBI_MPXY_ATTR_MSGPROTO_ATTR_START)

enum mpxy_msgprot_rpmi_attr_id {
	MPXY_MSGPROT_RPMI_ATTR_SERVICEGROUP_ID = SBI_MPXY_ATTR_MSGPROTO_ATTR_START,
	MPXY_MSGPROT_RPMI_ATTR_SERVICEGROUP_VERSION,
	MPXY_MSGPROT_RPMI_ATTR_MAX_ID,
};

/**
 * MPXY message protocol attributes for RPMI
 * Order of attribute fields must follow the
 * attribute IDs in `enum mpxy_msgprot_rpmi_attr_id`
 */
struct mpxy_rpmi_channel_attrs {
	u32 servicegrp_id;
	u32 servicegrp_ver;
};

/* RPMI mbox data per service group */
struct mpxy_mbox_data {
	u32 servicegrp_id;
	u32 num_services;
	u32 notifications_support;
	void *priv_data;
};

/* RPMI service data per service group */
struct rpmi_service_data {
	u8 id;
	u32 min_tx_len;
	u32 max_tx_len;
	u32 min_rx_len;
	u32 max_rx_len;
};

/**
 * MPXY mbox instance per MPXY channel. This ties
 * an MPXY channel with an RPMI Service group.
 */
struct mpxy_mbox {
	struct mbox_chan *chan;
	struct mpxy_mbox_data *mbox_data;
	struct mpxy_rpmi_channel_attrs msgprot_attrs;
	struct sbi_mpxy_channel channel;
};

/** Make sure all attributes are packed for direct memcpy */
#define assert_field_offset(field, attr_offset)				\
	_Static_assert(							\
		((offsetof(struct mpxy_rpmi_channel_attrs, field)) /	\
		 sizeof(u32)) == (attr_offset - SBI_MPXY_ATTR_MSGPROTO_ATTR_START),\
		"field " #field						\
		" from struct mpxy_rpmi_channel_attrs invalid offset, expected " #attr_offset)

assert_field_offset(servicegrp_id, MPXY_MSGPROT_RPMI_ATTR_SERVICEGROUP_ID);

/**
 * Discover the RPMI service data using message_id
 * MPXY message_id == RPMI service_id
 */
static struct rpmi_service_data *mpxy_find_rpmi_srvid(u32 message_id,
					struct mpxy_mbox_data *mbox_data)
{
	int mid = 0;
	struct rpmi_service_data *srv = mbox_data->priv_data;
	for (mid = 0; srv[mid].id < mbox_data->num_services; mid++) {
		if (srv[mid].id == (u8)message_id)
			return &srv[mid];
	}

	return NULL;
}

/** Copy attributes word size */
static void mpxy_copy_attrs(u32 *outmem, u32 *inmem, u32 count)
{
	u32 idx;
	for (idx = 0; idx < count; idx++)
		outmem[idx] = cpu_to_le32(inmem[idx]);
}

static int mpxy_mbox_read_attributes(struct sbi_mpxy_channel *channel,
				     u32 *outmem, u32 base_attr_id,
				     u32 attr_count)
{
	u32 end_id;
	struct mpxy_mbox *rmb =
		container_of(channel, struct mpxy_mbox, channel);

	u32 *attr_array = (u32 *)&rmb->msgprot_attrs;

	end_id = base_attr_id + attr_count - 1;

	if (end_id >= MPXY_MSGPROT_RPMI_ATTR_MAX_ID)
		return SBI_EBAD_RANGE;

	mpxy_copy_attrs(outmem, &attr_array[attr_id2index(base_attr_id)],
			attr_count);

	return SBI_OK;
}

/**
 * Verify the channel standard attribute wrt to write permission
 * and the value to be set if valid or not.
 * Only attributes needs to be checked which are defined Read/Write
 * permission. Other with Readonly permission will result in error.
 *
 * Attributes values to be written must also be checked because
 * before writing a range of attributes, we need to make sure that
 * either complete range of attributes is written successfully or not
 * at all.
 */
static int mpxy_check_write_attr(u32 attr_id, u32 attr_val)
{
	int ret = SBI_OK;

	switch(attr_id) {
	/** All RO access attributes falls under default */
	default:
		ret = SBI_EBAD_RANGE;
	};

	return ret;
}

static void mpxy_write_attr(struct mpxy_rpmi_channel_attrs *attrs,
			   u32 attr_id,
			   u32 attr_val)
{
	/* No writable attributes in RPMI */
}

static int mpxy_mbox_write_attributes(struct sbi_mpxy_channel *channel,
				     u32 *outmem, u32 base_attr_id,
				     u32 attr_count)
{
	int ret, mem_idx;
	u32 end_id, attr_val, idx;
	struct mpxy_mbox *rmb =
		container_of(channel, struct mpxy_mbox, channel);

	end_id = base_attr_id + attr_count - 1;

	if (end_id >= MPXY_MSGPROT_RPMI_ATTR_MAX_ID)
		return SBI_EBAD_RANGE;

	mem_idx = 0;
	for (idx = base_attr_id; idx <= end_id; idx++) {
		attr_val = le32_to_cpu(outmem[mem_idx++]);
		ret = mpxy_check_write_attr(idx, attr_val);
		if (ret)
			return ret;
	}

	mem_idx = 0;
	for (idx = base_attr_id; idx <= end_id; idx++) {
		attr_val = le32_to_cpu(outmem[mem_idx++]);
		mpxy_write_attr(&rmb->msgprot_attrs, idx, attr_val);
	}

	return SBI_OK;
}

static int __mpxy_mbox_send_message(struct sbi_mpxy_channel *channel,
				  u32 message_id, void *tx, u32 tx_len,
				  void *rx, u32 rx_max_len,
				  unsigned long *ack_len)
{
	int ret;
	u32 rx_len = 0;
	struct mbox_xfer xfer;
	struct rpmi_message_args args = {0};
	struct mpxy_mbox *rmb =
		container_of(channel, struct mpxy_mbox, channel);
	struct rpmi_service_data *srv =
			mpxy_find_rpmi_srvid(message_id, rmb->mbox_data);
	if (!srv)
		return SBI_ENOTSUPP;

	/** message data size to be sent is in the
	 * supported service data size range */
	if (tx_len < srv->min_tx_len || tx_len > srv->max_tx_len)
		return SBI_EFAIL;

	if (ack_len) {
		/** MPXY buffer cannnot hold service min response data */
		if (rx_max_len < srv->min_rx_len)
			return SBI_EFAIL;

		/* Max rx data size it can expect */
		if (srv->max_rx_len < channel->attrs.msg_data_maxlen)
			rx_len = srv->max_rx_len;
		else
			rx_len = channel->attrs.msg_data_maxlen;

		args.type = RPMI_MSG_NORMAL_REQUEST;
		args.flags = (rx) ? 0 : RPMI_MSG_FLAGS_NO_RX;
		args.service_id = srv->id;
		mbox_xfer_init_txrx(&xfer, &args,
				    tx, tx_len, RPMI_DEF_TX_TIMEOUT,
				    rx, rx_len, RPMI_DEF_RX_TIMEOUT);
	}
	else {
		args.type = RPMI_MSG_POSTED_REQUEST;
		args.flags = RPMI_MSG_FLAGS_NO_RX;
		args.service_id = srv->id;
		mbox_xfer_init_tx(&xfer, &args,
				  tx, tx_len, RPMI_DEF_TX_TIMEOUT);
	}

	ret = mbox_chan_xfer(rmb->chan, &xfer);
	if (ret)
		return ret;

	if (ack_len)
		*ack_len = args.rx_data_len;

	return SBI_OK;
}

static int mpxy_mbox_send_message_withresp(struct sbi_mpxy_channel *channel,
				  u32 message_id, void *tx, u32 tx_len,
				  void *rx, u32 rx_max_len,
				  unsigned long *ack_len)
{
	return __mpxy_mbox_send_message(channel, message_id, tx, tx_len,
				 rx, rx_max_len, ack_len);
}

static int mpxy_mbox_send_message_withoutresp(struct sbi_mpxy_channel *channel,
				  u32 message_id, void *tx, u32 tx_len)
{
	return __mpxy_mbox_send_message(channel, message_id, tx, tx_len,
				 NULL, 0, NULL);
}

static int mpxy_mbox_get_notifications(struct sbi_mpxy_channel *channel,
				       void *eventsbuf, u32 bufsize,
				       unsigned long *events_len)
{
	return SBI_ENOTSUPP;
}

static int mpxy_mbox_init(const void *fdt, int nodeoff,
			  const struct fdt_match *match)
{
	int rc, len;
	const fdt32_t *val;
	u32 channel_id;
	struct mpxy_mbox *rmb;
	struct mbox_chan *chan;
	const struct mpxy_mbox_data *data = match->data;

	/* Allocate context for RPXY mbox client */
	rmb = sbi_zalloc(sizeof(*rmb));
	if (!rmb)
		return SBI_ENOMEM;

	/*
	 * If channel request failed then other end does not support
	 * service group so do nothing.
	 */
	rc = fdt_mailbox_request_chan(fdt, nodeoff, 0, &chan);
	if (rc) {
		sbi_free(rmb);
		return SBI_ENODEV;
	}

	/* Match channel service group id */
	if (data->servicegrp_id != chan->chan_args[0]) {
		mbox_controller_free_chan(chan);
		sbi_free(rmb);
		return SBI_EINVAL;
	}

	/*
	 * The "riscv,sbi-mpxy-channel-id" DT property is mandatory
	 * for MPXY RPMI mailbox client driver so if this is not
	 * present then try other drivers.
	 */
	val = fdt_getprop(fdt, nodeoff, "riscv,sbi-mpxy-channel-id", &len);
	if (len > 0 && val)
		channel_id = fdt32_to_cpu(*val);
	else {
		mbox_controller_free_chan(chan);
		sbi_free(rmb);
		return SBI_ENODEV;
	}

	/* Setup MPXY mbox client */
	/* Channel ID*/
	rmb->channel.channel_id = channel_id;
	/* Callback for read RPMI attributes */
	rmb->channel.read_attributes = mpxy_mbox_read_attributes;
	/* Callback for write RPMI attributes */
	rmb->channel.write_attributes = mpxy_mbox_write_attributes;
	/* Callback for sending RPMI message */
	rmb->channel.send_message_with_response =
					mpxy_mbox_send_message_withresp;
	rmb->channel.send_message_without_response =
					mpxy_mbox_send_message_withoutresp;
	/* Callback to get RPMI notifications */
	rmb->channel.get_notification_events = mpxy_mbox_get_notifications;

	/* No callback to switch events state data */
	rmb->channel.switch_eventsstate = NULL;

	/* RPMI Message Protocol ID */
	rmb->channel.attrs.msg_proto_id = SBI_MPXY_MSGPROTO_RPMI_ID;
	/* RPMI Message Protocol Version */
	rmb->channel.attrs.msg_proto_version =
		SBI_MPXY_MSGPROTO_VERSION(RPMI_MAJOR_VER, RPMI_MINOR_VER);

	/* RPMI supported max message data length(bytes), same for
	 * all service groups */
	rmb->channel.attrs.msg_data_maxlen =
					RPMI_MSG_DATA_SIZE(RPMI_SLOT_SIZE_MIN);
	/* RPMI message send timeout(milliseconds)
	 * same for all service groups */
	rmb->channel.attrs.msg_send_timeout = RPMI_DEF_TX_TIMEOUT;
	rmb->channel.attrs.msg_completion_timeout =
				RPMI_DEF_TX_TIMEOUT + RPMI_DEF_RX_TIMEOUT;

	/* RPMI message protocol attributes */
	rmb->msgprot_attrs.servicegrp_id = data->servicegrp_id;
	rmb->msgprot_attrs.servicegrp_ver =
			SBI_MPXY_MSGPROTO_VERSION(RPMI_MAJOR_VER, RPMI_MINOR_VER);

	rmb->mbox_data = (struct mpxy_mbox_data *)data;
	rmb->chan = chan;

	/* Register RPXY service group */
	rc = sbi_mpxy_register_channel(&rmb->channel);
	if (rc) {
		mbox_controller_free_chan(chan);
		sbi_free(rmb);
		return rc;
	}

	return SBI_OK;
}

static struct rpmi_service_data clock_services[] = {
{
	.id = RPMI_CLOCK_SRV_ENABLE_NOTIFICATION,
	.min_tx_len = sizeof(struct rpmi_enable_notification_req),
	.max_tx_len = sizeof(struct rpmi_enable_notification_req),
	.min_rx_len = sizeof(struct rpmi_enable_notification_resp),
	.max_rx_len = sizeof(struct rpmi_enable_notification_resp),
},
{
	.id = RPMI_CLOCK_SRV_GET_NUM_CLOCKS,
	.min_tx_len = 0,
	.max_tx_len = 0,
	.min_rx_len = sizeof(struct rpmi_clock_get_num_clocks_resp),
	.max_rx_len = sizeof(struct rpmi_clock_get_num_clocks_resp),
},
{
	.id = RPMI_CLOCK_SRV_GET_ATTRIBUTES,
	.min_tx_len = sizeof(struct rpmi_clock_get_attributes_req),
	.max_tx_len = sizeof(struct rpmi_clock_get_attributes_req),
	.min_rx_len = sizeof(struct rpmi_clock_get_attributes_resp),
	.max_rx_len = sizeof(struct rpmi_clock_get_attributes_resp),
},
{
	.id = RPMI_CLOCK_SRV_GET_SUPPORTED_RATES,
	.min_tx_len = sizeof(struct rpmi_clock_get_supported_rates_req),
	.max_tx_len = sizeof(struct rpmi_clock_get_supported_rates_req),
	.min_rx_len = sizeof(struct rpmi_clock_get_supported_rates_resp),
	.max_rx_len = -1U,
},
{
	.id = RPMI_CLOCK_SRV_SET_CONFIG,
	.min_tx_len = sizeof(struct rpmi_clock_set_config_req),
	.max_tx_len = sizeof(struct rpmi_clock_set_config_req),
	.min_rx_len = sizeof(struct rpmi_clock_set_config_resp),
	.max_rx_len = sizeof(struct rpmi_clock_set_config_resp),
},
{
	.id = RPMI_CLOCK_SRV_GET_CONFIG,
	.min_tx_len = sizeof(struct rpmi_clock_get_config_req),
	.max_tx_len = sizeof(struct rpmi_clock_get_config_req),
	.min_rx_len = sizeof(struct rpmi_clock_get_config_resp),
	.max_rx_len = sizeof(struct rpmi_clock_get_config_resp),
},
{
	.id = RPMI_CLOCK_SRV_SET_RATE,
	.min_tx_len = sizeof(struct rpmi_clock_set_rate_req),
	.max_tx_len = sizeof(struct rpmi_clock_set_rate_req),
	.min_rx_len = sizeof(struct rpmi_clock_set_rate_resp),
	.max_rx_len = sizeof(struct rpmi_clock_set_rate_resp),
},
{
	.id = RPMI_CLOCK_SRV_GET_RATE,
	.min_tx_len = sizeof(struct rpmi_clock_get_rate_req),
	.max_tx_len = sizeof(struct rpmi_clock_get_rate_req),
	.min_rx_len = sizeof(struct rpmi_clock_get_rate_resp),
	.max_rx_len = sizeof(struct rpmi_clock_get_rate_resp),
},
};

static struct mpxy_mbox_data clock_data = {
	.servicegrp_id = RPMI_SRVGRP_CLOCK,
	.num_services = RPMI_CLOCK_SRV_MAX_COUNT,
	.notifications_support = 1,
	.priv_data = clock_services,
};

static const struct fdt_match mpxy_mbox_match[] = {
	{ .compatible = "riscv,rpmi-mpxy-clock", .data = &clock_data },
	{ },
};

struct fdt_driver fdt_mpxy_rpmi_mbox = {
	.match_table = mpxy_mbox_match,
	.init = mpxy_mbox_init,
	.experimental = true,
};
