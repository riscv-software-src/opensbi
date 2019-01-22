/*
 * Copyright (c) 2018 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sbi/riscv_io.h>
#include <sbi/riscv_atomic.h>
#include <sbi/sbi_hart.h>
#include <plat/sys/clint.h>

static u32 clint_ipi_hart_count;
static volatile void *clint_ipi_base;
static volatile u32 *clint_ipi;

void clint_ipi_inject(u32 target_hart)
{
	if (clint_ipi_hart_count <= target_hart)
		return;

	/* Set CLINT IPI */
	writel(1, &clint_ipi[target_hart]);
}

void clint_ipi_sync(u32 target_hart)
{
	u32 target_ipi, incoming_ipi;
	u32 source_hart = sbi_current_hartid();

	if (clint_ipi_hart_count <= target_hart)
		return;

	/* Wait until target HART has handled IPI */
	incoming_ipi = 0;
	while (1) {
		target_ipi = readl(&clint_ipi[target_hart]);
		if (!target_ipi)
			break;

		incoming_ipi |=
			atomic_raw_xchg_uint(&clint_ipi[source_hart], 0);
	}

	if (incoming_ipi)
		writel(incoming_ipi, &clint_ipi[source_hart]);
}

void clint_ipi_clear(u32 target_hart)
{
	if (clint_ipi_hart_count <= target_hart)
		return;

	/* Clear CLINT IPI */
	writel(0, &clint_ipi[target_hart]);
}

int clint_warm_ipi_init(void)
{
	u32 hartid = sbi_current_hartid();

	if (!clint_ipi_base)
		return -1;

	/* Clear CLINT IPI */
	clint_ipi_clear(hartid);

	return 0;
}

int clint_cold_ipi_init(unsigned long base, u32 hart_count)
{
	/* Figure-out CLINT IPI register address */
	clint_ipi_hart_count = hart_count;
	clint_ipi_base = (void *)base;
	clint_ipi = (u32 *)clint_ipi_base;

	return 0;
}

static u32 clint_time_hart_count;
static volatile void *clint_time_base;
static volatile u64 *clint_time_val;
static volatile u64 *clint_time_cmp;

u64 clint_timer_value(void)
{
	return readq_relaxed(clint_time_val);
}

void clint_timer_event_stop(u32 target_hart)
{
	if (clint_time_hart_count <= target_hart)
		return;

	/* Clear CLINT Time Compare */
	writeq_relaxed(-1ULL, &clint_time_cmp[target_hart]);
}

void clint_timer_event_start(u32 target_hart, u64 next_event)
{
	if (clint_time_hart_count <= target_hart)
		return;

	/* Program CLINT Time Compare */
	writeq_relaxed(next_event, &clint_time_cmp[target_hart]);
}

int clint_warm_timer_init(u32 target_hart)
{
	if (clint_time_hart_count <= target_hart ||
	    !clint_time_base)
		return -1;

	/* Clear CLINT Time Compare */
	writeq_relaxed(-1ULL, &clint_time_cmp[target_hart]);

	return 0;
}

int clint_cold_timer_init(unsigned long base, u32 hart_count)
{
	/* Figure-out CLINT Time register address */
	clint_time_hart_count = hart_count;
	clint_time_base = (void *)base;
	clint_time_val = (u64 *)(clint_time_base + 0xbff8);
	clint_time_cmp = (u64 *)(clint_time_base + 0x4000);

	return 0;
}
