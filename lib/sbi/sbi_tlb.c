/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Atish Patra <atish.patra@wdc.com>
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_barrier.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_fifo.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_bitops.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_tlb.h>
#include <sbi/sbi_string.h>

static unsigned long ipi_tlb_fifo_off;
static unsigned long ipi_tlb_fifo_mem_off;

static inline int __sbi_tlb_fifo_range_check(struct sbi_tlb_info *curr,
					     struct sbi_tlb_info *next)
{
	unsigned long curr_end;
	unsigned long next_end;
	int ret = SBI_FIFO_UNCHANGED;

	if (!curr || !next)
		return ret;

	next_end = next->start + next->size;
	curr_end = curr->start + curr->size;
	if (next->start <= curr->start && next_end > curr_end) {
		curr->start = next->start;
		curr->size  = next->size;
		ret	    = SBI_FIFO_UPDATED;
	} else if (next->start >= curr->start && next_end <= curr_end) {
		ret = SBI_FIFO_SKIP;
	}

	return ret;
}

/**
 * Call back to decide if an inplace fifo update is required or next entry can
 * can be skipped. Here are the different cases that are being handled.
 *
 * Case1:
 *	if next flush request range lies within one of the existing entry, skip
 *	the next entry.
 * Case2:
 *	if flush request range in current fifo entry lies within next flush
 *	request, update the current entry.
 * Case3:
	if a complete vma flush is requested, then all entries can be deleted
	and new request can be enqueued. This will not be done for ASID case
	as that means we have to iterate again in the fifo to figure out which
	entries belong to that ASID.
 */
static int sbi_tlb_fifo_update_cb(void *in, void *data)
{
	struct sbi_tlb_info *curr;
	struct sbi_tlb_info *next;
	int ret = SBI_FIFO_UNCHANGED;

	if (!in && !!data)
		return ret;

	curr = (struct sbi_tlb_info *)data;
	next = (struct sbi_tlb_info *)in;
	if (next->type == SBI_TLB_FLUSH_VMA_ASID &&
	    curr->type == SBI_TLB_FLUSH_VMA_ASID) {
		if (next->asid == curr->asid)
			ret = __sbi_tlb_fifo_range_check(curr, next);
	} else if (next->type == SBI_TLB_FLUSH_VMA &&
		   curr->type == SBI_TLB_FLUSH_VMA) {
		if (next->size == SBI_TLB_FLUSH_ALL)
			ret = SBI_FIFO_RESET;
		else
			ret = __sbi_tlb_fifo_range_check(curr, next);
	}

	return ret;
}

int sbi_tlb_fifo_update(struct sbi_scratch *scratch, u32 event, void *data)
{
	int ret;
	struct sbi_fifo *ipi_tlb_fifo;
	struct sbi_tlb_info *tinfo = data;

	ipi_tlb_fifo = sbi_scratch_offset_ptr(scratch,
					      ipi_tlb_fifo_off);
	/*
	 * If address range to flush is too big then simply
	 * upgrade it to flush all because we can only flush
	 * 4KB at a time.
	 */
	if (tinfo->size >= SBI_TLB_FLUSH_MAX_SIZE) {
		tinfo->start = 0;
		tinfo->size = SBI_TLB_FLUSH_ALL;
	}

	ret = sbi_fifo_inplace_update(ipi_tlb_fifo, data,
				      sbi_tlb_fifo_update_cb);
	if (ret == SBI_FIFO_SKIP || ret == SBI_FIFO_UPDATED) {
		return 1;
	}

	while (sbi_fifo_enqueue(ipi_tlb_fifo, data) < 0) {
		/**
		 * For now, Busy loop until there is space in the fifo.
		 * There may be case where target hart is also
		 * enqueue in source hart's fifo. Both hart may busy
		 * loop leading to a deadlock.
		 * TODO: Introduce a wait/wakeup event mechansim to handle
		 * this properly.
		 */
		__asm__ __volatile("nop");
		__asm__ __volatile("nop");
	}

	return 0;
}

