/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Atish Patra<atish.patra@wdc.com>
 *
 */
#include <common/sbi_circular_queue.h>
#include <sbi/riscv_locks.h>

void sbi_cq_init(struct sbi_cqueue *cq, unsigned long msize)
{
	cq->head = -1;
	cq->tail = -1;
	cq->maxsize = msize;
	SPIN_LOCK_INIT(&cq->qlock);
}

bool sbi_cq_is_full(struct sbi_cqueue *cq)
{
	if (((cq->head == SBI_TLB_INFO_MAX_QUEUE_SIZE-1) && cq->tail == 0) ||
	     (cq->tail == cq->head + 1)) {
		return TRUE;
	}
	return FALSE;
}

bool sbi_cq_is_empty(struct sbi_cqueue *cq)
{
	if (cq->head == -1)
		return TRUE;
	return FALSE;
}

int sbi_cq_enqueue(struct sbi_cqueue *cq, void *data)
{
	if (!cq)
		return -1;
	spin_lock(&cq->qlock);
	if (sbi_cq_is_full(cq)) {
		spin_unlock(&cq->qlock);
		return -1;
	}
	if (cq->tail == -1)
		cq->tail = 0;
	cq->head = (cq->head + 1) % cq->maxsize;
	sbi_tlb_info_copy(&cq->queue[cq->head], data);
	spin_unlock(&cq->qlock);

	return 0;
}

struct sbi_tlb_info * sbi_cq_dequeue(struct sbi_cqueue *cq)
{
	struct sbi_tlb_info *tinfo;

	if (!cq)
		return NULL;

	spin_lock(&cq->qlock);
	if (sbi_cq_is_empty(cq)) {
		spin_unlock(&cq->qlock);
		return NULL;
	}
	tinfo = &cq->queue[cq->tail];

	if (cq->tail == cq->head) {
		cq->tail = -1;
		cq->head = -1;
	} else {
		cq->tail = (cq->tail + 1) % cq->maxsize;
	}
	spin_unlock(&cq->qlock);

	return tinfo;
}
