/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Rahul Pathak <rpathak@ventanamicro.com>
 */

#ifndef __SBI_MPXY_H__
#define __SBI_MPXY_H__

#include <sbi/sbi_list.h>

struct sbi_scratch;

#define SBI_MPXY_MSGPROTO_VERSION(Major, Minor) ((Major << 16) | Minor)

enum sbi_mpxy_attr_id {
	/* Standard channel attributes managed by MPXY framework */
	SBI_MPXY_ATTR_MSG_PROT_ID		= 0x00000000,
	SBI_MPXY_ATTR_MSG_PROT_VER		= 0x00000001,
	SBI_MPXY_ATTR_MSG_MAX_LEN		= 0x00000002,
	SBI_MPXY_ATTR_MSG_SEND_TIMEOUT		= 0x00000003,
	SBI_MPXY_ATTR_MSG_COMPLETION_TIMEOUT	= 0x00000004,
	SBI_MPXY_ATTR_CHANNEL_CAPABILITY	= 0x00000005,
	SBI_MPXY_ATTR_SSE_EVENT_ID		= 0x00000006,
	SBI_MPXY_ATTR_MSI_CONTROL		= 0x00000007,
	SBI_MPXY_ATTR_MSI_ADDR_LO		= 0x00000008,
	SBI_MPXY_ATTR_MSI_ADDR_HI		= 0x00000009,
	SBI_MPXY_ATTR_MSI_DATA			= 0x0000000A,
	SBI_MPXY_ATTR_EVENTS_STATE_CONTROL	= 0x0000000B,
	SBI_MPXY_ATTR_STD_ATTR_MAX_IDX,
	/* Message protocol specific attributes, managed by
	 * message protocol driver */
	SBI_MPXY_ATTR_MSGPROTO_ATTR_START	= 0x80000000,
	SBI_MPXY_ATTR_MSGPROTO_ATTR_END		= 0xffffffff
};

/**
 * SBI MPXY Message Protocol IDs
 */
enum sbi_mpxy_msgproto_id {
	SBI_MPXY_MSGPROTO_RPMI_ID = 0x00000000,
	SBI_MPXY_MSGPROTO_MAX_IDX,
	/** Vendor specific message protocol IDs */
	SBI_MPXY_MSGPROTO_VENDOR_START	= 0x80000000,
	SBI_MPXY_MSGPROTO_VENDOR_END	= 0xffffffff
};

enum SBI_EXT_MPXY_SHMEM_FLAGS {
	SBI_EXT_MPXY_SHMEM_FLAG_OVERWRITE		= 0b00,
	SBI_EXT_MPXY_SHMEM_FLAG_OVERWRITE_RETURN	= 0b01,
	SBI_EXT_MPXY_SHMEM_FLAG_MAX_IDX
};

struct sbi_mpxy_msi_info {
	/* MSI target address low 32-bit */
	u32 msi_addr_lo;
	/* MSI target address high 32-bit */
	u32 msi_addr_hi;
	/* MSI data */
	u32 msi_data;
};

/**
 * Channel attributes.
 * NOTE: The sequence of attribute fields are as per the
 * defined sequence in the attribute table in spec(or as
 * per the enum sbi_mpxy_attr_id).
 */
struct sbi_mpxy_channel_attrs {
	/* Message protocol ID */
	u32 msg_proto_id;
	/* Message protocol Version */
	u32 msg_proto_version;
	/* Message protocol maximum message data length(bytes) */
	u32 msg_data_maxlen;
	/* Message protocol message send timeout
	 * in microseconds */
	u32 msg_send_timeout;
	/* Message protocol message response timeout in
	 * microseconds. Its the aggregate of msg_send_timeout
	 * and the timeout in receiving the response */
	u32 msg_completion_timeout;
	/* Bit array for channel capabilities */
	u32 capability;
	u32 sse_event_id;
	u32 msi_control;
	struct sbi_mpxy_msi_info msi_info;
	/* Events State Control */
	u32 eventsstate_ctrl;
};

/** A Message proxy channel accessible through SBI interface */
struct sbi_mpxy_channel {
	/** List head to a set of channels */
	struct sbi_dlist head;
	u32 channel_id;
	struct sbi_mpxy_channel_attrs attrs;

	/**
	 * Read message protocol attributes
	 * NOTE: inmem requires little-endian byte-ordering
	 */
	int (*read_attributes)(struct sbi_mpxy_channel *channel,
				u32 *outmem,
				u32 base_attr_id,
				u32 attr_count);

	/**
	 * Write message protocol attributes
	 * NOTE: outmem requires little-endian byte-ordering
	 */
	int (*write_attributes)(struct sbi_mpxy_channel *channel,
				u32 *inmem,
				u32 base_attr_id,
				u32 attr_count);
	/**
	 * Send a message and wait for response
	 * NOTE: msgbuf requires little-endian byte-ordering
	 */
	int (*send_message_with_response)(struct sbi_mpxy_channel *channel,
			    u32 msg_id, void *msgbuf, u32 msg_len,
			    void *respbuf, u32 resp_max_len,
			    unsigned long *resp_len);

	/** Send message without response */
	int (*send_message_without_response)(struct sbi_mpxy_channel *channel,
			    u32 msg_id, void *msgbuf, u32 msg_len);

	/**
	 * Get notifications events if supported on a channel
	 * NOTE: eventsbuf requires little-endian byte-ordering
	 */
	int (*get_notification_events)(struct sbi_mpxy_channel *channel,
					void *eventsbuf, u32 bufsize,
					unsigned long *events_len);

	/**
	 * Callback to enable the events state reporting
	 * in the message protocol implementation
	 */
	void (*switch_eventsstate)(u32 enable);
};

/** Register a Message proxy channel */
int sbi_mpxy_register_channel(struct sbi_mpxy_channel *channel);

/** Initialize Message proxy subsystem */
int sbi_mpxy_init(struct sbi_scratch *scratch);

/** Check if some Message proxy channel is available */
bool sbi_mpxy_channel_available(void);

/** Get message proxy shared memory size */
unsigned long sbi_mpxy_get_shmem_size(void);

/** Set message proxy shared memory on the calling HART */
int sbi_mpxy_set_shmem(unsigned long shmem_phys_lo,
		       unsigned long shmem_phys_hi,
		       unsigned long flags);

/** Get channel IDs list */
int sbi_mpxy_get_channel_ids(u32 start_index);

/** Read MPXY channel attributes */
int sbi_mpxy_read_attrs(u32 channel_id, u32 base_attr_id, u32 attr_count);

/** Write MPXY channel attributes */
int sbi_mpxy_write_attrs(u32 channel_id, u32 base_attr_id, u32 attr_count);

/**
 * Send a message over a MPXY channel.
 * In case if response is not expected, resp_data_len will be NULL.
 */
int sbi_mpxy_send_message(u32 channel_id, u8 msg_id,
					unsigned long msg_data_len,
					unsigned long *resp_data_len);

/** Get Message proxy notification events */
int sbi_mpxy_get_notification_events(u32 channel_id,
					unsigned long *events_len);

#endif
