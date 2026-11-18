/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2026 Qualcomm, Inc
 *
 * Authors:
 *   Pawandeep Oza <pawandeep.oza@oss.qualcomm.com>
 */

#include <libfdt.h>
#include <sbi/riscv_asm.h>
#include <sbi/sbi_byteorder.h>
#include <sbi/sbi_domain_context.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_heap.h>
#include <sbi/sbi_list.h>
#include <sbi/sbi_mpxy.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_scratch.h>
#include <sbi_utils/fdt/fdt_driver.h>
#include <sbi_utils/mailbox/rpmi_msgprot.h>
#include <sbi_utils/mailbox/rpmi_mailbox.h>
#include <sbi_utils/mailbox/mailbox.h>
#include <sbi_utils/mpxy/fdt_mpxy_bridge.h>
#include <sbi_utils/mpxy/mpxy_rpmi_bridge_src.h>

#define SBI_MPXY_BRIDGE_COMPATIBLE  "opensbi,rpmi-mpxy-reqfwd-bridge"
#define SBI_MPXY_BRIDGE_PROP_DOMAIN_NAME "riscv,domain"
#define SBI_MPXY_BRIDGE_PROP_CHAN "riscv,sbi-mpxy-channel-id"

extern struct sbi_rpmi_bridge_src *const fdt_mpxy_rpmi_bridge_sources[];
static uint16_t token = 0;

struct mpxy_bridge_hart_slot {
	int                        active_source;
	struct mpxy_bridge_src_ctx *src_ctx;
	struct mpxy_bridge_dst_ctx  dst_resp;
};

struct mpxy_bridge_slot_store {
	struct mpxy_bridge_hart_slot *slots;
	u32 hart_count;
};

struct bridge_rpmi_endpoint {
	struct mpxy_rpmi_data rpmi_data;
	struct mpxy_bridge *bridge;
	struct sbi_domain *domain;
	struct sbi_mpxy_channel *channel;
};

struct mpxy_bridge_endpoint {
	char domain_name[SBI_MPXY_BRIDGE_DOMAIN_NAME_MAXLEN];
	u32  channel_id;
	struct sbi_domain *dom;
	struct sbi_mpxy_channel ch;
	struct bridge_rpmi_endpoint rpmi_endpoint;
};

struct mpxy_bridge_source {
	int nodeoff;
	struct mpxy_bridge_endpoint ep;
	const struct mpxy_bridge_source_ops *ops;
};

struct mpxy_bridge_destination {
	struct mpxy_bridge_endpoint ep;
};

struct mpxy_bridge {
	struct sbi_dlist node;
	int nodeoff;
	struct mpxy_bridge_destination dst;
	struct mpxy_bridge_source *sources;
	u32 source_count;
	struct mpxy_bridge_slot_store store;
};

static SBI_LIST_HEAD(mpxy_bridge_list);
static bool mpxy_bridge_notifier_registered;

static bool bridge_name_match(const char *a, const char *b)
{
	return a && b && !sbi_strcmp(a, b);
}

static int bridge_source_index(struct mpxy_bridge *br,
			       struct sbi_mpxy_channel *channel)
{
	u32 i;

	for (i = 0; i < br->source_count; i++)
		if (&br->sources[i].ep.ch == channel)
			return (int)i;

	return -1;
}

static struct mpxy_bridge *bridge_from_channel(
					struct sbi_mpxy_channel *channel)
{
	struct mpxy_bridge *br;
	u32 i;

	sbi_list_for_each_entry(br, &mpxy_bridge_list, node) {
		if (&br->dst.ep.ch == channel)
			return br;
		for (i = 0; i < br->source_count; i++)
			if (&br->sources[i].ep.ch == channel)
				return br;
	}

	return NULL;
}

static int bridge_get_current_slot(struct mpxy_bridge *br,
				   struct mpxy_bridge_hart_slot **slot)
{
	u32 hartindex = current_hartindex();

	if (hartindex >= br->store.hart_count)
		return SBI_ERR_INVALID_PARAM;

	*slot = &br->store.slots[hartindex];
	return SBI_OK;
}

static void bridge_src_ctx_set(struct mpxy_bridge_src_ctx *src,
			       void *msgbuf, u32 msg_len,
			       void *respbuf, u32 resp_bufsize,
			       unsigned long *resp_len)
{
	src->msgbuf       = msgbuf;
	src->msg_len      = msg_len;
	src->respbuf      = respbuf;
	src->resp_bufsize = resp_bufsize;
}

