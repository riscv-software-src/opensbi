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
#include <sbi/riscv_atomic.h>
#include <sbi/riscv_barrier.h>
#include <sbi/riscv_unpriv.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_ipi.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_tlb.h>

static unsigned long ipi_data_off;

static int sbi_ipi_send(struct sbi_scratch *scratch, u32 hartid, u32 event,
			void *data)
{
	int ret;
	struct sbi_scratch *remote_scratch = NULL;
	const struct sbi_platform *plat = sbi_platform_ptr(scratch);
	struct sbi_ipi_data *ipi_data;

	if (sbi_platform_hart_disabled(plat, hartid))
		return -1;

	/*
	 * Set IPI type on remote hart's scratch area and
	 * trigger the interrupt
	 */
	remote_scratch = sbi_hart_id_to_scratch(scratch, hartid);
	ipi_data = sbi_scratch_offset_ptr(remote_scratch, ipi_data_off);
	if (event == SBI_IPI_EVENT_SFENCE_VMA ||
	    event == SBI_IPI_EVENT_SFENCE_VMA_ASID ||
	    event == SBI_IPI_EVENT_FENCE_I ) {
		ret = sbi_tlb_fifo_update(remote_scratch, hartid, data);
		if (ret < 0)
			return ret;
	}
	atomic_raw_set_bit(event, &ipi_data->ipi_type);
	smp_wmb();
	sbi_platform_ipi_send(plat, hartid);

	if (event == SBI_IPI_EVENT_SFENCE_VMA ||
	    event == SBI_IPI_EVENT_SFENCE_VMA_ASID ||
	    event == SBI_IPI_EVENT_FENCE_I ) {
		sbi_tlb_fifo_sync(scratch);
	}

	return 0;
}

int sbi_ipi_send_many(struct sbi_scratch *scratch, struct unpriv_trap *uptrap,
		      ulong *pmask, u32 event, void *data)
{
	ulong i, m;
	ulong mask = sbi_hart_available_mask();
	u32 hartid = sbi_current_hartid();

	if (pmask) {
		mask &= load_ulong(pmask, scratch, uptrap);
		if (uptrap->cause)
			return SBI_ETRAP;
	}

	/* Send IPIs to every other hart on the set */
	for (i = 0, m = mask; m; i++, m >>= 1)
		if ((m & 1UL) && (i != hartid))
			sbi_ipi_send(scratch, i, event, data);

	/*
	 * If the current hart is on the set, send an IPI
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

void sbi_ipi_process(struct sbi_scratch *scratch)
{
	unsigned long ipi_type;
	unsigned int ipi_event;
	const struct sbi_platform *plat = sbi_platform_ptr(scratch);
	struct sbi_ipi_data *ipi_data =
			sbi_scratch_offset_ptr(scratch, ipi_data_off);

	u32 hartid = sbi_current_hartid();
	sbi_platform_ipi_clear(plat, hartid);

	ipi_type = atomic_raw_xchg_ulong(&ipi_data->ipi_type, 0);
	ipi_event = 0;
	while (ipi_type) {
		if (!(ipi_type & 1UL))
			goto skip;

		switch (ipi_event) {
		case SBI_IPI_EVENT_SOFT:
			csr_set(CSR_MIP, MIP_SSIP);
			break;
		case SBI_IPI_EVENT_FENCE_I:
		case SBI_IPI_EVENT_SFENCE_VMA:
		case SBI_IPI_EVENT_SFENCE_VMA_ASID:
			sbi_tlb_fifo_process(scratch);
			break;
		case SBI_IPI_EVENT_HALT:
			sbi_hart_hang();
			break;
		default:
			break;
		};

skip:
		ipi_type = ipi_type >> 1;
		ipi_event++;
	};
}

int sbi_ipi_init(struct sbi_scratch *scratch, bool cold_boot)
{
	int ret;
	struct sbi_ipi_data *ipi_data;

	if (cold_boot) {
		ipi_data_off = sbi_scratch_alloc_offset(sizeof(*ipi_data),
							"IPI_DATA");
		if (!ipi_data_off)
			return SBI_ENOMEM;
	} else {
		if (!ipi_data_off)
			return SBI_ENOMEM;
	}

	ipi_data = sbi_scratch_offset_ptr(scratch, ipi_data_off);
	ipi_data->ipi_type = 0x00;

	ret = sbi_tlb_fifo_init(scratch, cold_boot);
	if (ret)
		return ret;

	/* Enable software interrupts */
	csr_set(CSR_MIE, MIP_MSIP);

	return sbi_platform_ipi_init(sbi_platform_ptr(scratch), cold_boot);
}
