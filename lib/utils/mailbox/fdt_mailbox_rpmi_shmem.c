/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Rahul Pathak <rpathak@ventanamicro.com>
 *   Subrahmanya Lingappa <slingappa@ventanamicro.com>
 *   Anup Patel <apatel@ventanamicro.com>
 */

#include <libfdt.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_heap.h>
#include <sbi/sbi_timer.h>
#include <sbi/riscv_io.h>
#include <sbi/riscv_locks.h>
#include <sbi/riscv_asm.h>
#include <sbi/riscv_barrier.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/mailbox/mailbox.h>
#include <sbi_utils/mailbox/fdt_mailbox.h>
#include <sbi_utils/mailbox/rpmi_mailbox.h>

/** Minimum Base group version required */
#define RPMI_BASE_VERSION_MIN		RPMI_VERSION(1, 0)

/**************** RPMI Transport Structures and Macros ***********/

#define GET_SERVICEGROUP_ID(msg)		\
({						\
	struct rpmi_message *mbuf = msg;	\
	le16_to_cpu(mbuf->header.servicegroup_id);\
})

#define GET_SERVICE_ID(msg)			\
({						\
	struct rpmi_message *mbuf = msg;	\
	mbuf->header.service_id;		\
})

#define GET_FLAGS(msg)				\
({						\
	struct rpmi_message *mbuf = msg;	\
	mbuf->header.flags;			\
})

#define GET_MESSAGE_ID(msg)			\
({						\
	struct rpmi_message *mbuf = msg;	\
	((u32)mbuf->header.flags << (RPMI_MSG_FLAGS_OFFSET * 8)) | \
	((u32)mbuf->header.service_id << (RPMI_MSG_SERVICE_ID_OFFSET * 8)) | \
	((u32)le16_to_cpu(mbuf->header.servicegroup_id)); \
})

#define MAKE_MESSAGE_ID(__group_id, __service_id, __flags)	\
({						\
	u32 __ret = 0;				\
	__ret |= (u32)(__group_id) << (RPMI_MSG_SERVICEGROUP_ID_OFFSET * 8); \
	__ret |= (u32)(__service_id) << (RPMI_MSG_SERVICE_ID_OFFSET * 8); \
	__ret |= (u32)(__flags) << (RPMI_MSG_FLAGS_OFFSET * 8); \
	__ret;					\
})

#define GET_DLEN(msg)				\
({						\
	struct rpmi_message *mbuf = msg;	\
	le16_to_cpu(mbuf->header.datalen);	\
})

#define GET_TOKEN(msg)				\
({						\
	struct rpmi_message *mbuf = msg;	\
	le16_to_cpu(mbuf->header.token);	\
})

#define GET_MESSAGE_TYPE(msg)						\
({									\
	uint8_t flags = *((uint8_t *)msg + RPMI_MSG_FLAGS_OFFSET);	\
	((flags & RPMI_MSG_FLAGS_TYPE) >> RPMI_MSG_FLAGS_TYPE_POS));	\
})

enum rpmi_queue_type {
	RPMI_QUEUE_TYPE_REQ = 0,
	RPMI_QUEUE_TYPE_ACK = 1,
};

enum rpmi_queue_idx {
	RPMI_QUEUE_IDX_A2P_REQ = 0,
	RPMI_QUEUE_IDX_P2A_ACK = 1,
	RPMI_QUEUE_IDX_P2A_REQ = 2,
	RPMI_QUEUE_IDX_A2P_ACK = 3,
	RPMI_QUEUE_IDX_MAX_COUNT,
};

enum rpmi_reg_idx {
	RPMI_REG_IDX_DB_REG = 0, /* Doorbell register */
	RPMI_REG_IDX_MAX_COUNT,
};

/** Mailbox registers */
struct rpmi_mb_regs {
	/* doorbell from AP -> PuC*/
	volatile le32_t db_reg;
} __packed;

