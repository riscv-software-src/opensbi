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
#include <sbi/sbi_error.h>
#include <sbi/sbi_hsm.h>
#include <spacemit/common.h>
#include <spacemit/k1.h>
#include <spacemit/k3.h>

static const u64 k1_cpu_wakeup_reg[] = {
	PMU_AP_CORE0_WAKEUP,
	PMU_AP_CORE1_WAKEUP,
	PMU_AP_CORE2_WAKEUP,
	PMU_AP_CORE3_WAKEUP,
	PMU_AP_CORE4_WAKEUP,
	PMU_AP_CORE5_WAKEUP,
	PMU_AP_CORE6_WAKEUP,
	PMU_AP_CORE7_WAKEUP,
};

static const u64 k1_cpu_idle_reg[] = {
	PMU_AP_CORE0_IDLE_CFG,
	PMU_AP_CORE1_IDLE_CFG,
	PMU_AP_CORE2_IDLE_CFG,
	PMU_AP_CORE3_IDLE_CFG,
	PMU_AP_CORE4_IDLE_CFG,
	PMU_AP_CORE5_IDLE_CFG,
	PMU_AP_CORE6_IDLE_CFG,
	PMU_AP_CORE7_IDLE_CFG,
};

static inline void spacemit_k1_set_cpu_power(u32 hartid, bool enable)
{
	unsigned int value;
	unsigned int *cpu_idle_base =
		(unsigned int *)(unsigned long)k1_cpu_idle_reg[hartid];

	value = readl(cpu_idle_base);

	if (enable)
		value &= ~PMU_AP_IDLE_PWRDOWN_K1_MASK;
	else
		value |= PMU_AP_IDLE_PWRDOWN_K1_MASK;

	writel(value, cpu_idle_base);
}

static void spacemit_k1_wakeup_cpu(u32 mpidr)
{
	unsigned int *cpu_reset_base;
	unsigned int cur_hartid = current_hartid();

	cpu_reset_base =
		(unsigned int *)(unsigned long)k1_cpu_wakeup_reg[cur_hartid];

	writel(1 << mpidr, cpu_reset_base);
}

static void spacemit_k1_assert_cpu(void)
{
	spacemit_k1_set_cpu_power(current_hartid(), false);
}

static void spacemit_k1_deassert_cpu(unsigned int hartid)
{
	spacemit_k1_set_cpu_power(hartid, true);
}

/* Start (or power-up) the given hart */
static int spacemit_k1_hart_start(unsigned int hartid, unsigned long saddr)
{
	spacemit_k1_deassert_cpu(hartid);
	spacemit_k1_wakeup_cpu(hartid);

	return 0;
}

/*
 * Stop (or power-down) the current hart from running. This call
 * doesn't expect to return if success.
 */
static int spacemit_k1_hart_stop(void)
{
	csr_write(CSR_STIMECMP, GENMASK_ULL(63, 0));
	csr_clear(CSR_MIE, MIP_SSIP | MIP_MSIP | MIP_STIP | MIP_MTIP |
		  MIP_SEIP | MIP_MEIP);

	/* disable data prefetch */
	csr_clear(CSR_MSETUP, MSETUP_PFE);
	asm volatile ("fence iorw, iorw");

	/* flush local dcache */
	csr_write(CSR_MRAOP, MRAOP_CACHE_FLUSH);
	asm volatile ("fence iorw, iorw");

	/* disable dcache */
	csr_clear(CSR_MSETUP, MSETUP_DE);
	asm volatile ("fence iorw, iorw");

	/*
	 * Core4-7 do not have dedicated bits in ML2SETUP;
	 * instead, they reuse the same bits as core0-3.
	 * Therefore, use modulo with PLATFORM_MAX_CPUS_PER_CLUSTER
	 * to select the proper bit.
	 */
	csr_clear(CSR_ML2SETUP,
		  1 << (current_hartid() % PLATFORM_MAX_CPUS_PER_CLUSTER));
	asm volatile ("fence iorw, iorw");

	spacemit_k1_assert_cpu();

	wfi();

	return SBI_ENOTSUPP;
}

static const struct sbi_hsm_device spacemit_k1_hsm_ops = {
	.name		= "spacemit-hsm",
	.hart_start	= spacemit_k1_hart_start,
	.hart_stop	= spacemit_k1_hart_stop,
};

