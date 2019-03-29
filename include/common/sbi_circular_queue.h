/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Atish Patra<atish.patra@wdc.com>
 *
 */

#ifndef __CQUEUE_H__
#define __CQUEUE_H__

#include <sbi/riscv_locks.h>
#include <sbi/sbi_types.h>
#include <sbi/sbi_ipi.h>

struct sbi_cqueue {
	int head;
	int tail;
	unsigned long maxsize;
	spinlock_t qlock;
	struct sbi_tlb_info queue[SBI_TLB_INFO_MAX_QUEUE_SIZE];
};

struct sbi_tlb_info * sbi_cq_dequeue(struct sbi_cqueue *cq);
int sbi_cq_enqueue(struct sbi_cqueue *cq, void *data);
void sbi_cq_init(struct sbi_cqueue *cq, unsigned long msize);
bool sbi_cq_is_empty(struct sbi_cqueue *cq);
bool sbi_cq_is_full(struct sbi_cqueue *cq);
#endif