/** Single Queue Context Structure */
struct smq_queue_ctx {
	u32 queue_id;
	u32 num_slots;
	spinlock_t queue_lock;
	/* Type of queue - REQ or ACK */
	enum rpmi_queue_type queue_type;
	/* Pointers to the queue shared memory */
	volatile le32_t *headptr;
	volatile le32_t *tailptr;
	volatile uint8_t *buffer;
	/* Name of the queue */
	char name[RPMI_NAME_CHARS_MAX];
};

struct rpmi_srvgrp_chan {
	u32 servicegroup_id;
	u32 servicegroup_version;
	struct mbox_chan chan;
};

#define to_srvgrp_chan(mbox_chan)	\
		container_of(mbox_chan, struct rpmi_srvgrp_chan, chan);

struct rpmi_shmem_mbox_controller {
	/* Driver specific members */
	u32 slot_size;
	u32 queue_count;
	u32 p2a_doorbell_sysmsi_index;
	u32 a2p_doorbell_value;
	struct rpmi_mb_regs *mb_regs;
	struct smq_queue_ctx queue_ctx_tbl[RPMI_QUEUE_IDX_MAX_COUNT];
	/* Mailbox framework related members */
	struct mbox_controller controller;
	struct mbox_chan *base_chan;
	u32 impl_version;
	u32 impl_id;
	u32 spec_version;
	u32 plat_info_len;
	char *plat_info;
	struct {
		u8 f0_priv_level;
		bool f0_ev_notif_en;
	} base_flags;
};

/**************** Shared Memory Queues Helpers **************/

static bool __smq_queue_full(struct smq_queue_ctx *qctx)
{
	return ((le32_to_cpu(*qctx->tailptr) + 1) % qctx->num_slots ==
			le32_to_cpu(*qctx->headptr)) ? true : false;
}

static bool __smq_queue_empty(struct smq_queue_ctx *qctx)
{
	return (le32_to_cpu(*qctx->headptr) ==
		le32_to_cpu(*qctx->tailptr)) ? true : false;
}

static int __smq_rx(struct smq_queue_ctx *qctx, u32 slot_size,
		    u32 service_group_id, struct mbox_xfer *xfer)
{
	void *dst, *src;
	struct rpmi_message *msg;
	u32 i, tmp, pos, dlen, msgidn, headidx, tailidx;
	struct rpmi_message_args *args = xfer->args;
	bool no_rx_token = (args->flags & RPMI_MSG_FLAGS_NO_RX_TOKEN) ?
			   true : false;

	/* Rx sanity checks */
	if ((sizeof(u32) * args->rx_endian_words) >
	    (slot_size - sizeof(struct rpmi_message_header)))
		return SBI_EINVAL;
	if ((sizeof(u32) * args->rx_endian_words) > xfer->rx_len)
		return SBI_EINVAL;

	/* There should be some message in the queue */
	if (__smq_queue_empty(qctx))
		return SBI_ENOENT;

	/* Get the head/read index and tail/write index */
	headidx = le32_to_cpu(*qctx->headptr);
	tailidx = le32_to_cpu(*qctx->tailptr);

	/*
	 * Compute msgidn expected in the incoming message
	 * NOTE: DOORBELL bit is not expected to be set.
	 */
	msgidn = MAKE_MESSAGE_ID(service_group_id, args->service_id, args->type);

	/* Find the Rx message with matching token */
	pos = headidx;
	while (pos != tailidx) {
		src = (void *)qctx->buffer + (pos * slot_size);
		if ((no_rx_token && GET_MESSAGE_ID(src) == msgidn) ||
		    (GET_TOKEN(src) == (xfer->seq & RPMI_MSG_TOKEN_MASK)))
			break;
		pos = (pos + 1) % qctx->num_slots;
	}
	if (pos == tailidx)
		return SBI_ENOENT;

	/* If Rx message is not first message then make it first message */
	if (pos != headidx) {
		src = (void *)qctx->buffer + (pos * slot_size);
		dst = (void *)qctx->buffer + (headidx * slot_size);
		for (i = 0; i < slot_size / sizeof(u32); i++) {
			tmp = ((u32 *)dst)[i];
			((u32 *)dst)[i] = ((u32 *)src)[i];
			((u32 *)src)[i] = tmp;
		}
	}

	/* Update rx_token if not available */
	msg = (void *)qctx->buffer + (headidx * slot_size);
	if (no_rx_token)
		args->rx_token = GET_TOKEN(msg);

	/* Extract data from the first message */
	if (xfer->rx) {
		args->rx_data_len = dlen = GET_DLEN(msg);
		if (dlen > xfer->rx_len)
			dlen = xfer->rx_len;
		src = (void *)msg + sizeof(struct rpmi_message_header);
		dst = xfer->rx;
		for (i = 0; i < args->rx_endian_words; i++)
			((u32 *)dst)[i] = le32_to_cpu(((u32 *)src)[i]);
		dst += sizeof(u32) * args->rx_endian_words;
		src += sizeof(u32) * args->rx_endian_words;
		sbi_memcpy(dst, src,
			xfer->rx_len - (sizeof(u32) * args->rx_endian_words));
	}

	/* Update the head/read index */
	*qctx->headptr = cpu_to_le32(headidx + 1) % qctx->num_slots;

	/* Make sure updates to head are immediately visible to PuC */
	smp_wmb();

	return SBI_OK;
}