static void bridge_src_ctx_release(struct mpxy_bridge_src_ctx *src)
{
	if (src)
		*src = (struct mpxy_bridge_src_ctx){ 0 };
}

static void bridge_slot_release(struct mpxy_bridge *br,
				struct mpxy_bridge_hart_slot *slot)
{
	u32 i;

	if (!br || !slot)
		return;

	for (i = 0; i < br->source_count; i++)
		bridge_src_ctx_release(&slot->src_ctx[i]);

	slot->active_source = -1;
	slot->dst_resp = (struct mpxy_bridge_dst_ctx){ 0 };
}

static int bridge_find_pending_source(struct mpxy_bridge *br,
				      struct mpxy_bridge_hart_slot *slot)
{
	u32 i;

	for (i = 0; i < br->source_count; i++)
		if (slot->src_ctx[i].occupied && !slot->src_ctx[i].retrieved)
			return (int)i;

	return -1;
}

static int bridge_find_completed_source(struct mpxy_bridge *br,
				      struct mpxy_bridge_hart_slot *slot)
{
	u32 i;

	for (i = 0; i < br->source_count; i++)
		if (slot->src_ctx[i].occupied &&
			slot->src_ctx[i].retrieved &&
			slot->src_ctx[i].completed )
			return (int)i;

	return -1;
}

struct mpxy_bridge *bridge_find_by_nodeoff(int nodeoff)
{
	struct mpxy_bridge *br;

	sbi_list_for_each_entry(br, &mpxy_bridge_list, node) {
		if (br->nodeoff == nodeoff)
			return br;
	}

	return NULL;
}

static void bridge_switch_domain(struct mpxy_bridge *br,
				 struct sbi_mpxy_channel *channel)
{
	if (channel == &br->dst.ep.ch)
		sbi_domain_context_exit();
	else
		sbi_domain_context_enter(br->dst.ep.ch.owner_domain);
}

static void bridge_save_dst_ctx(struct mpxy_bridge_hart_slot *slot,
				void *respbuf, u32 resp_bufsize,
				unsigned long *resp_len, void *msgbuf)
{
	slot->dst_resp.respbuf      = respbuf;
	slot->dst_resp.resp_bufsize = resp_bufsize;
	slot->dst_resp.resp_len_ptr = resp_len;
	slot->dst_resp.start_index  = msgbuf ? ((u32 *)msgbuf)[0] : 0;
}

static int bridge_dst_complete(struct mpxy_bridge *br,
			       struct mpxy_bridge_hart_slot *slot,
			       void *msgbuf, u32 msg_len, void *respbuf,
			       u32 resp_max_len, unsigned long *resp_len)
{
	struct rpmi_request_complete_resp *resp;
	struct mpxy_bridge_src_ctx *src = NULL;
	struct mpxy_bridge_source *source;
	int active_source;
	int ret = SBI_OK;

	resp = (struct rpmi_request_complete_resp *)respbuf;
	active_source = slot->active_source;

	do {
		if (active_source < 0) {
			resp->status = RPMI_ERR_NO_DATA;
			break;
		}

		src = &slot->src_ctx[active_source];

		if (!src->occupied || !src->retrieved) {
			resp->status = RPMI_ERR_NO_DATA;
			break;
		}

		source = &br->sources[active_source];
		ret = source->ops->complete(src, msgbuf, msg_len);
		src->completed = true;
		if (ret)
			resp->status   = RPMI_ERR_NOTSUPP;
		else
			resp->status   = RPMI_SUCCESS;
	} while (0);

	/*
	 * Number of forwarded messages available for retrieval
	 * this is always 0 as source is stalled on the same hart till the complete
	 * completes.
	 */
	resp->num_of_messages = 0;
	*resp_len = sizeof(*resp);

	return SBI_OK;
}

