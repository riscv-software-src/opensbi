/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 SpacemiT
 * Authors:
 *   Xianbin Zhu <xianbin.zhu@linux.spacemit.com>
 *   Troy Mitchell <troy.mitchell@linux.spacemit.com>
 */

#include <platform_override.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_hsm.h>
#include <spacemit/k1.h>

static const u64 cpu_wakeup_reg[] = {
	PMU_AP_CORE0_WAKEUP,
	PMU_AP_CORE1_WAKEUP,
	PMU_AP_CORE2_WAKEUP,
	PMU_AP_CORE3_WAKEUP,
	PMU_AP_CORE4_WAKEUP,
	PMU_AP_CORE5_WAKEUP,
	PMU_AP_CORE6_WAKEUP,
	PMU_AP_CORE7_WAKEUP,
};

static const u64 cpu_idle_reg[] = {
	PMU_AP_CORE0_IDLE_CFG,
	PMU_AP_CORE1_IDLE_CFG,
	PMU_AP_CORE2_IDLE_CFG,
	PMU_AP_CORE3_IDLE_CFG,
	PMU_AP_CORE4_IDLE_CFG,
	PMU_AP_CORE5_IDLE_CFG,
	PMU_AP_CORE6_IDLE_CFG,
	PMU_AP_CORE7_IDLE_CFG,
};

static inline void spacemit_set_cpu_power(u32 hartid, bool enable)
{
	unsigned int value;
	unsigned int *cpu_idle_base = (unsigned int *)(unsigned long)cpu_idle_reg[hartid];

	value = readl(cpu_idle_base);

	if (enable)
		value &= ~PMU_AP_IDLE_PWRDOWN_MASK;
	else
		value |= PMU_AP_IDLE_PWRDOWN_MASK;

	writel(value, cpu_idle_base);
}

static void spacemit_wakeup_cpu(u32 mpidr)
{
	unsigned int *cpu_reset_base;
	unsigned int cur_hartid = current_hartid();

	cpu_reset_base = (unsigned int *)(unsigned long)cpu_wakeup_reg[cur_hartid];

	writel(1 << mpidr, cpu_reset_base);
}

static void spacemit_assert_cpu(void)
{
	spacemit_set_cpu_power(current_hartid(), false);
}

static void spacemit_deassert_cpu(unsigned int hartid)
{
	spacemit_set_cpu_power(hartid, true);
}

/* Start (or power-up) the given hart */
static int spacemit_hart_start(unsigned int hartid, unsigned long saddr)
{
	spacemit_deassert_cpu(hartid);
	spacemit_wakeup_cpu(hartid);

	return 0;
}

/*
 * Stop (or power-down) the current hart from running. This call
 * doesn't expect to return if success.
 */
static int spacemit_hart_stop(void)
{
	csr_write(CSR_STIMECMP, GENMASK_ULL(63, 0));
	csr_clear(CSR_MIE, MIP_SSIP | MIP_MSIP | MIP_STIP | MIP_MTIP | MIP_SEIP | MIP_MEIP);

	/* disable data preftch */
	csr_clear(CSR_MSETUP, MSETUP_PFE);
	asm volatile ("fence iorw, iorw");

	/* flush local dcache */
	csr_write(CSR_MRAOP, MRAOP_ICACHE_INVALID);
	asm volatile ("fence iorw, iorw");

	/* disable dcache */
	csr_clear(CSR_MSETUP, MSETUP_DE);
	asm volatile ("fence iorw, iorw");

	/*
	 * Core4-7 do not have dedicated bits in ML2SETUP;
	 * instead, they reuse the same bits as core0-3.
	 *
	 * Thereforspacemit_deassert_cpue, use modulo with PLATFORM_MAX_CPUS_PER_CLUSTER
	 * to select the proper bit.
	 */
	csr_clear(CSR_ML2SETUP, 1 << (current_hartid() % PLATFORM_MAX_CPUS_PER_CLUSTER));
	asm volatile ("fence iorw, iorw");

	spacemit_assert_cpu();

	wfi();

	return SBI_ENOTSUPP;
}

static const struct sbi_hsm_device spacemit_hsm_ops = {
	.name		= "spacemit-hsm",
	.hart_start	= spacemit_hart_start,
	.hart_stop	= spacemit_hart_stop,
};

static int spacemit_hsm_probe(const void *fdt, int nodeoff, const struct fdt_match *match)
{
	sbi_hsm_set_device(&spacemit_hsm_ops);

	return 0;
}

static const struct fdt_match spacemit_hsm_match[] = {
	{ .compatible = "spacemit,k1" },
	{ },
};

const struct fdt_driver fdt_hsm_spacemit = {
	.match_table = spacemit_hsm_match,
	.init = spacemit_hsm_probe,
};