static int __smq_tx(struct smq_queue_ctx *qctx, struct rpmi_mb_regs *mb_regs,
		    u32 a2p_doorbell_value, u32 slot_size, u32 service_group_id,
		    struct mbox_xfer *xfer)
{
	u32 i, tailidx;
	void *dst, *src;
	struct rpmi_message_header header = { 0 };
	struct rpmi_message_args *args = xfer->args;

	/* Tx sanity checks */
	if ((sizeof(u32) * args->tx_endian_words) >
	    (slot_size - sizeof(struct rpmi_message_header)))
		return SBI_EINVAL;
	if ((sizeof(u32) * args->tx_endian_words) > xfer->tx_len)
		return SBI_EINVAL;

	/* There should be some room in the queue */
	if (__smq_queue_full(qctx))
		return SBI_ENOMEM;

	/* Get the tail/write index */
	tailidx = le32_to_cpu(*qctx->tailptr);

	/* Prepare the header to be written into the slot */
	header.servicegroup_id = cpu_to_le16(service_group_id);
	header.service_id = args->service_id;
	header.flags = args->type;
	header.datalen = cpu_to_le16((u16)xfer->tx_len);
	header.token = cpu_to_le16((u16)xfer->seq);

	/* Write header into the slot */
	dst = (char *)qctx->buffer + (tailidx * slot_size);
	sbi_memcpy(dst, &header, sizeof(header));
	dst += sizeof(header);

	/* Write data into the slot */
	if (xfer->tx) {
		src = xfer->tx;
		for (i = 0; i < args->tx_endian_words; i++)
			((u32 *)dst)[i] = cpu_to_le32(((u32 *)src)[i]);
		dst += sizeof(u32) * args->tx_endian_words;
		src += sizeof(u32) * args->tx_endian_words;
		sbi_memcpy(dst, src,
			xfer->tx_len - (sizeof(u32) * args->tx_endian_words));
	}

	/* Make sure queue chanages are visible to PuC before updating tail */
	smp_wmb();

	/* Update the tail/write index */
	*qctx->tailptr = cpu_to_le32(tailidx + 1) % qctx->num_slots;

	/* Ring the RPMI doorbell if present */
	if (mb_regs)
		writel(a2p_doorbell_value, &mb_regs->db_reg);

	return SBI_OK;
}

