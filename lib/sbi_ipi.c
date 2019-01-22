/*
 * Copyright (c) 2018 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
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

int sbi_ipi_send_many(struct sbi_scratch *scratch,
		      u32 hartid, ulong *pmask, u32 event)
{
	ulong i, m;
	struct sbi_scratch *oth;
	ulong mask = sbi_hart_available_mask();
	struct sbi_platform *plat = sbi_platform_ptr(scratch);

	if (pmask)
		mask &= load_ulong(pmask, csr_read(mepc));

	/* send IPIs to everyone */
	for (i = 0, m = mask; m; i++, m >>= 1) {
		if ((m & 1) && !sbi_platform_hart_disabled(plat, hartid)) {
			oth = sbi_hart_id_to_scratch(scratch, i);
			atomic_raw_set_bit(event, &oth->ipi_type);
			mb();
			sbi_platform_ipi_inject(plat, i, hartid);
			if (event != SBI_IPI_EVENT_SOFT)
				sbi_platform_ipi_sync(plat, i, hartid);
		}
	}

	return 0;
}

void sbi_ipi_clear_smode(struct sbi_scratch *scratch, u32 hartid)
{
	csr_clear(mip, MIP_SSIP);
}

void sbi_ipi_process(struct sbi_scratch *scratch, u32 hartid)
{
	struct sbi_platform *plat = sbi_platform_ptr(scratch);
	volatile unsigned long ipi_type;
	unsigned int ipi_event;

	sbi_platform_ipi_clear(plat, hartid);

	do {
		ipi_type = scratch->ipi_type;
		rmb();
		ipi_event = __ffs(ipi_type);
		switch (ipi_event) {
		case SBI_IPI_EVENT_SOFT:
			csr_set(mip, MIP_SSIP);
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

int sbi_ipi_init(struct sbi_scratch *scratch, u32 hartid, bool cold_boot)
{
	/* Enable software interrupts */
	csr_set(mie, MIP_MSIP);

	return sbi_platform_ipi_init(sbi_platform_ptr(scratch),
				     hartid, cold_boot);
}
