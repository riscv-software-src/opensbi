/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2026 Qualcomm, Inc
 *
 * Authors:
 *   Pawandeep Oza <pawandeep.oza@oss.qualcomm.com>
 */

#ifndef __FDT_MPXY_BRIDGE_H__
#define __FDT_MPXY_BRIDGE_H__

#include <sbi/sbi_console.h>
#include <sbi/sbi_types.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_string.h>
#include <sbi_utils/mpxy/fdt_mpxy_rpmi.h>

#define SBI_MPXY_BRIDGE_DEFAULT_MSG_DATA_MAXLEN  4096
#define SBI_MPXY_BRIDGE_DOMAIN_NAME_MAXLEN       64

struct mpxy_bridge_source;

struct mpxy_bridge_dst_ctx {
	void          *respbuf;
	u32            resp_bufsize;
	unsigned long *resp_len_ptr;
	u32            start_index;
};

struct mpxy_bridge_src_ctx {
	struct rpmi_message_header header;;
	bool occupied;
	bool retrieved;
	bool completed;
	void *msgbuf;
	void *respbuf;
	u32  msg_len;
	u32  resp_bufsize;
	u32  resp_length;
};

struct mpxy_bridge_source_driver {
    const struct fdt_match *match_table;
    int (*probe)(const void *fdt,
			int nodeoff, const struct fdt_match *match);
};

struct mpxy_bridge_source_ops {
	int (*xfer) (struct mpxy_bridge_src_ctx *src,
				struct mpxy_bridge_dst_ctx *dst);
	int (*complete) (struct mpxy_bridge_src_ctx *src,
				void *msgbuf, u32 msg_len);
};

int mpxy_bridge_register_source(const void *fdt, int nodeoff,
			    const struct mpxy_bridge_source_ops *ops,
			    struct mpxy_rpmi_service_data *srvcdata,
			    u32 num_services,
			    enum rpmi_servicegroup_id srvgrpid);

#endif /* __FDT_MPXY_BRIDGE_H__ */