static void sbi_tlb_flush_all(void)
{
	__asm__ __volatile("sfence.vma");
}

static void sbi_tlb_fifo_sfence_vma(struct sbi_tlb_info *tinfo)
{
	unsigned long start = tinfo->start;
	unsigned long size  = tinfo->size;
	unsigned long i;

	if ((start == 0 && size == 0) || (size == SBI_TLB_FLUSH_ALL)) {
		sbi_tlb_flush_all();
		return;
	}

	for (i = 0; i < size; i += PAGE_SIZE) {
		__asm__ __volatile__("sfence.vma %0"
				     :
				     : "r"(start + i)
				     : "memory");
	}
}

static void sbi_tlb_fifo_sfence_vma_asid(struct sbi_tlb_info *tinfo)
{
	unsigned long start = tinfo->start;
	unsigned long size  = tinfo->size;
	unsigned long asid  = tinfo->asid;
	unsigned long i;

	if (start == 0 && size == 0) {
		sbi_tlb_flush_all();
		return;
	}

	/* Flush entire MM context for a given ASID */
	if (size == SBI_TLB_FLUSH_ALL) {
		__asm__ __volatile__("sfence.vma x0, %0"
				     :
				     : "r"(asid)
				     : "memory");
		return;
	}

	for (i = 0; i < size; i += PAGE_SIZE) {
		__asm__ __volatile__("sfence.vma %0, %1"
				     :
				     : "r"(start + i), "r"(asid)
				     : "memory");
	}
}

void sbi_tlb_fifo_process(struct sbi_scratch *scratch, u32 event)
{
	struct sbi_tlb_info tinfo;
	struct sbi_fifo *ipi_tlb_fifo =
			sbi_scratch_offset_ptr(scratch, ipi_tlb_fifo_off);

	while (!sbi_fifo_dequeue(ipi_tlb_fifo, &tinfo)) {
		if (tinfo.type == SBI_TLB_FLUSH_VMA)
			sbi_tlb_fifo_sfence_vma(&tinfo);
		else if (tinfo.type == SBI_TLB_FLUSH_VMA_ASID)
			sbi_tlb_fifo_sfence_vma_asid(&tinfo);
		sbi_memset(&tinfo, 0, SBI_TLB_INFO_SIZE);
	}
}

int sbi_tlb_fifo_init(struct sbi_scratch *scratch, bool cold_boot)
{
	void *ipi_tlb_mem;
	struct sbi_fifo *ipi_tlb_q;

	if (cold_boot) {
		ipi_tlb_fifo_off = sbi_scratch_alloc_offset(sizeof(*ipi_tlb_q),
							    "IPI_TLB_FIFO");
		if (!ipi_tlb_fifo_off)
			return SBI_ENOMEM;
		ipi_tlb_fifo_mem_off = sbi_scratch_alloc_offset(
				SBI_TLB_FIFO_NUM_ENTRIES * SBI_TLB_INFO_SIZE,
				"IPI_TLB_FIFO_MEM");
		if (!ipi_tlb_fifo_mem_off) {
			sbi_scratch_free_offset(ipi_tlb_fifo_off);
			return SBI_ENOMEM;
		}
	} else {
		if (!ipi_tlb_fifo_off ||
		    !ipi_tlb_fifo_mem_off)
			return SBI_ENOMEM;
	}

	ipi_tlb_q = sbi_scratch_offset_ptr(scratch, ipi_tlb_fifo_off);
	ipi_tlb_mem = sbi_scratch_offset_ptr(scratch, ipi_tlb_fifo_mem_off);

	sbi_fifo_init(ipi_tlb_q, ipi_tlb_mem,
		      SBI_TLB_FIFO_NUM_ENTRIES, SBI_TLB_INFO_SIZE);

	return 0;
}
