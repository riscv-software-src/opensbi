/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Atish Patra <atish.patra@wdc.com>
 *   Anup Patel <anup.patel@wdc.com>
 */

#ifndef __SBI_TLB_H__
#define __SBI_TLB_H__

#include <sbi/sbi_types.h>

/* clang-format off */

#define SBI_TLB_FLUSH_ALL			((unsigned long)-1)

/* clang-format on */

#define SBI_TLB_FIFO_NUM_ENTRIES		8

enum sbi_tlb_info_types {
	SBI_TLB_FLUSH_VMA,
	SBI_TLB_FLUSH_VMA_ASID,
	SBI_TLB_FLUSH_GVMA,
	SBI_TLB_FLUSH_GVMA_VMID,
	SBI_TLB_FLUSH_VVMA,
	SBI_TLB_FLUSH_VVMA_ASID,
	SBI_ITLB_FLUSH
};

struct sbi_scratch;

struct sbi_tlb_info {
	unsigned long start;
	unsigned long size;
	unsigned long asid;
	unsigned long type;
	unsigned long shart_mask;
};

#define SBI_TLB_INFO_SIZE			sizeof(struct sbi_tlb_info)

int sbi_tlb_request(struct sbi_scratch *scratch, ulong hmask,
		    ulong hbase, struct sbi_tlb_info *tinfo);

int sbi_tlb_init(struct sbi_scratch *scratch, bool cold_boot);

#endif
