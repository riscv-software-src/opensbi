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
#include <sbi/sbi_hartmask.h>

/* clang-format off */

#define SBI_TLB_FLUSH_ALL			((unsigned long)-1)

/* clang-format on */

struct sbi_scratch;

enum sbi_tlb_type {
	SBI_TLB_FENCE_I = 0,
	SBI_TLB_SFENCE_VMA,
	SBI_TLB_SFENCE_VMA_ASID,
	SBI_TLB_HFENCE_GVMA_VMID,
	SBI_TLB_HFENCE_GVMA,
	SBI_TLB_HFENCE_VVMA_ASID,
	SBI_TLB_HFENCE_VVMA,
	SBI_TLB_TYPE_MAX,
};

struct sbi_tlb_info {
	unsigned long start;
	unsigned long size;
	uint16_t asid;
	uint16_t vmid;
	enum sbi_tlb_type type;
	struct sbi_hartmask smask;
};

#define SBI_TLB_INFO_INIT(__p, __start, __size, __asid, __vmid, __type, __src) \
do { \
	(__p)->start = (__start); \
	(__p)->size = (__size); \
	(__p)->asid = (__asid); \
	(__p)->vmid = (__vmid); \
	(__p)->type = (__type); \
	SBI_HARTMASK_INIT_EXCEPT(&(__p)->smask, (__src)); \
} while (0)

#define SBI_TLB_INFO_SIZE		sizeof(struct sbi_tlb_info)

/** Collection of Local TLB operations */
struct sbi_tlb_local_operations {
	/** Local fence.i */
	void (*local_fence_i)(struct sbi_tlb_info *tinfo);

	/** Local supervisor-mode TLB flush */
	void (*local_sfence_vma)(struct sbi_tlb_info *tinfo);

	/** Local supervisor-mode TLB flush with ASID */
	void (*local_sfence_vma_asid)(struct sbi_tlb_info *tinfo);

	/** Local G-stage TLB flush for a specific VMID */
	void (*local_hfence_gvma_vmid)(struct sbi_tlb_info *tinfo);

	/** Local G-stage TLB flush */
	void (*local_hfence_gvma)(struct sbi_tlb_info *tinfo);

	/** Local VS-stage TLB flush with ASID */
	void (*local_hfence_vvma_asid)(struct sbi_tlb_info *tinfo);

	/** Local VS-stage TLB flush */
	void (*local_hfence_vvma)(struct sbi_tlb_info *tinfo);
};

/** Get the local TLB operations */
const struct sbi_tlb_local_operations *sbi_tlb_get_local_operations(void);

/** Set the local TLB operations */
void sbi_tlb_set_local_operations(
		const struct sbi_tlb_local_operations *ops);

void __sbi_sfence_vma_all();

int sbi_tlb_request(ulong hmask, ulong hbase, struct sbi_tlb_info *tinfo);

int sbi_tlb_init(struct sbi_scratch *scratch, bool cold_boot);

#endif