static int bridge_dst_send_message_with_response(
				struct mpxy_bridge *br,
				struct mpxy_bridge_hart_slot *slot,
				u32 msg_id, void *msgbuf, u32 msg_len,
				void *respbuf, u32 resp_bufsize,
				unsigned long *resp_len)
{
	struct mpxy_bridge_source *source;
	struct mpxy_bridge_src_ctx *src;
	int source_index;
	int ret = SBI_OK;

	switch (msg_id) {
		case REQFWD_RETRIEVE_CURRENT_MESSAGE:
			source_index = bridge_find_pending_source(br, slot);
			bridge_save_dst_ctx(slot, respbuf, resp_bufsize, resp_len, msgbuf);
			/*
			 * Waiting for the message from one of the sources.
			 * schedule the next domain.
			 */
			if (source_index < 0) {
					source_index = bridge_find_completed_source(br, slot);
					if (source_index >= 0) {
						/* it is time to send reponse back to the source domain. */
						source = &br->sources[source_index];
						src = &slot->src_ctx[source_index];
						slot->active_source = -1;
						*resp_len = src->resp_length;
						bridge_src_ctx_release(src);
					}
				bridge_switch_domain(br, &br->dst.ep.ch);
			} else {
				source = &br->sources[source_index];
				src = &slot->src_ctx[source_index];
				/* hand over the transfer to the source driver. */
				ret = source->ops->xfer(src, &slot->dst_resp);
				src->retrieved = true;
			}
			break;

		case REQFWD_COMPLETE_CURRENT_MESSAGE:
			ret = bridge_dst_complete(br, slot, msgbuf, msg_len,
						respbuf, resp_bufsize, resp_len);
			break;

		default:
			ret = SBI_ERR_NOT_SUPPORTED;
			break;
	}

	return ret;
}


static int bridge_src_ctx_occupy(struct mpxy_bridge_src_ctx *src)
{
	if (src->occupied) {
		sbi_printf("%s: slot occupied\n", __func__);
		return SBI_ERR_FAILED;
	}
	src->occupied = true;
	src->retrieved = false;

	return SBI_OK;
}

static int bridge_send_message_with_response(struct sbi_mpxy_channel *channel,
					     u32 msg_id, void *msgbuf, u32 msg_len,
					     void *respbuf, u32 resp_bufsize,
					     unsigned long *resp_len)
{
	struct mpxy_bridge *br;
	struct mpxy_bridge_src_ctx *src;
	struct mpxy_bridge_source *source;
	struct mpxy_bridge_hart_slot *slot = NULL;
	int source_index;
	int ret = SBI_OK;

	br = bridge_from_channel(channel);
	if (!br) {
		ret = SBI_ERR_NOT_SUPPORTED;
	} else {
		ret = bridge_get_current_slot(br, &slot);
		if (!ret) {
			if (channel == &br->dst.ep.ch) {
				ret = bridge_dst_send_message_with_response(
					br, slot, msg_id, msgbuf, msg_len,
					respbuf, resp_bufsize, resp_len);
			} else {
				source_index = bridge_source_index(br, channel);
				if (source_index < 0) {
					ret = SBI_ERR_NOT_SUPPORTED;
				} else {
					src = &slot->src_ctx[source_index];
					source = &br->sources[source_index];

					src->header.servicegroup_id = cpu_to_le16(source->ep.rpmi_endpoint
							    .rpmi_data.servicegrp_id);
					src->header.service_id = msg_id;
					src->header.flags = 0;
					src->header.datalen =
						cpu_to_le16(sizeof(src->header)) +
						msg_len;
					src->header.token = cpu_to_le16(++token);

					bridge_src_ctx_set(src, msgbuf, msg_len,
							   respbuf, resp_bufsize,
							   resp_len);

					ret = bridge_src_ctx_occupy(src);
					slot->active_source = source_index;

					if (!ret) {
						if (slot->dst_resp.respbuf) {
							ret = source->ops->xfer(
								src, &slot->dst_resp);
							src->retrieved = true;
						}

						bridge_switch_domain(
							br,
							&br->sources[source_index].ep.ch);
					}
				}
			}
		}
	}

	return ret;
}

