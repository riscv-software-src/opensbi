/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Anup Patel <apatel@ventanamicro.com>
 */

#ifndef __RPMI_MAILBOX_H__
#define __RPMI_MAILBOX_H__

#include <sbi/sbi_error.h>
#include <sbi_utils/mailbox/mailbox.h>
#include <sbi_utils/mailbox/rpmi_msgprot.h>

#define rpmi_u32_count(__var)	(sizeof(__var) / sizeof(u32))

/** Convert RPMI error to SBI error */
int rpmi_xlate_error(enum rpmi_error error);

/** Typical RPMI normal request with at least status code in response */
int rpmi_normal_request_with_status(
			struct mbox_chan *chan, u32 service_id,
			void *req, u32 req_words, u32 req_endian_words,
			void *resp, u32 resp_words, u32 resp_endian_words);

/* RPMI posted request which is without any response*/
int rpmi_posted_request(
		struct mbox_chan *chan, u32 service_id,
		void *req, u32 req_words, u32 req_endian_words);

#endif /* !__RPMI_MAILBOX_H__ */
