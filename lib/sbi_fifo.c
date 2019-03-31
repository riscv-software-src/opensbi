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

void sbi_fifo_init(struct sbi_fifo *fifo, unsigned long msize)
{
	int i;
	struct sbi_tlb_info tlb_info_init = {0};

	fifo->head = -1;
	fifo->tail = -1;
	fifo->maxsize = msize;
	SPIN_LOCK_INIT(&fifo->qlock);

	for (i = 0; i < SBI_TLB_INFO_MAX_QUEUE_SIZE; i++) {
		sbi_tlb_info_copy(&fifo->queue[i], &tlb_info_init);
	}
}
bool sbi_fifo_is_full(struct sbi_fifo *fifo)
{
	if (((fifo->head == SBI_TLB_INFO_MAX_QUEUE_SIZE-1) && fifo->tail == 0) ||
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
	if (!fifo)
		return -1;
	spin_lock(&fifo->qlock);
	if (sbi_fifo_is_full(fifo)) {
		spin_unlock(&fifo->qlock);
		return -1;
	}
	if (fifo->tail == -1)
		fifo->tail = 0;
	fifo->head = (fifo->head + 1) % fifo->maxsize;
	sbi_tlb_info_copy(&fifo->queue[fifo->head], data);
	spin_unlock(&fifo->qlock);

	return 0;
}

struct sbi_tlb_info * sbi_fifo_dequeue(struct sbi_fifo *fifo)
{
	struct sbi_tlb_info *tinfo;

	if (!fifo)
		return NULL;

	spin_lock(&fifo->qlock);
	if (sbi_fifo_is_empty(fifo)) {
		spin_unlock(&fifo->qlock);
		return NULL;
	}
	tinfo = &fifo->queue[fifo->tail];

	if (fifo->tail == fifo->head) {
		fifo->tail = -1;
		fifo->head = -1;
	} else {
		fifo->tail = (fifo->tail + 1) % fifo->maxsize;
	}
	spin_unlock(&fifo->qlock);

	return tinfo;
}
