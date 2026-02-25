/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2026 Qualcomm Technologies, Inc.
 *
 * Authors:
 *   Ranbir Singh <ranbir.singh@oss.qualcomm.com>
 *   Sunil V L <sunilvl@oss.qualcomm.com>
 */

#include <sbi_utils/mpxy/fdt_mpxy_rpmi_mbox.h>
#include <sbi_utils/mailbox/rpmi_mailbox.h>

static struct rpmi_mm_get_attributes_rsp rsp;

static struct mpxy_rpmi_service_data mm_srvcdata[] = {
	[0] {
		.id = RPMI_MM_SRV_GET_ATTRIBUTES,
		.min_tx_len = 0,
		.max_tx_len = 0,
		.min_rx_len = sizeof(struct rpmi_mm_get_attributes_rsp),
		.max_rx_len = sizeof(struct rpmi_mm_get_attributes_rsp),
	},
	[1] {
		.id = RPMI_MM_SRV_COMMUNICATE,
		.min_tx_len = sizeof(struct rpmi_mm_communicate_req),
		.max_tx_len = sizeof(struct rpmi_mm_communicate_req),
		.min_rx_len = sizeof(struct rpmi_mm_communicate_rsp),
		.max_rx_len = sizeof(struct rpmi_mm_communicate_rsp),
	},
};

static int mpxy_rpmi_mm_setup(void **context, struct mbox_chan *chan,
			      const struct mpxy_rpmi_mbox_data *data)
{
	unsigned long mm_region_addr = 0;
	unsigned long mm_region_size = 0;
	unsigned long mm_region_flags;
	int rc = 0;

	rc = rpmi_normal_request_with_status(chan, RPMI_MM_SRV_GET_ATTRIBUTES,
					     NULL, 0, 0, &rsp,
					     rpmi_u32_count(rsp),
					     rpmi_u32_count(rsp));
	if (rc)
		return rc;

#if __riscv_xlen == 32
	mm_region_addr = rsp.mma.shmem_addr_lo;
#else
	mm_region_addr = ((unsigned long)(rsp.mma.shmem_addr_hi) << 32) |
	    rsp.mma.shmem_addr_lo;
#endif

	mm_region_size = rsp.mma.shmem_size;
	mm_region_flags = SBI_DOMAIN_MEMREGION_SHARED_SURW_MRW;

	rc = sbi_domain_root_add_memrange(mm_region_addr, mm_region_size,
					  PAGE_SIZE, mm_region_flags);
	return rc;
}

static int mpxy_rpmi_mm_xfer(void *context, struct mbox_chan *chan,
			     struct mbox_xfer *xfer)
{
	struct rpmi_message_args *args = xfer->args;
	int rc = 0;

	if (!xfer->rx || (args->type != RPMI_MSG_NORMAL_REQUEST))
		return 0;

	switch (args->service_id) {
	case RPMI_MM_SRV_GET_ATTRIBUTES:
		((u32 *)xfer->rx)[0] = cpu_to_le32(RPMI_SUCCESS);
		((u32 *)xfer->rx)[1] = cpu_to_le32(rsp.mma.mm_version);
		((u32 *)xfer->rx)[2] = cpu_to_le32(rsp.mma.shmem_addr_lo);
		((u32 *)xfer->rx)[3] = cpu_to_le32(rsp.mma.shmem_addr_hi);
		((u32 *)xfer->rx)[4] = cpu_to_le32(rsp.mma.shmem_size);
		args->rx_data_len = 5 * sizeof(u32);
		break;

	case RPMI_MM_SRV_COMMUNICATE:
		rc = mbox_chan_xfer(chan, xfer);
		break;

	default:
		((u32 *)xfer->rx)[0] = cpu_to_le32(RPMI_ERR_NOTSUPP);
		args->rx_data_len = sizeof(u32);
		break;
	};

	return rc;
}

static const struct mpxy_rpmi_mbox_data mm_data = {
	.servicegrp_id = RPMI_SRVGRP_MANAGEMENT_MODE,
	.num_services = RPMI_MM_SRV_MAX_COUNT,
	.service_data = mm_srvcdata,
	.setup_group = mpxy_rpmi_mm_setup,
	.xfer_group = mpxy_rpmi_mm_xfer,
};

/* one extra blank entry for loop termination while matching */
static const struct fdt_match mm_match[] = {
	{
		.compatible = "riscv,rpmi-mpxy-mm",
		.data = &mm_data,
	},
	{},
};

const struct fdt_driver fdt_mpxy_rpmi_mm = {
	.experimental = true,
	.match_table = mm_match,
	.init = mpxy_rpmi_mbox_init,
};
