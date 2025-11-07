// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 */

#include <platform_override.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/fdt/fdt_fixup.h>
#include <sbi_utils/ipi/aclint_mswi.h>
#include <sbi_utils/irqchip/plic.h>
#include <sbi_utils/timer/aclint_mtimer.h>

#define OPENPITON_DEFAULT_PLIC_ADDR		0xfff1100000ULL
#define OPENPITON_DEFAULT_PLIC_SIZE		(0x200000 + \
				(OPENPITON_DEFAULT_HART_COUNT * 0x1000))
#define OPENPITON_DEFAULT_PLIC_NUM_SOURCES	2
#define OPENPITON_DEFAULT_HART_COUNT		3
#define OPENPITON_DEFAULT_CLINT_ADDR		0xfff1020000ULL
#define OPENPITON_DEFAULT_ACLINT_MTIMER_FREQ	1000000
#define OPENPITON_DEFAULT_ACLINT_MSWI_ADDR	\
		(OPENPITON_DEFAULT_CLINT_ADDR + CLINT_MSWI_OFFSET)
#define OPENPITON_DEFAULT_ACLINT_MTIMER_ADDR	\
		(OPENPITON_DEFAULT_CLINT_ADDR + CLINT_MTIMER_OFFSET)

static struct plic_data plic = {
	.addr = (unsigned long)OPENPITON_DEFAULT_PLIC_ADDR,
	.size = OPENPITON_DEFAULT_PLIC_SIZE,
	.num_src = OPENPITON_DEFAULT_PLIC_NUM_SOURCES,
	.flags = PLIC_FLAG_ARIANE_BUG,
	.context_map = {
		[0] = { 0, 1 },
		[1] = { 2, 3 },
		[2] = { 4, 5 },
	},
};

static struct aclint_mswi_data mswi = {
	.addr = (unsigned long)OPENPITON_DEFAULT_ACLINT_MSWI_ADDR,
	.size = ACLINT_MSWI_SIZE,
	.first_hartid = 0,
	.hart_count = OPENPITON_DEFAULT_HART_COUNT,
};

static struct aclint_mtimer_data mtimer = {
	.mtime_freq = OPENPITON_DEFAULT_ACLINT_MTIMER_FREQ,
	.mtime_addr = (unsigned long)OPENPITON_DEFAULT_ACLINT_MTIMER_ADDR +
		      ACLINT_DEFAULT_MTIME_OFFSET,
	.mtime_size = ACLINT_DEFAULT_MTIME_SIZE,
	.mtimecmp_addr = (unsigned long)OPENPITON_DEFAULT_ACLINT_MTIMER_ADDR +
			 ACLINT_DEFAULT_MTIMECMP_OFFSET,
	.mtimecmp_size = ACLINT_DEFAULT_MTIMECMP_SIZE,
	.first_hartid = 0,
	.hart_count = OPENPITON_DEFAULT_HART_COUNT,
	.has_64bit_mmio = true,
};

/*
 * OpenPiton platform early initialization.
 */
static int openpiton_early_init(bool cold_boot)
{
	const void *fdt;
	struct plic_data plic_data = plic;
	unsigned long aclint_freq;
	uint64_t clint_addr;
	int rc;

	if (!cold_boot)
		return 0;
	fdt = fdt_get_address();

	rc = generic_early_init(cold_boot);
	if (rc)
		return rc;

	rc = fdt_parse_plic(fdt, &plic_data, "riscv,plic0");
	if (!rc)
		plic = plic_data;

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

	if (rc)
		return rc;

	return aclint_mswi_cold_init(&mswi);
}

/*
 * OpenPiton platform final initialization.
 */
static int openpiton_final_init(bool cold_boot)
{
	void *fdt;

	if (!cold_boot)
		return 0;

	fdt = fdt_get_address_rw();
	fdt_fixups(fdt);

	return 0;
}

/*
 * Initialize the openpiton interrupt controller during cold boot.
 */
static int openpiton_irqchip_init(void)
{
	return plic_cold_irqchip_init(&plic);
}

/*
 * Initialize openpiton timer during cold boot.
 */
static int openpiton_timer_init(void)
{
	return aclint_mtimer_cold_init(&mtimer, NULL);
}

static int openhwgroup_openpiton_platform_init(const void *fdt, int nodeoff, const struct fdt_match *match)
{
	generic_platform_ops.early_init = openpiton_early_init;
	generic_platform_ops.timer_init = openpiton_timer_init;
	generic_platform_ops.irqchip_init = openpiton_irqchip_init;
	generic_platform_ops.final_init = openpiton_final_init;

	return 0;
}

static const struct fdt_match openhwgroup_openpiton_match[] = {
	{ .compatible = "openpiton,cva6platform" },
	{ },
};

const struct fdt_driver openhwgroup_openpiton = {
	.match_table = openhwgroup_openpiton_match,
	.init = openhwgroup_openpiton_platform_init,
};
