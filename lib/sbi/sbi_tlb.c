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
#include <sbi/riscv_atomic.h>
#include <sbi/riscv_barrier.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_fifo.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_tlb.h>
#include <sbi/sbi_string.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_platform.h>

static unsigned long tlb_sync_off;
static unsigned long tlb_fifo_off;
static unsigned long tlb_fifo_mem_off;
static unsigned long tlb_range_flush_limit;

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

static void sbi_tlb_local_flush(struct sbi_tlb_info *tinfo)
{
	if (tinfo->type == SBI_TLB_FLUSH_VMA) {
		sbi_tlb_fifo_sfence_vma(tinfo);
	} else if (tinfo->type == SBI_TLB_FLUSH_VMA_ASID) {
		sbi_tlb_fifo_sfence_vma_asid(tinfo);
	} else if (tinfo->type == SBI_ITLB_FLUSH)
		__asm__ __volatile("fence.i");
	else
		sbi_printf("Invalid tlb flush request type [%lu]\n",
			   tinfo->type);
	return;
}

static void sbi_tlb_entry_process(struct sbi_scratch *scratch,
				  struct sbi_tlb_info *tinfo)
{
	u32 i;
	u64 m;
	struct sbi_scratch *rscratch = NULL;
	unsigned long *rtlb_sync = NULL;

	sbi_tlb_local_flush(tinfo);
	for (i = 0, m = tinfo->shart_mask; m; i++, m >>= 1) {
		if (!(m & 1UL))
			continue;

		rscratch = sbi_hart_id_to_scratch(scratch, i);
		rtlb_sync = sbi_scratch_offset_ptr(rscratch, tlb_sync_off);
		while (atomic_raw_xchg_ulong(rtlb_sync, 1)) ;
	}
}

static void sbi_tlb_fifo_process_count(struct sbi_scratch *scratch, int count)
{
	struct sbi_tlb_info tinfo;
	u32 deq_count = 0;
	struct sbi_fifo *tlb_fifo =
			sbi_scratch_offset_ptr(scratch, tlb_fifo_off);

	while (!sbi_fifo_dequeue(tlb_fifo, &tinfo)) {
		sbi_tlb_entry_process(scratch, &tinfo);
		deq_count++;
		if (deq_count > count)
			break;

	}
}

void sbi_tlb_fifo_process(struct sbi_scratch *scratch)
{
	struct sbi_tlb_info tinfo;
	struct sbi_fifo *tlb_fifo =
			sbi_scratch_offset_ptr(scratch, tlb_fifo_off);

	while (!sbi_fifo_dequeue(tlb_fifo, &tinfo))
		sbi_tlb_entry_process(scratch, &tinfo);
}

