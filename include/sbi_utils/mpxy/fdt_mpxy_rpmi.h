/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Anup Patel <apatel@ventanamicro.com>
 */

#ifndef __FDT_MPXY_RPMI_H__
#define __FDT_MPXY_RPMI_H__

#include <sbi_utils/mailbox/rpmi_msgprot.h>

struct mbox_chan;
struct mbox_xfer;

/** MPXY RPMI service data for each service group */
struct mpxy_rpmi_service_data {
	u8 id;
	u32 min_tx_len;
	u32 max_tx_len;
	u32 min_rx_len;
	u32 max_rx_len;
};

/** MPXY RPMI data for each service group */
struct mpxy_rpmi_data {
	u32 servicegrp_id;
	u32 num_services;
	struct mpxy_rpmi_service_data *service_data;

	/** Get RPMI service group attribute */
	int (*get_attribute_group)(void *context, struct mbox_chan *chan,
				   enum rpmi_channel_attribute_id attr_id,
				   u32 *out_value);

	/** Transfer RPMI service group message */
	int (*xfer_group)(void *context, struct mbox_chan *chan,
			  struct mbox_xfer *xfer);

	/** Setup RPMI service group context for MPXY */
	int (*setup_group)(void **context, struct mbox_chan *chan,
			   const struct mpxy_rpmi_data *data);

	/** Cleanup RPMI service group context for MPXY */
	void (*cleanup_group)(void *context);
};

/** Function to instantiate MPXY RPMI channel */
int mpxy_rpmi_no_mbox_init(const struct mpxy_rpmi_data *data, u32 mpxy_channel_id,
			   const char *dom_name);

/** FDT probe function to instantiate MPXY RPMI channel */
int fdt_mpxy_rpmi_init(const void *fdt, int nodeoff, const struct fdt_match *match);

#endif
