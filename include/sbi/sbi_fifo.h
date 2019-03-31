/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Atish Patra<atish.patra@wdc.com>
 *
 */

#ifndef __SBI_FIFO_H__
#define __SBI_FIFO_H__

#include <sbi/riscv_locks.h>
#include <sbi/sbi_types.h>
#include <sbi/sbi_ipi.h>

struct sbi_fifo {
	int head;
	int tail;
	unsigned long maxsize;
	spinlock_t qlock;
	struct sbi_tlb_info queue[SBI_TLB_INFO_MAX_QUEUE_SIZE];
};

struct sbi_tlb_info * sbi_fifo_dequeue(struct sbi_fifo *fifo);
int sbi_fifo_enqueue(struct sbi_fifo *fifo, void *data);
void sbi_fifo_init(struct sbi_fifo *fifo, unsigned long msize);
bool sbi_fifo_is_empty(struct sbi_fifo *fifo);
bool sbi_fifo_is_full(struct sbi_fifo *fifo);
#endif