void sbi_tlb_fifo_sync(struct sbi_scratch *scratch)
{
	unsigned long *tlb_sync =
			sbi_scratch_offset_ptr(scratch, tlb_sync_off);

	while (!atomic_raw_xchg_ulong(tlb_sync, 0)) {
		/*
		 * While we are waiting for remote hart to set the sync,
		 * consume fifo requests to avoid deadlock.
		 */
		sbi_tlb_fifo_process_count(scratch, 1);
	}

	return;
}

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
		curr->shart_mask = curr->shart_mask | next->shart_mask;
		ret	    = SBI_FIFO_UPDATED;
	} else if (next->start >= curr->start && next_end <= curr_end) {
		curr->shart_mask = curr->shart_mask | next->shart_mask;
		ret		 = SBI_FIFO_SKIP;
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
 *
 * Note:
 *	We can not issue a fifo reset anymore if a complete vma flush is requested.
 *	This is because we are queueing FENCE.I requests as well now.
 *	To ease up the pressure in enqueue/fifo sync path, try to dequeue 1 element
 *	before continuing the while loop. This method is preferred over wfi/ipi because
 *	of MMIO cost involved in later method.
 */
static int sbi_tlb_fifo_update_cb(void *in, void *data)
{
	struct sbi_tlb_info *curr;
	struct sbi_tlb_info *next;
	int ret = SBI_FIFO_UNCHANGED;

	if (!in || !data)
		return ret;

	curr = (struct sbi_tlb_info *)data;
	next = (struct sbi_tlb_info *)in;

	if (next->type == SBI_TLB_FLUSH_VMA_ASID &&
	    curr->type == SBI_TLB_FLUSH_VMA_ASID) {
		if (next->asid == curr->asid)
			ret = __sbi_tlb_fifo_range_check(curr, next);
	} else if (next->type == SBI_TLB_FLUSH_VMA &&
		   curr->type == SBI_TLB_FLUSH_VMA) {
			ret = __sbi_tlb_fifo_range_check(curr, next);
	}

	return ret;
}

int sbi_tlb_fifo_update(struct sbi_scratch *rscratch, u32 hartid, void *data)
{
	int ret;
	struct sbi_fifo *tlb_fifo_r;
	struct sbi_scratch *lscratch;
	struct sbi_tlb_info *tinfo = data;
	u32 curr_hartid = sbi_current_hartid();

	/*
	 * If address range to flush is too big then simply
	 * upgrade it to flush all because we can only flush
	 * 4KB at a time.
	 */
	if (tinfo->size > tlb_range_flush_limit) {
		tinfo->start = 0;
		tinfo->size = SBI_TLB_FLUSH_ALL;
	}

	/*
	 * If the request is to queue a tlb flush entry for itself
	 * then just do a local flush and return;
	 */
	if (hartid == curr_hartid) {
		sbi_tlb_local_flush(tinfo);
		return -1;
	}

	lscratch = sbi_hart_id_to_scratch(rscratch, curr_hartid);
	tlb_fifo_r = sbi_scratch_offset_ptr(rscratch, tlb_fifo_off);

	ret = sbi_fifo_inplace_update(tlb_fifo_r, data, sbi_tlb_fifo_update_cb);
	if (ret != SBI_FIFO_UNCHANGED) {
		return 1;
	}

	while (sbi_fifo_enqueue(tlb_fifo_r, data) < 0) {
		/**
		 * For now, Busy loop until there is space in the fifo.
		 * There may be case where target hart is also
		 * enqueue in source hart's fifo. Both hart may busy
		 * loop leading to a deadlock.
		 * TODO: Introduce a wait/wakeup event mechanism to handle
		 * this properly.
		 */
		sbi_tlb_fifo_process_count(lscratch, 1);
		sbi_dprintf(rscratch, "hart%d: hart%d tlb fifo full\n",
			    curr_hartid, hartid);
	}

	return 0;
}

int sbi_tlb_fifo_init(struct sbi_scratch *scratch, bool cold_boot)
{
	void *tlb_mem;
	unsigned long *tlb_sync;
	struct sbi_fifo *tlb_q;
	const struct sbi_platform *plat = sbi_platform_ptr(scratch);

	if (cold_boot) {
		tlb_sync_off = sbi_scratch_alloc_offset(sizeof(*tlb_sync),
							    "IPI_TLB_SYNC");
		if (!tlb_sync_off)
			return SBI_ENOMEM;
		tlb_fifo_off = sbi_scratch_alloc_offset(sizeof(*tlb_q),
							    "IPI_TLB_FIFO");
		if (!tlb_fifo_off) {
			sbi_scratch_free_offset(tlb_sync_off);
			return SBI_ENOMEM;
		}
		tlb_fifo_mem_off = sbi_scratch_alloc_offset(
				SBI_TLB_FIFO_NUM_ENTRIES * SBI_TLB_INFO_SIZE,
				"IPI_TLB_FIFO_MEM");
		if (!tlb_fifo_mem_off) {
			sbi_scratch_free_offset(tlb_fifo_off);
			sbi_scratch_free_offset(tlb_sync_off);
			return SBI_ENOMEM;
		}
		tlb_range_flush_limit = sbi_platform_tlbr_flush_limit(plat);
	} else {
		if (!tlb_sync_off ||
		    !tlb_fifo_off ||
		    !tlb_fifo_mem_off)
			return SBI_ENOMEM;
	}

	tlb_sync = sbi_scratch_offset_ptr(scratch, tlb_sync_off);
	tlb_q = sbi_scratch_offset_ptr(scratch, tlb_fifo_off);
	tlb_mem = sbi_scratch_offset_ptr(scratch, tlb_fifo_mem_off);

	*tlb_sync = 0;

	sbi_fifo_init(tlb_q, tlb_mem,
		      SBI_TLB_FIFO_NUM_ENTRIES, SBI_TLB_INFO_SIZE);

	return 0;
}
