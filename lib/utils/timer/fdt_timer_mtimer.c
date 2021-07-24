/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2021 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <libfdt.h>
#include <sbi/sbi_error.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/timer/fdt_timer.h>
#include <sbi_utils/timer/aclint_mtimer.h>

#define MTIMER_MAX_NR			16

static unsigned long mtimer_count = 0;
static struct aclint_mtimer_data mtimer[MTIMER_MAX_NR];

static int timer_mtimer_cold_init(void *fdt, int nodeoff,
				  const struct fdt_match *match)
{
	int rc;
	unsigned long offset, addr[2], size[2];
	struct aclint_mtimer_data *mt, *mtmaster = NULL;

	if (MTIMER_MAX_NR <= mtimer_count)
		return SBI_ENOSPC;
	mt = &mtimer[mtimer_count];
	if (0 < mtimer_count)
		mtmaster = &mtimer[0];

	rc = fdt_parse_aclint_node(fdt, nodeoff, true,
				   &addr[0], &size[0], &addr[1], &size[1],
				   &mt->first_hartid, &mt->hart_count);
	if (rc)
		return rc;
	mt->has_64bit_mmio = true;

	if (match->data) { /* SiFive CLINT */
		/* Set CLINT addresses */
		mt->mtimecmp_addr = addr[0] + ACLINT_DEFAULT_MTIMECMP_OFFSET;
		mt->mtimecmp_size = ACLINT_DEFAULT_MTIMECMP_SIZE;
		mt->mtime_addr = addr[0] + ACLINT_DEFAULT_MTIME_OFFSET;
		mt->mtime_size = size[0] - mt->mtimecmp_size;
		/* Adjust MTIMER address and size for CLINT device */
		offset = *((unsigned long *)match->data);
		mt->mtime_addr += offset;
		mt->mtimecmp_addr += offset;
		mt->mtime_size -= offset;
		/* Parse additional CLINT properties */
		if (fdt_getprop(fdt, nodeoff, "clint,has-no-64bit-mmio", &rc))
			mt->has_64bit_mmio = false;
	} else { /* RISC-V ACLINT MTIMER */
		/* Set ACLINT MTIMER addresses */
		mt->mtime_addr = addr[0];
		mt->mtime_size = size[0];
		mt->mtimecmp_addr = addr[1];
		mt->mtimecmp_size = size[1];
		/* Parse additional ACLINT MTIMER properties */
		if (fdt_getprop(fdt, nodeoff, "mtimer,no-64bit-mmio", &rc))
			mt->has_64bit_mmio = false;
	}

	rc = aclint_mtimer_cold_init(mt, mtmaster);
	if (rc)
		return rc;

	mtimer_count++;
	return 0;
}

static unsigned long clint_offset = CLINT_MTIMER_OFFSET;

static const struct fdt_match timer_mtimer_match[] = {
	{ .compatible = "riscv,clint0", .data = &clint_offset },
	{ .compatible = "sifive,clint0", .data = &clint_offset },
	{ .compatible = "riscv,aclint-mtimer" },
	{ },
};

struct fdt_timer fdt_timer_mtimer = {
	.match_table = timer_mtimer_match,
	.cold_init = timer_mtimer_cold_init,
	.warm_init = aclint_mtimer_warm_init,
	.exit = NULL,
};
