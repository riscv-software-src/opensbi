/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Anup Patel <apatel@ventanamicro.com>
 */

#ifndef __FDT_MAILBOX_H__
#define __FDT_MAILBOX_H__

#include <sbi_utils/fdt/fdt_driver.h>
#include <sbi_utils/mailbox/mailbox.h>

struct fdt_phandle_args;

/** FDT based mailbox driver */
struct fdt_mailbox {
	struct fdt_driver driver;
	int (*xlate)(struct mbox_controller *mbox,
		     const struct fdt_phandle_args *pargs,
		     u32 *out_chan_args);
};

/** Request a mailbox channel using "mboxes" DT property of client DT node */
int fdt_mailbox_request_chan(const void *fdt, int nodeoff, int index,
			     struct mbox_chan **out_chan);

/** Simple xlate function to convert one mailbox FDT cell into channel args */
int fdt_mailbox_simple_xlate(struct mbox_controller *mbox,
			     const struct fdt_phandle_args *pargs,
			     u32 *out_chan_args);

#endif
