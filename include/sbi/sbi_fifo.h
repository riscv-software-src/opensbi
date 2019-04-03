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

struct sbi_fifo {
	/* Static members of struct */
	void *queue;
	unsigned long entry_size;
	unsigned long num_entries;
	/* Dynamic members of struct protected by lock */
	spinlock_t qlock;
	unsigned long avail;
	unsigned long head;
	unsigned long tail;
};

int sbi_fifo_dequeue(struct sbi_fifo *fifo, void *data);
int sbi_fifo_enqueue(struct sbi_fifo *fifo, void *data);
void sbi_fifo_init(struct sbi_fifo *fifo, void *queue_mem,
		   unsigned long entries, unsigned long entry_size);
bool sbi_fifo_is_empty(struct sbi_fifo *fifo);
bool sbi_fifo_is_full(struct sbi_fifo *fifo);

#endif
