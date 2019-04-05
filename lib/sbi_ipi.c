/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 *   Nick Kossifidis <mick@ics.forth.gr>
 */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_barrier.h>
#include <sbi/riscv_atomic.h>
#include <sbi/riscv_unpriv.h>
#include <sbi/sbi_fifo.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_bitops.h>
#include <sbi/sbi_ipi.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_timer.h>
#include <plat/string.h>

static inline int __sbi_tlb_fifo_range_check(struct sbi_tlb_info *curr,
					     struct sbi_tlb_info *next)
{
	int curr_end;
	int next_end;
	int ret = SBI_FIFO_UNCHANGED;

	if (!curr || !next)
		return ret;

	next_end = next->start + next->size;
	curr_end = curr->start + curr->size;
	if (next->start <= curr->start && next_end > curr_end) {
		curr->start = next->start;
		curr->size = next->size;
		ret = SBI_FIFO_UPDATED;
	} else if (next->start >= curr->start &&
		   next_end <= curr_end) {
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
int sbi_tlb_fifo_update_cb(void *in, void *data)
{
	struct sbi_tlb_info *curr;
	struct sbi_tlb_info *next;
	int ret = SBI_FIFO_UNCHANGED;

	if (!in && !!data)
		return ret;

	curr = (struct sbi_tlb_info *) data;
	next = (struct sbi_tlb_info *) in;
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

static int sbi_ipi_send(struct sbi_scratch *scratch, u32 hartid,
			u32 event, void *data)
{
	struct sbi_scratch *remote_scratch = NULL;
	const struct sbi_platform *plat = sbi_platform_ptr(scratch);
	struct sbi_fifo *ipi_tlb_fifo;
	int ret = SBI_FIFO_UNCHANGED;

	if (sbi_platform_hart_disabled(plat, hartid))
		return -1;

	/* Set IPI type on remote hart's scratch area and
	 * trigger the interrupt
	 */
	remote_scratch = sbi_hart_id_to_scratch(scratch, hartid);
	if (event == SBI_IPI_EVENT_SFENCE_VMA ||
	    event == SBI_IPI_EVENT_SFENCE_VMA_ASID) {
		ipi_tlb_fifo = sbi_tlb_fifo_head_ptr(remote_scratch);
		ret = sbi_fifo_inplace_update(ipi_tlb_fifo, data,
					   sbi_tlb_fifo_update_cb);
		if (ret == SBI_FIFO_SKIP || ret == SBI_FIFO_UPDATED) {
			goto done;
		}
		while(sbi_fifo_enqueue(ipi_tlb_fifo, data) < 0) {
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
	}
	atomic_raw_set_bit(event, &sbi_ipi_data_ptr(remote_scratch)->ipi_type);
	mb();
	sbi_platform_ipi_send(plat, hartid);
	if (event != SBI_IPI_EVENT_SOFT)
		sbi_platform_ipi_sync(plat, hartid);
done:

	return 0;
}

int sbi_ipi_send_many(struct sbi_scratch *scratch,
		      ulong *pmask, u32 event, void *data)
{
	ulong i, m;
	ulong mask = sbi_hart_available_mask();
	u32 hartid = sbi_current_hartid();

	if (pmask)
		mask &= load_ulong(pmask);

	/* send IPIs to every other hart on the set */
	for (i = 0, m = mask; m; i++, m >>= 1)
		if ((m & 1UL) && (i != hartid))
			sbi_ipi_send(scratch, i, event, data);

	/* If the current hart is on the set, send an IPI
	 * to it as well
	 */
	if (mask & (1UL << hartid))
		sbi_ipi_send(scratch, hartid, event, data);


	return 0;
}

void sbi_ipi_clear_smode(struct sbi_scratch *scratch)
{
	csr_clear(CSR_MIP, MIP_SSIP);
}

static void sbi_ipi_tlb_flush_all()
{
	__asm__ __volatile("sfence.vma");
}

static void sbi_ipi_sfence_vma(struct sbi_tlb_info *tinfo)
{
	unsigned long start = tinfo->start;
	unsigned long size = tinfo->size;
	unsigned long i;

	if ((start == 0 && size == 0) || (size == SBI_TLB_FLUSH_ALL)) {
		sbi_ipi_tlb_flush_all();
		return;
	}

	for (i = 0; i < size; i += PAGE_SIZE) {
		__asm__ __volatile__ ("sfence.vma %0"
				      : : "r" (start + i)
				      : "memory");
	}
}

static void sbi_ipi_sfence_vma_asid(struct sbi_tlb_info *tinfo)
{
	unsigned long start = tinfo->start;
	unsigned long size = tinfo->size;
	unsigned long asid = tinfo->asid;
	unsigned long i;

	if (start == 0 && size == 0) {
		sbi_ipi_tlb_flush_all();
		return;
	}

	/* Flush entire MM context for a given ASID */
	if (size == SBI_TLB_FLUSH_ALL) {
		__asm__ __volatile__ ("sfence.vma x0, %0"
				       : : "r" (asid)
				       : "memory");
		return;
	}

	for (i = 0; i < size; i += PAGE_SIZE) {
		__asm__ __volatile__ ("sfence.vma %0, %1"
				      : : "r" (start + i), "r" (asid)
				      : "memory");
	}
}

void sbi_ipi_process(struct sbi_scratch *scratch)
{
	volatile unsigned long ipi_type;
	struct sbi_tlb_info tinfo;
	unsigned int ipi_event;
	const struct sbi_platform *plat = sbi_platform_ptr(scratch);
	struct sbi_fifo *ipi_tlb_fifo = sbi_tlb_fifo_head_ptr(scratch);

	u32 hartid = sbi_current_hartid();
	sbi_platform_ipi_clear(plat, hartid);

	do {
		ipi_type = sbi_ipi_data_ptr(scratch)->ipi_type;
		rmb();
		ipi_event = __ffs(ipi_type);
		switch (ipi_event) {
		case SBI_IPI_EVENT_SOFT:
			csr_set(CSR_MIP, MIP_SSIP);
			break;
		case SBI_IPI_EVENT_FENCE_I:
			__asm__ __volatile("fence.i");
			break;
		case SBI_IPI_EVENT_SFENCE_VMA:
		case SBI_IPI_EVENT_SFENCE_VMA_ASID:
			while(!sbi_fifo_dequeue(ipi_tlb_fifo, &tinfo)) {
				if (tinfo.type == SBI_TLB_FLUSH_VMA)
					sbi_ipi_sfence_vma(&tinfo);
				else if (tinfo.type == SBI_TLB_FLUSH_VMA_ASID)
					sbi_ipi_sfence_vma_asid(&tinfo);
				memset(&tinfo, 0, SBI_TLB_INFO_SIZE);
			}
			break;
		case SBI_IPI_EVENT_HALT:
			sbi_hart_hang();
			break;
		};
		ipi_type = atomic_raw_clear_bit(ipi_event, &sbi_ipi_data_ptr(scratch)->ipi_type);
	} while(ipi_type > 0);
}

int sbi_ipi_init(struct sbi_scratch *scratch, bool cold_boot)
{
	struct sbi_fifo *tlb_info_q = sbi_tlb_fifo_head_ptr(scratch);

	sbi_ipi_data_ptr(scratch)->ipi_type = 0x00;
	sbi_fifo_init(tlb_info_q, sbi_tlb_fifo_mem_ptr(scratch),
		      SBI_TLB_FIFO_NUM_ENTRIES, SBI_TLB_INFO_SIZE);

	/* Enable software interrupts */
	csr_set(CSR_MIE, MIP_MSIP);

	return sbi_platform_ipi_init(sbi_platform_ptr(scratch),
				     cold_boot);
}
