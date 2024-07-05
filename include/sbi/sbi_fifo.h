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
	void *queue;
	spinlock_t qlock;
	u16 entry_size;
	u16 num_entries;
	u16 avail;
	u16 tail;
};

#define SBI_FIFO_INITIALIZER(__queue_mem, __entries, __entry_size)	\
{	.queue = __queue_mem,						\
	.qlock = SPIN_LOCK_INITIALIZER,					\
	.num_entries = __entries,					\
	.entry_size = __entry_size,					\
	.avail = 0,							\
	.tail = 0,							\
}

#define SBI_FIFO_DEFINE(__name, __queue_mem, __entries, __entry_size)	\
struct sbi_fifo __name = SBI_FIFO_INITIALIZER(__queue_mem, __entries, __entry_size)

enum sbi_fifo_inplace_update_types {
	SBI_FIFO_SKIP,
	SBI_FIFO_UPDATED,
	SBI_FIFO_UNCHANGED,
};

int sbi_fifo_dequeue(struct sbi_fifo *fifo, void *data);
int sbi_fifo_enqueue(struct sbi_fifo *fifo, void *data, bool force);
void sbi_fifo_init(struct sbi_fifo *fifo, void *queue_mem, u16 entries,
		   u16 entry_size);
int sbi_fifo_is_empty(struct sbi_fifo *fifo);
int sbi_fifo_is_full(struct sbi_fifo *fifo);
int sbi_fifo_inplace_update(struct sbi_fifo *fifo, void *in,
			    int (*fptr)(void *in, void *data));
u16 sbi_fifo_avail(struct sbi_fifo *fifo);

#endif
