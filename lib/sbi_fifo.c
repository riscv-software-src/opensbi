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
#include <sbi/sbi_fifo.h>
#include <plat/string.h>

void sbi_fifo_init(struct sbi_fifo *fifo, unsigned long entries,
		   unsigned long size)
{
	fifo->head = -1;
	fifo->tail = -1;
	fifo->num_entries = entries;
	fifo->entrysize = size;
	SPIN_LOCK_INIT(&fifo->qlock);
	memset(fifo->queue, 0, entries * size);
}

bool sbi_fifo_is_full(struct sbi_fifo *fifo)
{
	if (((fifo->head == fifo->num_entries-1) && fifo->tail == 0) ||
	     (fifo->tail == fifo->head + 1)) {
		return TRUE;
	}
	return FALSE;
}

bool sbi_fifo_is_empty(struct sbi_fifo *fifo)
{
	if (fifo->head == -1)
		return TRUE;
	return FALSE;
}

int sbi_fifo_enqueue(struct sbi_fifo *fifo, void *data)
{
	if (!fifo || !data)
		return -1;

	spin_lock(&fifo->qlock);
	if (sbi_fifo_is_full(fifo)) {
		spin_unlock(&fifo->qlock);
		return -1;
	}
	if (fifo->tail == -1)
		fifo->tail = 0;
	fifo->head = (fifo->head + 1) % fifo->num_entries;
	memcpy(fifo->queue + fifo->head * fifo->entrysize, data,
	       fifo->entrysize);
	spin_unlock(&fifo->qlock);

	return 0;
}

int sbi_fifo_dequeue(struct sbi_fifo *fifo, void *data)
{
	if (!fifo || !data)
		return -1;

	spin_lock(&fifo->qlock);
	if (sbi_fifo_is_empty(fifo)) {
		spin_unlock(&fifo->qlock);
		return -1;
	}
	memcpy(data, fifo->queue + fifo->tail * fifo->entrysize,
	       fifo->entrysize);

	if (fifo->tail == fifo->head) {
		fifo->tail = -1;
		fifo->head = -1;
	} else {
		fifo->tail = (fifo->tail + 1) % fifo->num_entries;
	}
	spin_unlock(&fifo->qlock);

	return 0;
}