static int bridge_rpmi_get_attribute_group(void *context,
					   struct mbox_chan *chan,
					   enum rpmi_channel_attribute_id attr_id,
					   u32 *out_value)
{
	struct bridge_rpmi_endpoint *ep = context;
	(void)chan;

	switch (attr_id) {
		case RPMI_CHANNEL_ATTR_PROTOCOL_VERSION:
			*((u32 *)out_value) = 1;
			break;
		case RPMI_CHANNEL_ATTR_MAX_DATA_LEN:
			*((u32 *)out_value) = SBI_MPXY_BRIDGE_DEFAULT_MSG_DATA_MAXLEN;
			break;
		case RPMI_CHANNEL_ATTR_TX_TIMEOUT:
			*((u32 *)out_value) = RPMI_DEF_TX_TIMEOUT;
			break;
		case RPMI_CHANNEL_ATTR_RX_TIMEOUT:
			*((u32 *)out_value) = RPMI_DEF_RX_TIMEOUT;
			break;
		case RPMI_CHANNEL_ATTR_SERVICEGROUP_ID:
			*((u32 *)out_value) = ep->rpmi_data.servicegrp_id;
			break;
		default:
			*((u32 *)out_value) = 0;
			break;
	}

	return SBI_OK;
}

static int bridge_rpmi_xfer_group(void *context,
				  struct mbox_chan *chan,
				  struct mbox_xfer *xfer)
{
	struct bridge_rpmi_endpoint *ep = context;
	struct rpmi_message_args *args;
	unsigned long ack_len = 0;
	int ret;

	(void)chan;

	args = xfer->args;

	/*
	 * we only support send message with response.
	 * TDO: bridge_send_message_without_response
	 */
	if (! (args->flags & RPMI_MSG_FLAGS_NO_RX)) {
		ret = bridge_send_message_with_response(
				ep->channel,
				args->service_id,
				xfer->tx, xfer->tx_len,
				xfer->rx, xfer->rx_len,
				&ack_len);

		if (ret)
			return ret;
	}

	args->rx_data_len = ack_len;
	xfer->rx_len      = ack_len;

	return SBI_OK;
}

static int bridge_rpmi_setup_group(void **context,
				   struct mbox_chan *chan,
				   struct sbi_domain *dom,
				   const struct mpxy_rpmi_data *data)
{
	struct bridge_rpmi_endpoint *ep;
	int ret = SBI_OK;

	(void)chan;

	if (!context || !dom || !data)
		return SBI_EINVAL;

	ep = container_of(data, struct bridge_rpmi_endpoint, rpmi_data);

	ep->domain = dom;
	*context = ep;
	return ret;
}

static struct mpxy_rpmi_service_data bridge_dst_rpmi_services[] = {
	{
		.id          = REQFWD_RETRIEVE_CURRENT_MESSAGE,
		.min_tx_len  = 0,
		.max_tx_len  = SBI_MPXY_BRIDGE_DEFAULT_MSG_DATA_MAXLEN,
		.min_rx_len  = sizeof(struct rpmi_request_forward_resp),
		.max_rx_len  = SBI_MPXY_BRIDGE_DEFAULT_MSG_DATA_MAXLEN,
	},
	{
		.id          = REQFWD_COMPLETE_CURRENT_MESSAGE,
		.min_tx_len  = 0,
		.max_tx_len  = SBI_MPXY_BRIDGE_DEFAULT_MSG_DATA_MAXLEN,
		.min_rx_len  = sizeof(struct rpmi_request_complete_resp),
		.max_rx_len  = sizeof(struct rpmi_request_complete_resp),
	},
};

static const struct mpxy_rpmi_data bridge_dst_rpmi_data_template = {
	.servicegrp_id    = RPMI_SRVGRP_REQFWD,
	.num_services     = sizeof(bridge_dst_rpmi_services) /
			    sizeof(bridge_dst_rpmi_services[0]),
	.service_data     = bridge_dst_rpmi_services,
	.get_attribute_group = bridge_rpmi_get_attribute_group,
	.xfer_group       = bridge_rpmi_xfer_group,
	.setup_group      = bridge_rpmi_setup_group,
};

static const struct mpxy_rpmi_data bridge_src_rpmi_data_template = {
	.get_attribute_group = bridge_rpmi_get_attribute_group,
	.xfer_group       = bridge_rpmi_xfer_group,
	.setup_group      = bridge_rpmi_setup_group,
};

static void bridge_init_internal_channel(struct sbi_mpxy_channel *ch,
					 struct sbi_domain *dom, u32 id)
{
	sbi_memset(ch, 0, sizeof(*ch));
	ch->owner_domain            = dom;
	ch->channel_id              = id;
	ch->attrs.msg_data_maxlen   = SBI_MPXY_BRIDGE_DEFAULT_MSG_DATA_MAXLEN;
}

