/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Atish Patra<atish.patra@wdc.com>
 *
 */
#include <sbi/riscv_locks.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_fifo.h>
#include <plat/string.h>

void sbi_fifo_init(struct sbi_fifo *fifo, void *queue_mem,
		   unsigned long entries, unsigned long entry_size)
{
	fifo->queue = queue_mem;
	fifo->num_entries = entries;
	fifo->entry_size = entry_size;
	SPIN_LOCK_INIT(&fifo->qlock);
	fifo->avail = fifo->head = fifo->tail = 0;
	memset(fifo->queue, 0, entries * entry_size);
}

/* Note: must be called with fifo->qlock held */
static inline bool __sbi_fifo_is_full(struct sbi_fifo *fifo)
{
	return (fifo->avail == fifo->num_entries) ? TRUE : FALSE;
}

bool sbi_fifo_is_full(struct sbi_fifo *fifo)
{
	bool ret;

	spin_lock(&fifo->qlock);
	ret = __sbi_fifo_is_full(fifo);
	spin_unlock(&fifo->qlock);

	return ret;
}

/* Note: must be called with fifo->qlock held */
static inline bool __sbi_fifo_is_empty(struct sbi_fifo *fifo)
{
	return (fifo->avail == 0) ? TRUE : FALSE;
}

bool sbi_fifo_is_empty(struct sbi_fifo *fifo)
{
	bool ret;

	spin_lock(&fifo->qlock);
	ret = __sbi_fifo_is_empty(fifo);
	spin_unlock(&fifo->qlock);

	return ret;
}

int sbi_fifo_enqueue(struct sbi_fifo *fifo, void *data)
{
	if (!fifo || !data)
		return SBI_EINVAL;

	spin_lock(&fifo->qlock);

	if (__sbi_fifo_is_full(fifo)) {
		spin_unlock(&fifo->qlock);
		return SBI_ENOSPC;
	}

	memcpy(fifo->queue + fifo->head * fifo->entry_size, data,
	       fifo->entry_size);

	fifo->avail++;
	fifo->head++;
	if (fifo->head >= fifo->num_entries)
		fifo->head = 0;

	spin_unlock(&fifo->qlock);

	return 0;
}

int sbi_fifo_dequeue(struct sbi_fifo *fifo, void *data)
{
	if (!fifo || !data)
		return SBI_EINVAL;

	spin_lock(&fifo->qlock);

	if (__sbi_fifo_is_empty(fifo)) {
		spin_unlock(&fifo->qlock);
		return SBI_ENOENT;
	}

	memcpy(data, fifo->queue + fifo->tail * fifo->entry_size,
	       fifo->entry_size);

	fifo->avail--;
	fifo->tail++;
	if (fifo->tail >= fifo->num_entries)
		fifo->tail = 0;

	spin_unlock(&fifo->qlock);

	return 0;
}