static int smq_rx(struct rpmi_shmem_mbox_controller *mctl,
		  u32 queue_id, u32 service_group_id, struct mbox_xfer *xfer)
{
	int ret, rxretry = 0;
	struct smq_queue_ctx *qctx;

	if (mctl->queue_count < queue_id) {
		sbi_printf("%s: invalid queue_id or service_group_id\n",
			   __func__);
		return SBI_EINVAL;
	}
	qctx = &mctl->queue_ctx_tbl[queue_id];

	/*
	 * Once the timeout happens and call this function is returned
	 * to the client then there is no way to deliver the response
	 * message after that if it comes later.
	 *
	 * REVISIT: In complete timeout duration how much duration
	 * it should wait(delay) before recv retry. udelay or mdelay
	 */
	do {
		spin_lock(&qctx->queue_lock);
		ret = __smq_rx(qctx, mctl->slot_size, service_group_id, xfer);
		spin_unlock(&qctx->queue_lock);
		if (!ret)
			return 0;

		sbi_timer_mdelay(1);
		rxretry += 1;
	} while (rxretry < xfer->rx_timeout);

	return SBI_ETIMEDOUT;
}

static int smq_tx(struct rpmi_shmem_mbox_controller *mctl,
		  u32 queue_id, u32 service_group_id, struct mbox_xfer *xfer)
{
	int ret, txretry = 0;
	struct smq_queue_ctx *qctx;

	if (mctl->queue_count < queue_id) {
		sbi_printf("%s: invalid queue_id or service_group_id\n",
			   __func__);
		return SBI_EINVAL;
	}
	qctx = &mctl->queue_ctx_tbl[queue_id];

	/*
	 * Ignoring the tx timeout since in RPMI has no mechanism
	 * with which other side can let know about the reception of
	 * message which marks as tx complete. For RPMI tx complete is
	 * marked as done when message in successfully copied in queue.
	 *
	 * REVISIT: In complete timeout duration how much duration
	 * it should wait(delay) before send retry. udelay or mdelay
	 */
	do {
		spin_lock(&qctx->queue_lock);
		ret = __smq_tx(qctx, mctl->mb_regs, mctl->a2p_doorbell_value,
				mctl->slot_size, service_group_id, xfer);
		spin_unlock(&qctx->queue_lock);
		if (!ret)
			return 0;

		sbi_timer_mdelay(1);
		txretry += 1;
	} while (txretry < xfer->tx_timeout);

	return SBI_ETIMEDOUT;
}

static int rpmi_get_platform_info(struct rpmi_shmem_mbox_controller *mctl)
{
	int ret = SBI_OK;

	/**
	 * platform string can occupy max possible size
	 * max possible space in the message data as
	 * per the format
	 */
	struct rpmi_base_get_platform_info_resp *resp =
			sbi_zalloc(RPMI_MSG_DATA_SIZE(mctl->slot_size));
	if (!resp)
		return SBI_ENOMEM;

	ret = rpmi_normal_request_with_status(mctl->base_chan,
				 RPMI_BASE_SRV_GET_PLATFORM_INFO,
				 NULL, 0, 0,
				 resp,
				 RPMI_MSG_DATA_SIZE(mctl->slot_size)/4,
				 RPMI_MSG_DATA_SIZE(mctl->slot_size)/4);
	if (ret)
		goto fail_free_resp;

	mctl->plat_info_len = resp->plat_info_len;
	mctl->plat_info = sbi_zalloc(mctl->plat_info_len);
	if (!mctl->plat_info) {
		ret = SBI_ENOMEM;
		goto fail_free_resp;
	}

	sbi_strncpy(mctl->plat_info, resp->plat_info, mctl->plat_info_len);

fail_free_resp:
	sbi_free(resp);
	return ret;
}

static int smq_base_get_two_u32(struct rpmi_shmem_mbox_controller *mctl,
				u32 service_id, u32 *inarg, u32 *outvals)
{
	return rpmi_normal_request_with_status(
			mctl->base_chan, service_id,
			inarg, (inarg) ? 1 : 0, (inarg) ? 1 : 0,
			outvals, 2, 2);
}

/**************** Mailbox Controller Functions **************/