static int bridge_register_destination(struct mpxy_bridge *br,
					struct sbi_domain *dom)
{
	struct bridge_rpmi_endpoint *ep;
	int ret;

	ep             = &br->dst.ep.rpmi_endpoint;
	ep->rpmi_data  = bridge_dst_rpmi_data_template;
	ep->bridge     = br;
	ep->domain     = dom;

	bridge_init_internal_channel(&br->dst.ep.ch, dom,
				     br->dst.ep.channel_id);
	ep->channel = &br->dst.ep.ch;

	ret = mpxy_rpmi_no_mbox_init(&ep->rpmi_data,
				     br->dst.ep.channel_id,
				     br->dst.ep.domain_name);

	if (!ret)
		br->dst.ep.dom = dom;

	return ret;
}

int mpxy_bridge_register_source(const void *fdt,
				int nodeoff,
				const struct mpxy_bridge_source_ops *ops,
				struct mpxy_rpmi_service_data *srvcdata,
				u32 num_services,
				enum rpmi_servicegroup_id srvgrpid)
{
	struct mpxy_bridge *br;
	struct mpxy_bridge_source *src = NULL;
	struct bridge_rpmi_endpoint *ep;
	int parent_nodeoff;
	u32 i;

	if (!fdt || nodeoff < 0 || !ops)
		return SBI_EINVAL;

	parent_nodeoff = fdt_parent_offset(fdt, nodeoff);
	if (parent_nodeoff < 0)
		return SBI_ENODEV;

	br = bridge_find_by_nodeoff(parent_nodeoff);
	if (!br)
		return SBI_ENODEV;

	for (i = 0; i < br->source_count; i++) {
		if (br->sources[i].nodeoff == nodeoff) {
			src = &br->sources[i];
			break;
		}
	}

	if (!src)
		return SBI_ENODEV;

	src->ops = ops;
	if (!src->ops->xfer || !src->ops->complete) {
		sbi_printf("source driver must provide xfer and complete\n");
		src->ep.dom = NULL;
		return SBI_ENOTSUPP;
	}

	ep = &src->ep.rpmi_endpoint;
	ep->bridge = br;
	ep->channel = &src->ep.ch;
	ep->rpmi_data = bridge_src_rpmi_data_template;
	ep->rpmi_data.service_data = srvcdata;
	ep->rpmi_data.servicegrp_id = srvgrpid;
	ep->rpmi_data.num_services = num_services;

	src->ep.ch.channel_id = src->ep.channel_id;
	src->ep.ch.owner_domain = src->ep.dom;

	return SBI_OK;
}

static int bridge_get_domain_prop(const void *fdt, int nodeoff,
				  const char *propname, char *out,
				  size_t outsz)
{
	const fdt32_t *value;
	const char    *name;
	int            len, domain_nodeoff;
	size_t         n;

	value = fdt_getprop(fdt, nodeoff, propname, &len);
	if (!value || len < (int)sizeof(*value))
		return SBI_ENODEV;

	domain_nodeoff = fdt_node_offset_by_phandle(fdt,
					fdt32_to_cpu(*value));
	if (domain_nodeoff < 0)
		return SBI_ENODEV;

	name = fdt_get_name(fdt, domain_nodeoff, NULL);
	if (!name || !name[0] || !outsz)
		return SBI_ENODEV;

	n = sbi_strlen(name);
	if (n >= outsz)
		n = outsz - 1;
	sbi_memcpy(out, name, n);
	out[n] = '\0';

	return SBI_OK;
}

static int bridge_get_u32_prop(const void *fdt, int nodeoff,
			       const char *propname, u32 *out)
{
	const fdt32_t *value;
	int            len;

	value = fdt_getprop(fdt, nodeoff, propname, &len);
	if (!value || len < (int)sizeof(*value))
		return SBI_ENODEV;

	*out = fdt32_to_cpu(*value);
	return SBI_OK;
}

