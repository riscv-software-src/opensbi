/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2021 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_atomic.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_hartmask.h>
#include <sbi/sbi_ipi.h>
#include <sbi/sbi_timer.h>
#include <sbi_utils/timer/aclint_mtimer.h>

#define MTIMER_CMP_OFF		0x0000
#define MTIMER_VAL_OFF		0x7ff8

static struct aclint_mtimer_data *mtimer_hartid2data[SBI_HARTMASK_MAX_BITS];

#if __riscv_xlen != 32
static u64 mtimer_time_rd64(volatile u64 *addr)
{
	return readq_relaxed(addr);
}

static void mtimer_time_wr64(bool timecmp, u64 value, volatile u64 *addr)
{
	writeq_relaxed(value, addr);
}
#endif

static u64 mtimer_time_rd32(volatile u64 *addr)
{
	u32 lo, hi;

	do {
		hi = readl_relaxed((u32 *)addr + 1);
		lo = readl_relaxed((u32 *)addr);
	} while (hi != readl_relaxed((u32 *)addr + 1));

	return ((u64)hi << 32) | (u64)lo;
}

static void mtimer_time_wr32(bool timecmp, u64 value, volatile u64 *addr)
{
	writel_relaxed((timecmp) ? -1U : 0U, (void *)(addr));
	writel_relaxed((u32)(value >> 32), (void *)(addr) + 0x04);
	writel_relaxed((u32)value, (void *)(addr));
}

static u64 mtimer_value(void)
{
	struct aclint_mtimer_data *mt = mtimer_hartid2data[current_hartid()];
	u64 *time_val = ((void *)mt->addr) + MTIMER_VAL_OFF;

	/* Read MTIMER Time Value */
	return mt->time_rd(time_val) + mt->time_delta;
}

static void mtimer_event_stop(void)
{
	u32 target_hart = current_hartid();
	struct aclint_mtimer_data *mt = mtimer_hartid2data[target_hart];
	u64 *time_cmp = (void *)mt->addr + MTIMER_CMP_OFF;

	/* Clear MTIMER Time Compare */
	mt->time_wr(true, -1ULL, &time_cmp[target_hart - mt->first_hartid]);
}

static void mtimer_event_start(u64 next_event)
{
	u32 target_hart = current_hartid();
	struct aclint_mtimer_data *mt = mtimer_hartid2data[target_hart];
	u64 *time_cmp = (void *)mt->addr + MTIMER_CMP_OFF;

	/* Program MTIMER Time Compare */
	mt->time_wr(true, next_event - mt->time_delta,
		    &time_cmp[target_hart - mt->first_hartid]);
}

static struct sbi_timer_device mtimer = {
	.name = "aclint-mtimer",
	.timer_value = mtimer_value,
	.timer_event_start = mtimer_event_start,
	.timer_event_stop = mtimer_event_stop
};

int aclint_mtimer_warm_init(void)
{
	u64 v1, v2, mv;
	u32 target_hart = current_hartid();
	struct aclint_mtimer_data *reference;
	u64 *mt_time_val, *mt_time_cmp, *ref_time_val;
	struct aclint_mtimer_data *mt = mtimer_hartid2data[target_hart];

	if (!mt)
		return SBI_ENODEV;

	/*
	 * Compute delta if reference available
	 *
	 * We deliberately compute time_delta in warm init so that time_delta
	 * is computed on a HART which is going to use given MTIMER. We use
	 * atomic flag timer_delta_computed to ensure that only one HART does
	 * time_delta computation.
	 */
	if (mt->time_delta_reference) {
		reference = mt->time_delta_reference;
		mt_time_val = (void *)mt->addr + MTIMER_VAL_OFF;
		ref_time_val = (void *)reference->addr + MTIMER_VAL_OFF;
		if (!atomic_raw_xchg_ulong(&mt->time_delta_computed, 1)) {
			v1 = mt->time_rd(mt_time_val);
			mv = reference->time_rd(ref_time_val);
			v2 = mt->time_rd(mt_time_val);
			mt->time_delta = mv - ((v1 / 2) + (v2 / 2));
		}
	}

	/* Clear Time Compare */
	mt_time_cmp = (void *)mt->addr + MTIMER_CMP_OFF;
	mt->time_wr(true, -1ULL,
		    &mt_time_cmp[target_hart - mt->first_hartid]);

	return 0;
}

int aclint_mtimer_cold_init(struct aclint_mtimer_data *mt,
			    struct aclint_mtimer_data *reference)
{
	u32 i;
	int rc;
	unsigned long pos, region_size;
	struct sbi_domain_memregion reg;

	/* Sanity checks */
	if (!mt || (mt->addr & (ACLINT_MTIMER_ALIGN - 1)) ||
	    (mt->size < ACLINT_MTIMER_SIZE) ||
	    (mt->first_hartid >= SBI_HARTMASK_MAX_BITS) ||
	    (mt->hart_count > ACLINT_MTIMER_MAX_HARTS))
		return SBI_EINVAL;

	/* Initialize private data */
	mt->time_delta_reference = reference;
	mt->time_delta_computed = 0;
	mt->time_delta = 0;
	mt->time_rd = mtimer_time_rd32;
	mt->time_wr = mtimer_time_wr32;

	/* Override read/write accessors for 64bit MMIO */
#if __riscv_xlen != 32
	if (mt->has_64bit_mmio) {
		mt->time_rd = mtimer_time_rd64;
		mt->time_wr = mtimer_time_wr64;
	}
#endif

	/* Update MTIMER hartid table */
	for (i = 0; i < mt->hart_count; i++)
		mtimer_hartid2data[mt->first_hartid + i] = mt;

	/* Add MTIMER regions to the root domain */
	for (pos = 0; pos < mt->size; pos += ACLINT_MTIMER_ALIGN) {
		region_size = ((mt->size - pos) < ACLINT_MTIMER_ALIGN) ?
			      (mt->size - pos) : ACLINT_MTIMER_ALIGN;
		sbi_domain_memregion_init(mt->addr + pos, region_size,
					  SBI_DOMAIN_MEMREGION_MMIO, &reg);
		rc = sbi_domain_root_add_memregion(&reg);
		if (rc)
			return rc;
	}

	sbi_timer_set_device(&mtimer);

	return 0;
}