static int rpmi_shmem_mbox_xfer(struct mbox_chan *chan, struct mbox_xfer *xfer)
{
	int ret;
	u32 tx_qid = 0, rx_qid = 0;
	struct rpmi_shmem_mbox_controller *mctl =
			container_of(chan->mbox,
				     struct rpmi_shmem_mbox_controller,
				     controller);
	struct rpmi_srvgrp_chan *srvgrp_chan = to_srvgrp_chan(chan);

	struct rpmi_message_args *args = xfer->args;
	bool do_tx = (args->flags & RPMI_MSG_FLAGS_NO_TX) ? false : true;
	bool do_rx = (args->flags & RPMI_MSG_FLAGS_NO_RX) ? false : true;

	if (!do_tx && !do_rx)
		return SBI_EINVAL;

	switch (args->type) {
	case RPMI_MSG_NORMAL_REQUEST:
		if (do_tx && do_rx) {
			tx_qid = RPMI_QUEUE_IDX_A2P_REQ;
			rx_qid = RPMI_QUEUE_IDX_P2A_ACK;
		} else if (do_tx) {
			tx_qid = RPMI_QUEUE_IDX_A2P_REQ;
		} else if (do_rx) {
			rx_qid = RPMI_QUEUE_IDX_P2A_REQ;
		}
		break;
	case RPMI_MSG_POSTED_REQUEST:
		if (do_tx && do_rx)
			return SBI_EINVAL;
		if (do_tx) {
			tx_qid = RPMI_QUEUE_IDX_A2P_REQ;
		} else {
			rx_qid = RPMI_QUEUE_IDX_P2A_REQ;
		}
		break;
	case RPMI_MSG_ACKNOWLDGEMENT:
		if (do_tx && do_rx)
			return SBI_EINVAL;
		if (do_tx) {
			tx_qid = RPMI_QUEUE_IDX_A2P_ACK;
		} else {
			rx_qid = RPMI_QUEUE_IDX_P2A_ACK;
		}
		break;
	default:
		return SBI_ENOTSUPP;
	}

	if (do_tx) {
		ret = smq_tx(mctl, tx_qid, srvgrp_chan->servicegroup_id, xfer);
		if (ret)
			return ret;
	}

	if (do_rx) {
		ret = smq_rx(mctl, rx_qid, srvgrp_chan->servicegroup_id, xfer);
		if (ret)
			return ret;
	}

	return 0;
}

static int rpmi_shmem_mbox_get_attribute(struct mbox_chan *chan,
					 int attr_id, void *out_value)
{
	struct rpmi_shmem_mbox_controller *mctl =
			container_of(chan->mbox,
				     struct rpmi_shmem_mbox_controller,
				     controller);
	struct rpmi_srvgrp_chan *srvgrp_chan = to_srvgrp_chan(chan);

	switch (attr_id) {
	case RPMI_CHANNEL_ATTR_PROTOCOL_VERSION:
		*((u32 *)out_value) = mctl->spec_version;
		break;
	case RPMI_CHANNEL_ATTR_MAX_DATA_LEN:
		*((u32 *)out_value) = RPMI_MSG_DATA_SIZE(mctl->slot_size);
		break;
	case RPMI_CHANNEL_ATTR_P2A_DOORBELL_SYSMSI_INDEX:
		*((u32 *)out_value) = mctl->p2a_doorbell_sysmsi_index;
		break;
	case RPMI_CHANNEL_ATTR_TX_TIMEOUT:
		*((u32 *)out_value) = RPMI_DEF_TX_TIMEOUT;
		break;
	case RPMI_CHANNEL_ATTR_RX_TIMEOUT:
		*((u32 *)out_value) = RPMI_DEF_RX_TIMEOUT;
		break;
	case RPMI_CHANNEL_ATTR_SERVICEGROUP_ID:
		*((u32 *)out_value) = srvgrp_chan->servicegroup_id;
		break;
	case RPMI_CHANNEL_ATTR_SERVICEGROUP_VERSION:
		*((u32 *)out_value) = srvgrp_chan->servicegroup_version;
		break;
	case RPMI_CHANNEL_ATTR_IMPL_ID:
		*((u32 *)out_value) = mctl->impl_id;
		break;
	case RPMI_CHANNEL_ATTR_IMPL_VERSION:
		*((u32 *)out_value) = mctl->impl_version;
		break;
	default:
		return SBI_ENOTSUPP;
	}

	return 0;
}