static void bridge_try_bind_domain(struct mpxy_bridge *br,
				   const struct sbi_domain *dom)
{
	u32 i, rc;

	if (!br || !dom)
		return;

	if (!br->dst.ep.dom &&
	    bridge_name_match(dom->name, br->dst.ep.domain_name)) {
		bridge_register_destination(br, (struct sbi_domain *)dom);
	}

	for (i = 0; i < br->source_count; i++) {
		if (!br->sources[i].ep.dom &&
			bridge_name_match(dom->name, br->sources[i].ep.domain_name)) {
			br->sources[i].ep.dom = (struct sbi_domain *)dom;

			rc = mpxy_rpmi_no_mbox_init(&br->sources[i].ep.rpmi_endpoint.rpmi_data,
				    br->sources[i].ep.channel_id,
				    br->sources[i].ep.domain_name);
			if (rc) {
				sbi_printf("%s: mpxy_rpmi_no_mbox_init failed with ret %d for source[%u]\n",
				   __func__, rc, i);
				br->sources[i].ep.dom = NULL;
			}
		}
	}

	if (br->dst.ep.dom) {
		for (i = 0; i < br->source_count; i++) {
			if (br->sources[i].ep.dom &&
			    !sbi_hartmask_equal(
				br->sources[i].ep.dom->possible_harts,
				br->dst.ep.dom->possible_harts)) {
				sbi_printf("%s: source[%u] and destination "
					   "have different possible harts\n",
					   __func__, i);
				br->sources[i].ep.dom = NULL;
			}
		}
	}
}

static int bridge_domain_notifier(struct sbi_domain *dom,
				  enum sbi_domain_notify_event event,
				  void *priv)
{
	struct mpxy_bridge *br;

	(void)priv;

	if (event == SBI_DOMAIN_NOTIFY_EVENT_REGISTER)
		sbi_list_for_each_entry(br, &mpxy_bridge_list, node)
			bridge_try_bind_domain(br, dom);

	return SBI_OK;
}

static int bridge_parse_fdt(const void *fdt, int nodeoff,
			    struct mpxy_bridge *br)
{
	int ret;

	ret = bridge_get_domain_prop(fdt, nodeoff,
				     SBI_MPXY_BRIDGE_PROP_DOMAIN_NAME,
				     br->dst.ep.domain_name,
				     sizeof(br->dst.ep.domain_name));
	if (ret)
		return ret;

	return bridge_get_u32_prop(fdt, nodeoff,
				   SBI_MPXY_BRIDGE_PROP_CHAN,
				   &br->dst.ep.channel_id);
}

static int bridge_count_sources(const void *fdt, int nodeoff)
{
	int child, count = 0;

	fdt_for_each_subnode(child, fdt, nodeoff)
		count++;

	return count;
}

static const struct sbi_rpmi_bridge_src *
bridge_find_source_driver(const void *fdt, int nodeoff)
{
	const struct sbi_rpmi_bridge_src *driver;
	const char *node_name;
	int i, rc, name_len;

	node_name = fdt_get_name(fdt, nodeoff, &name_len);
	if (!node_name)
		node_name = "<unknown>";

	for (i = 0; fdt_mpxy_rpmi_bridge_sources[i]; i++) {
		driver = fdt_mpxy_rpmi_bridge_sources[i];

		if (!driver->compatible) {
			sbi_printf("%s: driver[%d] has no compatible string\n",
				   __func__, i);
			continue;
		}

		rc = fdt_node_check_compatible(fdt, nodeoff,
					       driver->compatible);
		if (rc == 0)
			return driver;
	}

	return NULL;
}

static int bridge_probe_sources(const void *fdt, int bridge_nodeoff)
{
	const struct sbi_rpmi_bridge_src *driver;
	int child_nodeoff, rc;

	fdt_for_each_subnode(child_nodeoff, fdt, bridge_nodeoff) {
		driver = bridge_find_source_driver(fdt, child_nodeoff);
		if (!driver || !driver->probe)
			continue;

		rc = driver->probe(fdt, child_nodeoff);
		if (rc) {
			sbi_printf("%s: failed to probe source at node %d: %d\n",
				   __func__, child_nodeoff, rc);
		}
	}

	return SBI_OK;
}

static int bridge_parse_sources(const void *fdt, int nodeoff,
				struct mpxy_bridge *br)
{
	struct mpxy_bridge_source *src;
	int   child;
	u32   index = 0;
	int   ret = SBI_OK;

	fdt_for_each_subnode(child, fdt, nodeoff) {
		src = &br->sources[index];
		src->nodeoff = child;

		if ((ret = bridge_get_domain_prop(fdt, child,
				SBI_MPXY_BRIDGE_PROP_DOMAIN_NAME,
				src->ep.domain_name,
				sizeof(src->ep.domain_name))))
			continue;

		if ((ret = bridge_get_u32_prop(fdt, child,
				SBI_MPXY_BRIDGE_PROP_CHAN,
				&src->ep.channel_id)))
			continue;

		index++;
	}

