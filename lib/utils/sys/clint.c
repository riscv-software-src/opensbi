/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_atomic.h>
#include <sbi/riscv_io.h>
#include <sbi_utils/sys/clint.h>

static u32 clint_ipi_hart_count;
static volatile void *clint_ipi_base;
static volatile u32 *clint_ipi;

void clint_ipi_send(u32 target_hart)
{
	if (clint_ipi_hart_count <= target_hart)
		return;

	/* Set CLINT IPI */
	writel(1, &clint_ipi[target_hart]);
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
	u32 hartid = current_hartid();

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
	clint_ipi_base	     = (void *)base;
	clint_ipi	     = (u32 *)clint_ipi_base;

	return 0;
}

static u32 clint_time_hart_count;
static volatile void *clint_time_base;
static volatile u64 *clint_time_val;
static volatile u64 *clint_time_cmp;

#if __riscv_xlen != 32
static u64 clint_time_rd64(volatile u64 *addr)
{
	return readq_relaxed(addr);
}

static void clint_time_wr64(u64 value, volatile u64 *addr)
{
	writeq_relaxed(value, addr);
}
#endif

static u64 clint_time_rd32(volatile u64 *addr)
{
	u32 lo, hi;

	do {
		hi = readl_relaxed((u32 *)addr + 1);
		lo = readl_relaxed((u32 *)addr);
	} while (hi != readl_relaxed((u32 *)addr + 1));

	return ((u64)hi << 32) | (u64)lo;
}

static void clint_time_wr32(u64 value, volatile u64 *addr)
{
	u32 mask = -1U;

	writel_relaxed(value & mask, (void *)(addr));
	writel_relaxed(value >> 32, (void *)(addr) + 0x04);
}

static u64 (*clint_time_rd)(volatile u64 *addr) = clint_time_rd32;
static void (*clint_time_wr)(u64 value, volatile u64 *addr) = clint_time_wr32;

u64 clint_timer_value(void)
{
	/* Read CLINT Time Value */
	return clint_time_rd(clint_time_val);
}

void clint_timer_event_stop(void)
{
	u32 target_hart = current_hartid();

	if (clint_time_hart_count <= target_hart)
		return;

	/* Clear CLINT Time Compare */
	clint_time_wr(-1ULL, &clint_time_cmp[target_hart]);
}

void clint_timer_event_start(u64 next_event)
{
	u32 target_hart = current_hartid();

	if (clint_time_hart_count <= target_hart)
		return;

	/* Program CLINT Time Compare */
	clint_time_wr(next_event, &clint_time_cmp[target_hart]);
}

int clint_warm_timer_init(void)
{
	u32 target_hart = current_hartid();

	if (clint_time_hart_count <= target_hart || !clint_time_base)
		return -1;

	/* Clear CLINT Time Compare */
	clint_time_wr(-1ULL, &clint_time_cmp[target_hart]);

	return 0;
}

int clint_cold_timer_init(unsigned long base, u32 hart_count,
			  bool has_64bit_mmio)
{
	/* Figure-out CLINT Time register address */
	clint_time_hart_count		= hart_count;
	clint_time_base			= (void *)base;
	clint_time_val			= (u64 *)(clint_time_base + 0xbff8);
	clint_time_cmp			= (u64 *)(clint_time_base + 0x4000);

	/* Override read/write accessors for 64bit MMIO */
#if __riscv_xlen != 32
	if (has_64bit_mmio) {
		clint_time_rd		= clint_time_rd64;
		clint_time_wr		= clint_time_wr64;
	}
#endif

	return 0;
}