static struct mbox_chan *rpmi_shmem_mbox_request_chan(
						struct mbox_controller *mbox,
						u32 *chan_args)
{
	int ret;
	u32 tval[2] = { 0 };
	struct rpmi_srvgrp_chan *srvgrp_chan;
	struct rpmi_shmem_mbox_controller *mctl =
			container_of(mbox,
				     struct rpmi_shmem_mbox_controller,
				     controller);

	/* Service group id not defined or in reserved range is invalid */
	if (chan_args[0] >= RPMI_SRVGRP_ID_MAX_COUNT &&
		chan_args[0] <= RPMI_SRVGRP_RESERVE_END)
		return NULL;

	/* Base serivce group is always present so probe other groups */
	if (chan_args[0] != RPMI_SRVGRP_BASE) {
		/* Probe service group */
		ret = smq_base_get_two_u32(mctl,
					   RPMI_BASE_SRV_PROBE_SERVICE_GROUP,
					   chan_args, tval);
		if (ret || !tval[1])
			return NULL;
	}

	srvgrp_chan = sbi_zalloc(sizeof(*srvgrp_chan));
	if (!srvgrp_chan)
		return NULL;

	srvgrp_chan->servicegroup_id = chan_args[0];
	srvgrp_chan->servicegroup_version = tval[1];

	return &srvgrp_chan->chan;
}

static void rpmi_shmem_mbox_free_chan(struct mbox_controller *mbox,
				      struct mbox_chan *chan)
{
	struct rpmi_srvgrp_chan *srvgrp_chan = to_srvgrp_chan(chan);
	sbi_free(srvgrp_chan);
}

extern struct fdt_mailbox fdt_mailbox_rpmi_shmem;

