/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Anup Patel <apatel@ventanamicro.com>
 */

#ifndef __MAILBOX_H__
#define __MAILBOX_H__

#include <sbi/sbi_types.h>
#include <sbi/sbi_list.h>
#include <sbi/riscv_atomic.h>

/** Representation of a mailbox channel */
struct mbox_chan {
	/** List head */
	struct sbi_dlist node;
	/** Pointer to the mailbox controller */
	struct mbox_controller *mbox;
	/**
	 * Arguments (or parameters) to identify a mailbox channel
	 * within a mailbox controller.
	 */
#define MBOX_CHAN_MAX_ARGS	2
	u32 chan_args[MBOX_CHAN_MAX_ARGS];
};

#define to_mbox_chan(__node)	\
	container_of((__node), struct mbox_chan, node)

/**
 * Representation of a mailbox data transfer
 *
 * NOTE: If both "tx" and "rx" are non-NULL then Tx is done before Rx.
 */
struct mbox_xfer {
#define MBOX_XFER_SEQ			(1UL << 0)
	/** Transfer flags */
	unsigned long flags;
	/** Transfer arguments (or parameters) */
	void *args;
	/**
	 * Sequence number
	 *
	 * If MBOX_XFER_SEQ is not set in flags then mbox_chan_xfer()
	 * will generate a unique sequence number and update this field
	 * else mbox_chan_xfer() will blindly use the sequence number
	 * specified by this field.
	 */
	long seq;
	/** Send data pointer */
	void *tx;
	/** Send data length (valid only if tx != NULL) */
	unsigned long tx_len;
	/**
	 * Send timeout milliseconds (valid only if tx != NULL)
	 *
	 * If this field is non-zero along with tx != NULL then the
	 * mailbox controller driver will wait specified milliseconds
	 * for send data transfer to complete else the mailbox controller
	 * driver will not wait.
	 */
	unsigned long tx_timeout;
	/** Receive data pointer */
	void *rx;
	/** Receive data length (valid only if rx != NULL) */
	unsigned long rx_len;
	/**
	 * Receive timeout milliseconds (valid only if rx != NULL)
	 *
	 * If this field is non-zero along with rx != NULL then the
	 * mailbox controller driver will wait specified milliseconds
	 * for receive data transfer to complete else the mailbox
	 * controller driver will not wait.
	 */
	unsigned long rx_timeout;
};

#define mbox_xfer_init_tx(__p, __a, __t, __t_len, __t_tim)		\
do {									\
	(__p)->flags = 0;						\
	(__p)->args = (__a);						\
	(__p)->tx = (__t);						\
	(__p)->tx_len = (__t_len);					\
	(__p)->tx_timeout = (__t_tim);					\
	(__p)->rx = NULL;						\
	(__p)->rx_len = 0;						\
	(__p)->rx_timeout = 0;						\
} while (0)

#define mbox_xfer_init_rx(__p, __a, __r, __r_len, __r_tim)		\
do {									\
	(__p)->flags = 0;						\
	(__p)->args = (__a);						\
	(__p)->tx = NULL;						\
	(__p)->tx_len = 0;						\
	(__p)->tx_timeout = 0;						\
	(__p)->rx = (__r);						\
	(__p)->rx_len = (__r_len);					\
	(__p)->rx_timeout = (__r_tim);					\
} while (0)

#define mbox_xfer_init_txrx(__p, __a, __t, __t_len, __t_tim, __r, __r_len, __r_tim)\
do {									\
	(__p)->flags = 0;						\
	(__p)->args = (__a);						\
	(__p)->tx = (__t);						\
	(__p)->tx_len = (__t_len);					\
	(__p)->tx_timeout = (__t_tim);					\
	(__p)->rx = (__r);						\
	(__p)->rx_len = (__r_len);					\
	(__p)->rx_timeout = (__r_tim);					\
} while (0)

#define mbox_xfer_set_sequence(__p, __seq)				\
do {									\
	(__p)->flags |= MBOX_XFER_SEQ;					\
	(__p)->seq = (__seq);						\
} while (0)

/** Representation of a mailbox controller */
struct mbox_controller {
	/** List head */
	struct sbi_dlist node;
	/** Next sequence atomic counter */
	atomic_t xfer_next_seq;
	/* List of mailbox channels */
	struct sbi_dlist chan_list;
	/** Unique ID of the mailbox controller assigned by the driver */
	unsigned int id;
	/** Maximum length of transfer supported by the mailbox controller */
	unsigned int max_xfer_len;
	/** Pointer to mailbox driver owning this mailbox controller */
	void *driver;
	/** Request a mailbox channel from the mailbox controller */
	struct mbox_chan *(*request_chan)(struct mbox_controller *mbox,
					  u32 *chan_args);
	/** Free a mailbox channel from the mailbox controller */
	void (*free_chan)(struct mbox_controller *mbox,
			  struct mbox_chan *chan);
	/** Transfer data over mailbox channel */
	int (*xfer)(struct mbox_chan *chan, struct mbox_xfer *xfer);
	/** Get an attribute of mailbox channel */
	int (*get_attribute)(struct mbox_chan *chan, int attr_id, void *out_value);
	/** Set an attribute of mailbox channel */
	int (*set_attribute)(struct mbox_chan *chan, int attr_id, void *new_value);
};

#define to_mbox_controller(__node)	\
	container_of((__node), struct mbox_controller, node)

/** Find a registered mailbox controller */
struct mbox_controller *mbox_controller_find(unsigned int id);

/** Register mailbox controller */
int mbox_controller_add(struct mbox_controller *mbox);

/** Un-register mailbox controller */
void mbox_controller_remove(struct mbox_controller *mbox);

/** Request a mailbox channel */
struct mbox_chan *mbox_controller_request_chan(struct mbox_controller *mbox,
					       u32 *chan_args);

/** Free a mailbox channel */
void mbox_controller_free_chan(struct mbox_chan *chan);

/** Data transfer over mailbox channel */
int mbox_chan_xfer(struct mbox_chan *chan, struct mbox_xfer *xfer);

/** Get an attribute of mailbox channel */
int mbox_chan_get_attribute(struct mbox_chan *chan, int attr_id, void *out_value);

/** Set an attribute of mailbox channel */
int mbox_chan_set_attribute(struct mbox_chan *chan, int attr_id, void *new_value);

#endif