static const unsigned long k3_cluster_idle_reg[] = {
	PMU_CX_CAPMP_IDLE_CFG0,  PMU_CX_CAPMP_IDLE_CFG1,
	PMU_CX_CAPMP_IDLE_CFG2,  PMU_CX_CAPMP_IDLE_CFG3,
	PMU_CX_CAPMP_IDLE_CFG4,  PMU_CX_CAPMP_IDLE_CFG5,
	PMU_CX_CAPMP_IDLE_CFG6,  PMU_CX_CAPMP_IDLE_CFG7,
	PMU_CX_CAPMP_IDLE_CFG8,  PMU_CX_CAPMP_IDLE_CFG9,
	PMU_CX_CAPMP_IDLE_CFG10, PMU_CX_CAPMP_IDLE_CFG11,
	PMU_CX_CAPMP_IDLE_CFG12, PMU_CX_CAPMP_IDLE_CFG13,
	PMU_CX_CAPMP_IDLE_CFG14, PMU_CX_CAPMP_IDLE_CFG15,
};

static const unsigned long k3_core_idle_reg[] = {
	PMU_AP_CORE0_IDLE_CFG,   PMU_AP_CORE1_IDLE_CFG,
	PMU_AP_CORE2_IDLE_CFG,   PMU_AP_CORE3_IDLE_CFG,
	PMU_AP_CORE4_IDLE_CFG,   PMU_AP_CORE5_IDLE_CFG,
	PMU_AP_CORE6_IDLE_CFG,   PMU_AP_CORE7_IDLE_CFG,
	PMU_CAP_CORE8_IDLE_CFG,  PMU_CAP_CORE9_IDLE_CFG,
	PMU_CAP_CORE10_IDLE_CFG, PMU_CAP_CORE11_IDLE_CFG,
	PMU_CAP_CORE12_IDLE_CFG, PMU_CAP_CORE13_IDLE_CFG,
	PMU_CAP_CORE14_IDLE_CFG, PMU_CAP_CORE15_IDLE_CFG,
};

static const unsigned long k3_core_wakeup_reg[] = {
	PMU_AP_CORE0_WAKEUP,     PMU_AP_CORE1_WAKEUP,
	PMU_AP_CORE2_WAKEUP,     PMU_AP_CORE3_WAKEUP,
	PMU_AP_CORE4_WAKEUP,     PMU_AP_CORE5_WAKEUP,
	PMU_AP_CORE6_WAKEUP,     PMU_AP_CORE7_WAKEUP,
	PMU_CAP_CORE8_WAKEUP,    PMU_CAP_CORE9_WAKEUP,
	PMU_CAP_CORE10_WAKEUP,   PMU_CAP_CORE11_WAKEUP,
	PMU_CAP_CORE12_WAKEUP,   PMU_CAP_CORE13_WAKEUP,
	PMU_CAP_CORE14_WAKEUP,   PMU_CAP_CORE15_WAKEUP,
};

static int spacemit_k3_hart_start(u32 hartid, ulong saddr)
{
	u32 value;

	if (hartid >= array_size(k3_core_wakeup_reg))
		return SBI_EINVAL;

	value = readl((void *)k3_cluster_idle_reg[hartid]);
	value &= ~CLUSTER_PWR_DOWN_VALUE;
	writel(value, (void *)k3_cluster_idle_reg[hartid]);

	value = readl((void *)k3_core_idle_reg[hartid]);
	value &= ~PMU_AP_IDLE_PWRDOWN_K3_MASK;
	writel(value, (void *)k3_core_idle_reg[hartid]);

	writel(BIT(hartid), (void *)k3_core_wakeup_reg[hartid]);

	return 0;
}

/* K3 uses this only to power on a hart which has not entered OpenSBI yet. */
static const struct sbi_hsm_device spacemit_k3_hsm_ops = {
	.name		= "spacemit-k3-hsm",
	.hart_start	= spacemit_k3_hart_start,
};

static int spacemit_hsm_probe(const void *fdt, int nodeoff,
			      const struct fdt_match *match)
{
	sbi_hsm_set_device(match->data);

	return 0;
}

static const struct fdt_match spacemit_hsm_match[] = {
	{ .compatible = "spacemit,k1", .data = &spacemit_k1_hsm_ops },
	{ .compatible = "spacemit,k3", .data = &spacemit_k3_hsm_ops },
	{ },
};

const struct fdt_driver fdt_hsm_spacemit = {
	.match_table = spacemit_hsm_match,
	.init = spacemit_hsm_probe,
};