	return ret;
}

static int bridge_validate(struct mpxy_bridge *br)
{
	u32 i, j;

	if (!br->dst.ep.domain_name[0] || !br->source_count)
		return SBI_EINVAL;

	for (i = 0; i < br->source_count; i++) {
		if (!br->sources[i].ep.domain_name[0] ||
		    br->sources[i].ep.channel_id == br->dst.ep.channel_id)
			return SBI_EINVAL;

		for (j = i + 1; j < br->source_count; j++)
			if (br->sources[i].ep.channel_id ==
			    br->sources[j].ep.channel_id)
				return SBI_EINVAL;
	}

	return SBI_OK;
}

static void bridge_free(struct mpxy_bridge *br)
{
	u32 i;

	if (!br)
		return;

	for (i = 0; i < br->store.hart_count; i++)
		if (br->store.slots[i].src_ctx)
			sbi_free(br->store.slots[i].src_ctx);

	if (br->store.slots)
		sbi_free(br->store.slots);

	if (br->sources)
		sbi_free(br->sources);

	sbi_free(br);
}

static struct mpxy_bridge *bridge_alloc(u32 hart_count, u32 source_count)
{
	struct mpxy_bridge *br;
	u32 i;

	if (!hart_count || !source_count)
		return NULL;

	br = sbi_zalloc(sizeof(*br));
	if (!br)
		return NULL;

	br->sources     = sbi_zalloc(sizeof(*br->sources) * source_count);
	br->store.slots = sbi_zalloc(sizeof(*br->store.slots) * hart_count);

	if (!br->sources || !br->store.slots) {
		bridge_free(br);
		return NULL;
	}

	for (i = 0; i < hart_count; i++) {
		br->store.slots[i].src_ctx = sbi_zalloc(
			sizeof(struct mpxy_bridge_src_ctx) * source_count);
		if (!br->store.slots[i].src_ctx) {
			bridge_free(br);
			return NULL;
		}
	}

	br->source_count      = source_count;
	br->store.hart_count  = hart_count;
	return br;
}

static int bridge_register_notifier(void)
{
	int ret = SBI_OK;

	if (!mpxy_bridge_notifier_registered) {
		ret = sbi_domain_register_notifier(
				bridge_domain_notifier,
				SBI_DOMAIN_NOTIFY_EVENT_REGISTER,
				NULL);
		if (!ret)
			mpxy_bridge_notifier_registered = true;
	}

	return ret;
}

static int bridge_init(const void *fdt, int nodeoff,
		       const struct fdt_match *match)
{
	struct mpxy_bridge *br = NULL;
	struct sbi_scratch *scratch;
	u32                 hart_count;
	u32                 source_count;
	u32                 i;
	int                 ret = SBI_OK;

	(void)match;

	scratch    = sbi_hartindex_to_scratch(current_hartindex());
	hart_count = sbi_platform_hart_count(sbi_platform_ptr(scratch));

	do {
		source_count = bridge_count_sources(fdt, nodeoff);
		br           = bridge_alloc(hart_count, source_count);
		if (!br) {
			ret = SBI_ENOMEM;
			break;
		}

		br->nodeoff = nodeoff;

		if ((ret = bridge_parse_fdt(fdt, nodeoff, br)))
			break;

		if ((ret = bridge_parse_sources(fdt, nodeoff, br)))
			break;

		if ((ret = bridge_validate(br)))
			break;

		sbi_list_add_tail(&br->node, &mpxy_bridge_list);

		if ((ret = bridge_probe_sources(fdt, nodeoff))) {
			sbi_list_del(&br->node);
			break;
		}

		for (i = 0; i < hart_count; i++)
			bridge_slot_release(br, &br->store.slots[i]);

		if ((ret = bridge_register_notifier()))
			sbi_list_del(&br->node);

	} while (0);

	if (ret && br)
		bridge_free(br);

	return ret;
}

static const struct fdt_match bridge_match[] = {
	{ .compatible = SBI_MPXY_BRIDGE_COMPATIBLE },
	{ }
};

const struct fdt_driver fdt_mpxy_rpmi_bridge = {
	.match_table = bridge_match,
	.init        = bridge_init,
};
