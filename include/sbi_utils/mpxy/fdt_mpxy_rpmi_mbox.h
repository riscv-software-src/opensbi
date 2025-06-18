/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Anup Patel <apatel@ventanamicro.com>
 */

#ifndef __FDT_MPXY_RPMI_MBOX_H__
#define __FDT_MPXY_RPMI_MBOX_H__

#include <sbi/sbi_types.h>
#include <sbi/sbi_mpxy.h>
#include <sbi_utils/mailbox/fdt_mailbox.h>
#include <sbi_utils/mailbox/rpmi_msgprot.h>
#include <sbi_utils/mpxy/fdt_mpxy.h>

/** Convert the mpxy attribute ID to attribute array index */
#define attr_id2index(attr_id)	(attr_id - SBI_MPXY_ATTR_MSGPROTO_ATTR_START)

enum mpxy_msgprot_rpmi_attr_id {
	MPXY_MSGPROT_RPMI_ATTR_SERVICEGROUP_ID = SBI_MPXY_ATTR_MSGPROTO_ATTR_START,
	MPXY_MSGPROT_RPMI_ATTR_SERVICEGROUP_VERSION,
	MPXY_MSGPROT_RPMI_ATTR_IMPL_ID,
	MPXY_MSGPROT_RPMI_ATTR_IMPL_VERSION,
	MPXY_MSGPROT_RPMI_ATTR_MAX_ID
};

/**
 * MPXY message protocol attributes for RPMI
 * Order of attribute fields must follow the
 * attribute IDs in `enum mpxy_msgprot_rpmi_attr_id`
 */
struct mpxy_rpmi_channel_attrs {
	u32 servicegrp_id;
	u32 servicegrp_ver;
	u32 impl_id;
	u32 impl_ver;
};

/** Make sure all attributes are packed for direct memcpy */
#define assert_field_offset(field, attr_offset)				\
	_Static_assert(							\
		((offsetof(struct mpxy_rpmi_channel_attrs, field)) /	\
		 sizeof(u32)) == (attr_offset - SBI_MPXY_ATTR_MSGPROTO_ATTR_START),\
		"field " #field						\
		" from struct mpxy_rpmi_channel_attrs invalid offset, expected " #attr_offset)

assert_field_offset(servicegrp_id, MPXY_MSGPROT_RPMI_ATTR_SERVICEGROUP_ID);
assert_field_offset(servicegrp_ver, MPXY_MSGPROT_RPMI_ATTR_SERVICEGROUP_VERSION);
assert_field_offset(impl_id, MPXY_MSGPROT_RPMI_ATTR_IMPL_ID);
assert_field_offset(impl_ver, MPXY_MSGPROT_RPMI_ATTR_IMPL_VERSION);

/** MPXY RPMI service data for each service group */
struct mpxy_rpmi_service_data {
	u8 id;
	u32 min_tx_len;
	u32 max_tx_len;
	u32 min_rx_len;
	u32 max_rx_len;
};

/** MPXY RPMI mbox data for each service group */
struct mpxy_rpmi_mbox_data {
	u32 servicegrp_id;
	u32 num_services;
	struct mpxy_rpmi_service_data *service_data;

	/** Transfer RPMI service group message */
	int (*xfer_group)(void *context, struct mbox_chan *chan,
			  struct mbox_xfer *xfer);

	/** Setup RPMI service group context for MPXY */
	int (*setup_group)(void **context, struct mbox_chan *chan,
			   const struct mpxy_rpmi_mbox_data *data);

	/** Cleanup RPMI service group context for MPXY */
	void (*cleanup_group)(void *context);
};

/** Common probe function for MPXY RPMI drivers */
int mpxy_rpmi_mbox_init(const void *fdt, int nodeoff, const struct fdt_match *match);

#endif
