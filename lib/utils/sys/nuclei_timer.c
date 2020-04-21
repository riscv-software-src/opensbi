/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Nuclei Corporation or its affiliates.
 *
 * Authors:
 *   lujun <lujun@nucleisys.com>
 */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_atomic.h>
#include <sbi/riscv_io.h>
#include <sbi_utils/sys/nuclei_timer.h>

static u32 nuclei_timer_ipi_hart_count;
static volatile void *nuclei_timer_ipi_base;
static volatile u32 *nuclei_timer_ipi;

void nuclei_timer_ipi_send(u32 target_hart)
{
	if (nuclei_timer_ipi_hart_count <= target_hart)
		return;

	/* Set RV_TIMER IPI */
	writel(1, &nuclei_timer_ipi[target_hart]);
}

void nuclei_timer_ipi_clear(u32 target_hart)
{
	if (nuclei_timer_ipi_hart_count <= target_hart)
		return;

	/* Clear RV_TIMER IPI */
	writel(0, &nuclei_timer_ipi[target_hart]);
}

int nuclei_timer_warm_ipi_init(void)
{
	u32 hartid = current_hartid();

	if (!nuclei_timer_ipi_base)
		return -1;

	/* Clear RV_TIMER IPI */
	nuclei_timer_ipi_clear(hartid);

	return 0;
}

int nuclei_timer_cold_ipi_init(unsigned long base, u32 hart_count)
{
	/* Figure-out RV_TIMER IPI register address */
	nuclei_timer_ipi_hart_count = hart_count;
	nuclei_timer_ipi_base	     = (void *)base;
	nuclei_timer_ipi	     = (u32 *)nuclei_timer_ipi_base;

	return 0;
}

static u32 nuclei_timer_time_hart_count;
static volatile void *nuclei_timer_time_base;
static volatile u64 *nuclei_timer_time_val;
static volatile u64 *nuclei_timer_time_cmp;

#if __riscv_xlen != 32
static u64 nuclei_timer_time_rd64(volatile u64 *addr)
{
	return readq_relaxed(addr);
}

static void nuclei_timer_time_wr64(u64 value, volatile u64 *addr)
{
	writeq_relaxed(value, addr);
}
#endif

static u64 nuclei_timer_time_rd32(volatile u64 *addr)
{
	u32 lo, hi;

	do {
		hi = readl_relaxed((u32 *)addr + 1);
		lo = readl_relaxed((u32 *)addr);
	} while (hi != readl_relaxed((u32 *)addr + 1));

	return ((u64)hi << 32U) | (u64)lo;
}

static void nuclei_timer_time_wr32(u64 value, volatile u64 *addr)
{
	u32 mask = -1U;

	writel_relaxed(value & mask, (void *)(addr));
	writel_relaxed(value >> 32U, (void *)(addr) + 0x04);
}

static u64 (*nuclei_timer_time_rd)(volatile u64 *addr) = nuclei_timer_time_rd32;
static void (*nuclei_timer_time_wr)(u64 value, volatile u64 *addr) = nuclei_timer_time_wr32;

u64 nuclei_timer_timer_value(void)
{
	/* Read RV_TIMER Time Value */
	return nuclei_timer_time_rd(nuclei_timer_time_val);
}

void nuclei_timer_timer_event_stop(void)
{
	u32 target_hart = current_hartid();

	if (nuclei_timer_time_hart_count <= target_hart)
		return;

	/* Clear RV_TIMER Time Compare */
	nuclei_timer_time_wr(-1ULL, &nuclei_timer_time_cmp[target_hart]);
}

void nuclei_timer_timer_event_start(u64 next_event)
{
	u32 target_hart = current_hartid();

	if (nuclei_timer_time_hart_count <= target_hart)
		return;

	/* Program RV_TIMER Time Compare */
	nuclei_timer_time_wr(next_event, &nuclei_timer_time_cmp[target_hart]);
}

int nuclei_timer_warm_timer_init(void)
{
	u32 target_hart = current_hartid();

	if (nuclei_timer_time_hart_count <= target_hart || !nuclei_timer_time_base)
		return -1;

	/* Clear RV_TIMER Time Compare */
	nuclei_timer_time_wr(-1ULL, &nuclei_timer_time_cmp[target_hart]);

	return 0;
}

int nuclei_timer_cold_timer_init(unsigned long base, u32 hart_count,
			  bool has_64bit_mmio)
{
	/* Figure-out RV_TIMER Time register address */
	nuclei_timer_time_hart_count		= hart_count;
	nuclei_timer_time_base			= (void *)base;
	nuclei_timer_time_val			= (u64 *)(nuclei_timer_time_base + 0x0);
	nuclei_timer_time_cmp			= (u64 *)(nuclei_timer_time_base + 0x8);

	/* Override read/write accessors for 64bit MMIO */
#if __riscv_xlen != 32
	if (has_64bit_mmio) {
		nuclei_timer_time_rd		= nuclei_timer_time_rd64;
		nuclei_timer_time_wr		= nuclei_timer_time_wr64;
	}
#endif

	return 0;
}
