/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2019 FORTH-ICS/CARV
 *				Panagiotis Peristerakis <perister@ics.forth.gr>
 */

#include <platform_override.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/fdt/fdt_fixup.h>
#include <sbi_utils/ipi/aclint_mswi.h>
#include <sbi_utils/irqchip/plic.h>
#include <sbi_utils/timer/aclint_mtimer.h>

#define ARIANE_PLIC_ADDR			0xc000000
#define ARIANE_PLIC_SIZE			(0x200000 + \
						 (ARIANE_HART_COUNT * 0x1000))
#define ARIANE_PLIC_NUM_SOURCES			3
#define ARIANE_HART_COUNT			1
#define ARIANE_CLINT_ADDR			0x2000000
#define ARIANE_ACLINT_MTIMER_FREQ		1000000
#define ARIANE_ACLINT_MSWI_ADDR			(ARIANE_CLINT_ADDR + \
						 CLINT_MSWI_OFFSET)
#define ARIANE_ACLINT_MTIMER_ADDR		(ARIANE_CLINT_ADDR + \
						 CLINT_MTIMER_OFFSET)

static struct plic_data plic = {
	.addr = ARIANE_PLIC_ADDR,
	.size = ARIANE_PLIC_SIZE,
	.num_src = ARIANE_PLIC_NUM_SOURCES,
	.flags = PLIC_FLAG_ARIANE_BUG,
	.context_map = {
		[0] = { 0, 1 },
	},
};

static struct aclint_mswi_data mswi = {
	.addr = ARIANE_ACLINT_MSWI_ADDR,
	.size = ACLINT_MSWI_SIZE,
	.first_hartid = 0,
	.hart_count = ARIANE_HART_COUNT,
};

static struct aclint_mtimer_data mtimer = {
	.mtime_freq = ARIANE_ACLINT_MTIMER_FREQ,
	.mtime_addr = ARIANE_ACLINT_MTIMER_ADDR +
		      ACLINT_DEFAULT_MTIME_OFFSET,
	.mtime_size = ACLINT_DEFAULT_MTIME_SIZE,
	.mtimecmp_addr = ARIANE_ACLINT_MTIMER_ADDR +
			 ACLINT_DEFAULT_MTIMECMP_OFFSET,
	.mtimecmp_size = ACLINT_DEFAULT_MTIMECMP_SIZE,
	.first_hartid = 0,
	.hart_count = ARIANE_HART_COUNT,
	.has_64bit_mmio = true,
};

/*
 * Ariane platform early initialization.
 */
static int ariane_early_init(bool cold_boot)
{
	const void *fdt;
	struct plic_data plic_data = plic;
	unsigned long aclint_freq;
	uint64_t clint_addr;
	int rc;

	if (!cold_boot)
		return 0;

	rc = generic_early_init(cold_boot);
	if (rc)
		return rc;

	fdt = fdt_get_address();

	rc = fdt_parse_timebase_frequency(fdt, &aclint_freq);
	if (!rc)
		mtimer.mtime_freq = aclint_freq;

	rc = fdt_parse_compat_addr(fdt, &clint_addr, "riscv,clint0");
	if (!rc) {
		mswi.addr = clint_addr;
		mtimer.mtime_addr = clint_addr + CLINT_MTIMER_OFFSET +
				    ACLINT_DEFAULT_MTIME_OFFSET;
		mtimer.mtimecmp_addr = clint_addr + CLINT_MTIMER_OFFSET +
				       ACLINT_DEFAULT_MTIMECMP_OFFSET;
	}

	rc = fdt_parse_plic(fdt, &plic_data, "riscv,plic0");
	if (!rc)
		plic = plic_data;

	return aclint_mswi_cold_init(&mswi);
}

/*
 * Ariane platform final initialization.
 */
static int ariane_final_init(bool cold_boot)
{
	void *fdt;

	if (!cold_boot)
		return 0;

	fdt = fdt_get_address_rw();
	fdt_fixups(fdt);

	return 0;
}

/*
 * Initialize the ariane interrupt controller during cold boot.
 */
static int ariane_irqchip_init(void)
{
	return plic_cold_irqchip_init(&plic);
}

/*
 * Initialize ariane timer during cold boot.
 */
static int ariane_timer_init(void)
{
	return aclint_mtimer_cold_init(&mtimer, NULL);
}

static int openhwgroup_ariane_platform_init(const void *fdt, int nodeoff, const struct fdt_match *match)
{
	generic_platform_ops.early_init = ariane_early_init;
	generic_platform_ops.timer_init = ariane_timer_init;
	generic_platform_ops.irqchip_init = ariane_irqchip_init;
	generic_platform_ops.final_init = ariane_final_init;

	return 0;
}

static const struct fdt_match openhwgroup_ariane_match[] = {
	{ .compatible = "eth,ariane-bare-dev" },
	{ },
};

const struct fdt_driver openhwgroup_ariane = {
	.match_table = openhwgroup_ariane_match,
	.init = openhwgroup_ariane_platform_init,
};
