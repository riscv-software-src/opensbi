/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_encoding.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_timer.h>

static unsigned long time_delta_off;

#if __riscv_xlen == 32
u64 get_ticks(void)
{
	u32 lo, hi, tmp;
	__asm__ __volatile__("1:\n"
			     "rdtimeh %0\n"
			     "rdtime %1\n"
			     "rdtimeh %2\n"
			     "bne %0, %2, 1b"
			     : "=&r"(hi), "=&r"(lo), "=&r"(tmp));
	return ((u64)hi << 32) | lo;
}
#else
u64 get_ticks(void)
{
	unsigned long n;

	__asm__ __volatile__("rdtime %0" : "=r"(n));
	return n;
}
#endif

u64 sbi_timer_value(struct sbi_scratch *scratch)
{
	const struct sbi_platform *plat = sbi_platform_ptr(scratch);

	if (sbi_platform_has_timer_value(plat))
		return sbi_platform_timer_value(plat);
	else
		return get_ticks();
}

u64 sbi_timer_virt_value(struct sbi_scratch *scratch)
{
	u64 *time_delta = sbi_scratch_offset_ptr(scratch, time_delta_off);

	return sbi_timer_value(scratch) + *time_delta;
}

u64 sbi_timer_get_delta(struct sbi_scratch *scratch)
{
	u64 *time_delta = sbi_scratch_offset_ptr(scratch, time_delta_off);

	return *time_delta;
}

void sbi_timer_set_delta(struct sbi_scratch *scratch, ulong delta)
{
	u64 *time_delta = sbi_scratch_offset_ptr(scratch, time_delta_off);

	*time_delta = (u64)delta;
}

void sbi_timer_set_delta_upper(struct sbi_scratch *scratch, ulong delta_upper)
{
	u64 *time_delta = sbi_scratch_offset_ptr(scratch, time_delta_off);

	*time_delta &= 0xffffffffULL;
	*time_delta |= ((u64)delta_upper << 32);
}

void sbi_timer_event_start(struct sbi_scratch *scratch, u64 next_event)
{
	sbi_platform_timer_event_start(sbi_platform_ptr(scratch), next_event);
	csr_clear(CSR_MIP, MIP_STIP);
	csr_set(CSR_MIE, MIP_MTIP);
}

void sbi_timer_process(struct sbi_scratch *scratch)
{
	csr_clear(CSR_MIE, MIP_MTIP);
	csr_set(CSR_MIP, MIP_STIP);
}

int sbi_timer_init(struct sbi_scratch *scratch, bool cold_boot)
{
	u64 *time_delta;

	if (cold_boot) {
		time_delta_off = sbi_scratch_alloc_offset(sizeof(*time_delta),
							  "TIME_DELTA");
		if (!time_delta_off)
			return SBI_ENOMEM;
	} else {
		if (!time_delta_off)
			return SBI_ENOMEM;
	}

	time_delta = sbi_scratch_offset_ptr(scratch, time_delta_off);
	*time_delta = 0;

	return sbi_platform_timer_init(sbi_platform_ptr(scratch), cold_boot);
}

void sbi_timer_exit(struct sbi_scratch *scratch)
{
	sbi_platform_timer_event_stop(sbi_platform_ptr(scratch));

	csr_clear(CSR_MIP, MIP_STIP);
	csr_clear(CSR_MIE, MIP_MTIP);

	sbi_platform_timer_exit(sbi_platform_ptr(scratch));
}