static int rpmi_shmem_transport_init(struct rpmi_shmem_mbox_controller *mctl,
				     const void *fdt, int nodeoff)
{
	const char *name;
	const fdt32_t *prop;
	int count, len, ret, qid;
	uint64_t reg_addr, reg_size;
	struct smq_queue_ctx *qctx;

	ret = fdt_node_check_compatible(fdt, nodeoff,
					"riscv,rpmi-shmem-mbox");
	if (ret)
		return ret;

	/* get queue slot size in bytes */
	prop = fdt_getprop(fdt, nodeoff, "riscv,slot-size", &len);
	if (!prop)
		return SBI_ENOENT;

	mctl->slot_size = fdt32_to_cpu(*prop);
	if (mctl->slot_size < RPMI_SLOT_SIZE_MIN) {
		sbi_printf("%s: slot_size < mimnum required message size\n",
			   __func__);
		mctl->slot_size = RPMI_SLOT_SIZE_MIN;
	}

	/* get p2a doorbell system MSI index */
	prop = fdt_getprop(fdt, nodeoff, "riscv,p2a-doorbell-sysmsi-index", &len);
	mctl->p2a_doorbell_sysmsi_index = prop ? fdt32_to_cpu(*prop) : -1U;

	/* get a2p doorbell value */
	prop = fdt_getprop(fdt, nodeoff, "riscv,a2p-doorbell-value", &len);
	mctl->a2p_doorbell_value = prop ? fdt32_to_cpu(*prop) : 1;

	/*
	 * queue names count is taken as the number of queues
	 * supported which make it mandatory to provide the
	 * name of the queue.
	 */
	count = fdt_stringlist_count(fdt, nodeoff, "reg-names");
	if (count < 0 ||
	    count > (RPMI_QUEUE_IDX_MAX_COUNT + RPMI_REG_IDX_MAX_COUNT))
		return SBI_EINVAL;

	mctl->queue_count = count - RPMI_REG_IDX_MAX_COUNT;

	/* parse all queues and populate queues context structure */
	for (qid = 0; qid < mctl->queue_count; qid++) {
		qctx = &mctl->queue_ctx_tbl[qid];

		/* get each queue share-memory base address and size*/
		ret = fdt_get_node_addr_size(fdt, nodeoff, qid,
					     &reg_addr, &reg_size);
		if (ret < 0 || !reg_addr || !reg_size)
			return SBI_ENOENT;

		ret = sbi_domain_root_add_memrange(reg_addr, reg_size, reg_size,
						   (SBI_DOMAIN_MEMREGION_MMIO |
						    SBI_DOMAIN_MEMREGION_M_READABLE |
						    SBI_DOMAIN_MEMREGION_M_WRITABLE));
		if (ret)
			return ret;

		/* calculate number of slots in each queue */
		qctx->num_slots =
			(reg_size - (mctl->slot_size * RPMI_QUEUE_HEADER_SLOTS)) / mctl->slot_size;

		/* setup queue pointers */
		qctx->headptr = ((void *)(unsigned long)reg_addr) +
				RPMI_QUEUE_HEAD_SLOT * mctl->slot_size;
		qctx->tailptr = ((void *)(unsigned long)reg_addr) +
				RPMI_QUEUE_TAIL_SLOT * mctl->slot_size;
		qctx->buffer = ((void *)(unsigned long)reg_addr) +
				RPMI_QUEUE_HEADER_SLOTS * mctl->slot_size;

		/* get the queue name */
		name = fdt_stringlist_get(fdt, nodeoff, "reg-names",
					  qid, &len);
		if (!name || (name && len < 0))
			return len;

		sbi_memcpy(qctx->name, name, len);

		/* store the index as queue_id */
		qctx->queue_id = qid;

		SPIN_LOCK_INIT(qctx->queue_lock);
	}

	/* get the a2p-doorbell property name */
	name = fdt_stringlist_get(fdt, nodeoff, "reg-names", qid, &len);
	if (!name || (name && len < 0))
		return len;

	/* fetch doorbell register address*/
	ret = fdt_get_node_addr_size(fdt, nodeoff, qid, &reg_addr,
				       &reg_size);
	if (!ret && !(strncmp(name, "a2p-doorbell", strlen("a2p-doorbell")))) {
		mctl->mb_regs = (void *)(unsigned long)reg_addr;
		ret = sbi_domain_root_add_memrange(reg_addr, reg_size, reg_size,
						   (SBI_DOMAIN_MEMREGION_MMIO |
						    SBI_DOMAIN_MEMREGION_M_READABLE |
						    SBI_DOMAIN_MEMREGION_M_WRITABLE));
		if (ret)
			return ret;
	}

	return SBI_SUCCESS;
}

