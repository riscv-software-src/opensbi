/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 *   Nick Kossifidis <mick@ics.forth.gr>
 */

#include <common/sbi_circular_queue.h>
#include <sbi/riscv_asm.h>
#include <sbi/riscv_barrier.h>
#include <sbi/riscv_atomic.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_bitops.h>
#include <sbi/sbi_ipi.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_timer.h>
#include <sbi/sbi_unpriv.h>

static int sbi_ipi_send(struct sbi_scratch *scratch, u32 hartid,
			u32 event, void *data)
{
	struct sbi_scratch *remote_scratch = NULL;
	const struct sbi_platform *plat = sbi_platform_ptr(scratch);
	struct sbi_cqueue *ipi_tlb_queue;
	int ret;

	if (sbi_platform_hart_disabled(plat, hartid))
		return -1;

	/* Set IPI type on remote hart's scratch area and
	 * trigger the interrupt
	 */
	remote_scratch = sbi_hart_id_to_scratch(scratch, hartid);
	if (event == SBI_IPI_EVENT_SFENCE_VMA ||
	    event == SBI_IPI_EVENT_SFENCE_VMA_ASID) {
		ipi_tlb_queue = sbi_tlb_info_queue_ptr(remote_scratch);
		ret = sbi_cq_enqueue(ipi_tlb_queue, data);
		while (ret < 0) {
			/**
			 * For now, Busy loop until there is space in the queue.
			 * There may be case where target hart is also
			 * enqueue in source hart's queue. Both hart may busy
			 * loop leading to a deadlock.
			 * TODO: Introduce a wait/wakeup event mechansim to handle
			 * this properly.
			 */
			__asm__ __volatile("nop");
			__asm__ __volatile("nop");
			ret = sbi_cq_enqueue(ipi_tlb_queue, data);
		}
	}
	atomic_raw_set_bit(event, &sbi_ipi_data_ptr(remote_scratch)->ipi_type);
	mb();
	sbi_platform_ipi_send(plat, hartid);
	if (event != SBI_IPI_EVENT_SOFT)
		sbi_platform_ipi_sync(plat, hartid);

	return 0;
}

int sbi_ipi_send_many(struct sbi_scratch *scratch,
		      ulong *pmask, u32 event, void *data)
{
	ulong i, m;
	ulong mask = sbi_hart_available_mask();
	u32 hartid = sbi_current_hartid();

	if (pmask)
		mask &= load_ulong(pmask, csr_read(CSR_MEPC));

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
	if (!tinfo)
		return;

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
	if (!tinfo)
		return;

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
	struct sbi_tlb_info *tinfo;
	unsigned int ipi_event;
	const struct sbi_platform *plat = sbi_platform_ptr(scratch);
	struct sbi_cqueue *ipi_tlb_queue = sbi_tlb_info_queue_ptr(scratch);

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
			/**
			 *TODO: We clear the queue here. But event bit is not
			 *cleared from ipi_data.
			 */
			do {
				tinfo = sbi_cq_dequeue(ipi_tlb_queue);
				if (!tinfo) {
					if (tinfo->type == SBI_TLB_FLUSH_VMA)
						sbi_ipi_sfence_vma(tinfo);
					else if (tinfo->type == SBI_TLB_FLUSH_VMA_ASID)
						sbi_ipi_sfence_vma_asid(tinfo);
				}
			} while (!sbi_cq_is_empty(ipi_tlb_queue));
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
	struct sbi_cqueue *tlb_info_q = sbi_tlb_info_queue_ptr(scratch);

	sbi_ipi_data_ptr(scratch)->ipi_type = 0x00;
	sbi_cq_init(tlb_info_q, SBI_TLB_INFO_MAX_QUEUE_SIZE);

	/* Enable software interrupts */
	csr_set(CSR_MIE, MIP_MSIP);

	return sbi_platform_ipi_init(sbi_platform_ptr(scratch),
				     cold_boot);
}
