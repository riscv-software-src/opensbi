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
#include <sbi/sbi_hart.h>
#include <sbi/sbi_bitops.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_ipi.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_timer.h>
#include <sbi/sbi_unpriv.h>

static int sbi_ipi_send(struct sbi_scratch *scratch, u32 hartid, u32 event)
{
	struct sbi_scratch *remote_scratch = NULL;
	const struct sbi_platform *plat = sbi_platform_ptr(scratch);

	if (sbi_platform_hart_disabled(plat, hartid))
		return -1;

	/* Set IPI type on remote hart's scratch area and
	 * trigger the interrupt
	 */
	remote_scratch = sbi_hart_id_to_scratch(scratch, hartid);
	atomic_raw_set_bit(event, &remote_scratch->ipi_type);
	mb();
	sbi_platform_ipi_send(plat, hartid);
	if (event != SBI_IPI_EVENT_SOFT)
		sbi_platform_ipi_sync(plat, hartid);

	return 0;
}

int sbi_ipi_send_many(struct sbi_scratch *scratch,
		      ulong *pmask, u32 event)
{
	ulong i, m;
	ulong mask = sbi_hart_available_mask();
	u32 hartid = sbi_current_hartid();

	if (pmask)
		mask &= load_ulong(pmask, csr_read(CSR_MEPC));

	/* send IPIs to every other hart on the set */
	for (i = 0, m = mask; m; i++, m >>= 1)
		if ((m & 1UL) && (i != hartid))
			sbi_ipi_send(scratch, i, event);

	/* If the current hart is on the set, send an IPI
	 * to it as well
	 */
	if (mask & (1UL << hartid))
		sbi_ipi_send(scratch, hartid, event);


	return 0;
}

void sbi_ipi_clear_smode(struct sbi_scratch *scratch)
{
	csr_clear(CSR_MIP, MIP_SSIP);
}

void sbi_ipi_process(struct sbi_scratch *scratch)
{
	const struct sbi_platform *plat = sbi_platform_ptr(scratch);
	volatile unsigned long ipi_type;
	unsigned int ipi_event;
	u32 hartid = sbi_current_hartid();

	sbi_platform_ipi_clear(plat, hartid);

	do {
		ipi_type = scratch->ipi_type;
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
			__asm__ __volatile("sfence.vma");
			break;
		case SBI_IPI_EVENT_HALT:
			sbi_hart_hang();
			break;
		};
		ipi_type = atomic_raw_clear_bit(ipi_event, &scratch->ipi_type);
	} while(ipi_type > 0);
}

int sbi_ipi_init(struct sbi_scratch *scratch, bool cold_boot)
{
	/* Enable software interrupts */
	csr_set(CSR_MIE, MIP_MSIP);

	return sbi_platform_ipi_init(sbi_platform_ptr(scratch),
				     cold_boot);
}