static int rpmi_shmem_mbox_init(const void *fdt, int nodeoff,
				const struct fdt_match *match)
{
	struct rpmi_base_get_attributes_resp resp;
	struct rpmi_shmem_mbox_controller *mctl;
	struct rpmi_srvgrp_chan *base_srvgrp;
	u32 tval[2], args[1];
	int ret = 0;

	mctl = sbi_zalloc(sizeof(*mctl));
	if (!mctl)
		return SBI_ENOMEM;

	/* Initialization transport from device tree */
	ret = rpmi_shmem_transport_init(mctl, fdt, nodeoff);
	if (ret)
		goto fail_free_controller;

	/* Register mailbox controller */
	mctl->controller.id = nodeoff;
	mctl->controller.max_xfer_len =
			mctl->slot_size - sizeof(struct rpmi_message_header);
	mctl->controller.driver = &fdt_mailbox_rpmi_shmem;
	mctl->controller.request_chan = rpmi_shmem_mbox_request_chan;
	mctl->controller.free_chan = rpmi_shmem_mbox_free_chan;
	mctl->controller.xfer = rpmi_shmem_mbox_xfer;
	mctl->controller.get_attribute = rpmi_shmem_mbox_get_attribute;
	ret = mbox_controller_add(&mctl->controller);
	if (ret)
		goto fail_free_controller;

	/* Request base service group channel */
	tval[0] = RPMI_SRVGRP_BASE;
	mctl->base_chan = mbox_controller_request_chan(&mctl->controller,
							tval);
	if (!mctl->base_chan) {
		ret = SBI_ENOENT;
		goto fail_remove_controller;
	}

	/* Update base service group version */
	base_srvgrp = to_srvgrp_chan(mctl->base_chan);
	args[0] = RPMI_SRVGRP_BASE;
	ret = smq_base_get_two_u32(mctl, RPMI_BASE_SRV_PROBE_SERVICE_GROUP,
				   &args[0], tval);
	if (ret)
		goto fail_free_chan;
	base_srvgrp->servicegroup_version = tval[1];
	if (base_srvgrp->servicegroup_version < RPMI_BASE_VERSION_MIN) {
		ret = SBI_EINVAL;
		goto fail_free_chan;
	}

	/* Get implementation id */
	ret = smq_base_get_two_u32(mctl,
				   RPMI_BASE_SRV_GET_IMPLEMENTATION_VERSION,
				   NULL, tval);
	if (ret)
		goto fail_free_chan;
	mctl->impl_version = tval[1];

	/* Get implementation version */
	ret = smq_base_get_two_u32(mctl, RPMI_BASE_SRV_GET_IMPLEMENTATION_IDN,
				   NULL, tval);
	if (ret)
		goto fail_free_chan;
	mctl->impl_id = tval[1];

	/* Get specification version */
	ret = smq_base_get_two_u32(mctl, RPMI_BASE_SRV_GET_SPEC_VERSION,
				   NULL, tval);
	if (ret)
		goto fail_free_chan;
	mctl->spec_version = tval[1];
	if (mctl->spec_version < RPMI_BASE_VERSION_MIN ||
	    mctl->spec_version != base_srvgrp->servicegroup_version) {
		ret = SBI_EINVAL;
		goto fail_free_chan;
	}

	/* Get optional features implementation flags */
	ret = rpmi_normal_request_with_status(
			mctl->base_chan, RPMI_BASE_SRV_GET_ATTRIBUTES,
			NULL, 0, 0,
			&resp, rpmi_u32_count(resp), rpmi_u32_count(resp));
	if (ret)
		goto fail_free_chan;

	/* 1: M-mode, 0: S-mode */
	mctl->base_flags.f0_priv_level =
			resp.f0 & RPMI_BASE_FLAGS_F0_PRIVILEGE ? 1 : 0;
	/* 1: Supported, 0: Not Supported */
	mctl->base_flags.f0_ev_notif_en =
			resp.f0 & RPMI_BASE_FLAGS_F0_EV_NOTIFY ? 1 : 0;

	/* We only use M-mode RPMI context in OpenSBI */
	if (!mctl->base_flags.f0_priv_level) {
		ret = SBI_ENODEV;
		goto fail_free_chan;
	}

	/*
	 * Continue without platform information string if not
	 * available or if an error is encountered while fetching
	 */
	rpmi_get_platform_info(mctl);

	return 0;

fail_free_chan:
	mbox_controller_free_chan(mctl->base_chan);
fail_remove_controller:
	mbox_controller_remove(&mctl->controller);
fail_free_controller:
	sbi_free(mctl);
	return ret;
}

static const struct fdt_match rpmi_shmem_mbox_match[] = {
	{ .compatible = "riscv,rpmi-shmem-mbox" },
	{ },
};

struct fdt_mailbox fdt_mailbox_rpmi_shmem = {
	.driver = {
		.match_table = rpmi_shmem_mbox_match,
		.init = rpmi_shmem_mbox_init,
	},
	.xlate = fdt_mailbox_simple_xlate,
};
