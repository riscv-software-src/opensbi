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
#include <sbi/sbi_bitops.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_init.h>
#include <sbi/sbi_ipi.h>
#include <sbi/sbi_platform.h>

struct sbi_ipi_data {
	unsigned long ipi_type;
};

static unsigned long ipi_data_off;

static const struct sbi_ipi_event_ops *ipi_ops_array[SBI_IPI_EVENT_MAX];

static int sbi_ipi_send(struct sbi_scratch *scratch, u32 remote_hartid,
			u32 event, void *data)
{
	int ret;
	struct sbi_scratch *remote_scratch = NULL;
	const struct sbi_platform *plat = sbi_platform_ptr(scratch);
	struct sbi_ipi_data *ipi_data;
	const struct sbi_ipi_event_ops *ipi_ops;

	if ((SBI_IPI_EVENT_MAX <= event) ||
	    !ipi_ops_array[event] ||
	    sbi_platform_hart_disabled(plat, remote_hartid))
		return SBI_EINVAL;
	ipi_ops = ipi_ops_array[event];

	/*
	 * Set IPI type on remote hart's scratch area and
	 * trigger the interrupt
	 */
	remote_scratch = sbi_hart_id_to_scratch(scratch, remote_hartid);
	ipi_data = sbi_scratch_offset_ptr(remote_scratch, ipi_data_off);

	if (ipi_ops->update) {
		ret = ipi_ops->update(scratch, remote_scratch,
				      remote_hartid, data);
		if (ret < 0)
			return ret;
	}

	atomic_raw_set_bit(event, &ipi_data->ipi_type);
	smp_wmb();
	sbi_platform_ipi_send(plat, remote_hartid);

	if (ipi_ops->sync)
		ipi_ops->sync(scratch);

	return 0;
}

/**
 * As this this function only handlers scalar values of hart mask, it must be
 * set to all online harts if the intention is to send IPIs to all the harts.
 * If hmask is zero, no IPIs will be sent.
 */
int sbi_ipi_send_many(struct sbi_scratch *scratch, ulong hmask, ulong hbase,
			u32 event, void *data)
{
	ulong i, m;
	ulong mask = sbi_hart_available_mask();
	ulong tempmask;
	unsigned long last_bit = __fls(mask);

	if (hbase != -1UL) {
		if (hbase > last_bit)
			/* hart base is not available */
			return SBI_EINVAL;
		/**
		 * FIXME: This check is valid only ULONG size. This is okay for
		 * now as avaialble hart mask can support upto ULONG size only.
		 */
		tempmask = hmask << hbase;
		tempmask = ~mask & tempmask;
		if (tempmask)
			/* at least one of the hart in hmask is not available */
			return SBI_EINVAL;

		mask &= (hmask << hbase);
	}

	/* Send IPIs to every other hart on the set */
	for (i = 0, m = mask; m; i++, m >>= 1)
		if (m & 1UL)
			sbi_ipi_send(scratch, i, event, data);

	return 0;
}

int sbi_ipi_event_create(const struct sbi_ipi_event_ops *ops)
{
	int i, ret = SBI_ENOSPC;

	if (!ops || !ops->process)
		return SBI_EINVAL;

	for (i = 0; i < SBI_IPI_EVENT_MAX; i++) {
		if (!ipi_ops_array[i]) {
			ret = i;
			ipi_ops_array[i] = ops;
			break;
		}
	}

	return ret;
}

void sbi_ipi_event_destroy(u32 event)
{
	if (SBI_IPI_EVENT_MAX <= event)
		return;

	ipi_ops_array[event] = NULL;
}

static void sbi_ipi_process_smode(struct sbi_scratch *scratch)
{
	csr_set(CSR_MIP, MIP_SSIP);
}

static struct sbi_ipi_event_ops ipi_smode_ops = {
	.name = "IPI_SMODE",
	.process = sbi_ipi_process_smode,
};

static u32 ipi_smode_event = SBI_IPI_EVENT_MAX;

int sbi_ipi_send_smode(struct sbi_scratch *scratch, ulong hmask, ulong hbase)
{
	return sbi_ipi_send_many(scratch, hmask, hbase, ipi_smode_event, NULL);
}

void sbi_ipi_clear_smode(struct sbi_scratch *scratch)
{
	csr_clear(CSR_MIP, MIP_SSIP);
}

static void sbi_ipi_process_halt(struct sbi_scratch *scratch)
{
	sbi_exit(scratch);
}

static struct sbi_ipi_event_ops ipi_halt_ops = {
	.name = "IPI_HALT",
	.process = sbi_ipi_process_halt,
};

static u32 ipi_halt_event = SBI_IPI_EVENT_MAX;

int sbi_ipi_send_halt(struct sbi_scratch *scratch, ulong hmask, ulong hbase)
{
	return sbi_ipi_send_many(scratch, hmask, hbase, ipi_halt_event, NULL);
}

void sbi_ipi_process(struct sbi_scratch *scratch)
{
	unsigned long ipi_type;
	unsigned int ipi_event;
	const struct sbi_ipi_event_ops *ipi_ops;
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

		ipi_ops = ipi_ops_array[ipi_event];
		if (ipi_ops && ipi_ops->process)
			ipi_ops->process(scratch);

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
		ret = sbi_ipi_event_create(&ipi_smode_ops);
		if (ret < 0)
			return ret;
		ipi_smode_event = ret;
		ret = sbi_ipi_event_create(&ipi_halt_ops);
		if (ret < 0)
			return ret;
		ipi_halt_event = ret;
	} else {
		if (!ipi_data_off)
			return SBI_ENOMEM;
		if (SBI_IPI_EVENT_MAX <= ipi_smode_event ||
		    SBI_IPI_EVENT_MAX <= ipi_halt_event)
			return SBI_ENOSPC;
	}

	ipi_data = sbi_scratch_offset_ptr(scratch, ipi_data_off);
	ipi_data->ipi_type = 0x00;

	/* Platform init */
	ret = sbi_platform_ipi_init(sbi_platform_ptr(scratch), cold_boot);
	if (ret)
		return ret;

	/* Enable software interrupts */
	csr_set(CSR_MIE, MIP_MSIP);

	return 0;
}

void sbi_ipi_exit(struct sbi_scratch *scratch)
{
	/* Disable software interrupts */
	csr_clear(CSR_MIE, MIP_MSIP);

	/* Process pending IPIs */
	sbi_ipi_process(scratch);

	/* Platform exit */
	sbi_platform_ipi_exit(sbi_platform_ptr(scratch));
}
