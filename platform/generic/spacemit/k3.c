/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2026 SpacemiT
 * Authors:
 *   Xianbin Zhu <xianbin.zhu@linux.spacemit.com>
 *   Troy Mitchell <troy.mitchell@linux.spacemit.com>
 */

#include <platform_override.h>
#include <sbi/riscv_asm.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_hfence.h>
#include <sbi/sbi_tlb.h>
#include <spacemit/k3.h>
#include <spacemit/k3_asm.h>
#include <spacemit/spacemit.h>

static const int cci_map[] = {
	PLAT_CCI_K3_AIDMA_IFACE_IX,
	PLAT_CCI_K3_CLUSTER0_IFACE_IX,
	PLAT_CCI_K3_CLUSTER1_IFACE_IX,
	PLAT_CCI_K3_CLUSTER2_0_IFACE_IX,
	PLAT_CCI_K3_CLUSTER2_1_IFACE_IX,
	PLAT_CCI_K3_CLUSTER3_0_IFACE_IX,
	PLAT_CCI_K3_CLUSTER3_1_IFACE_IX,
};

/*
 * The boot hart reaches this function before caches are enabled. Secondary
 * harts perform the same setup in _spacemit_k3_warm_start before entering
 * the common warm-boot path.
 */
static void spacemit_k3_hart_init(u32 hartid)
{
	csr_clear(CSR_PMACFG0,
		  PMACFG0_AUDIO_BUF_FIELD_MASK <<
		  PMACFG0_AUDIO_BUF_FIELD_SHIFT);
	csr_set(CSR_PMACFG0,
		PMACFG0_AUDIO_BUF_ATTR_CACHEABLE <<
		PMACFG0_AUDIO_BUF_FIELD_SHIFT);
	csr_clear(CSR_PMACFG0, PMACFG0_XIP_FIELD_CLEAR);
	csr_set(CSR_PMACFG0, PMACFG0_XIP_ATTR_IO);
	__sbi_sfence_vma_all();

	csr_set(CSR_ML2SETUP,
		_BITUL(hartid % PLATFORM_MAX_CPUS_PER_CLUSTER) |
		ML2SETUP_IPRF | ML2SETUP_TPRF);
	csr_set(CSR_ML2HINT, ML2HINT_TRACE_TOP_ICGEN);
	csr_set(CSR_MSETUP, MSETUP_BOOT_FLAGS);

	if (hartid < PLATFORM_A100_FIRST_ID) {
		/* MISA.H is writable on X100 and not implemented on A100. */
		csr_set(CSR_MISA, MISA_EXT_H);
		__sbi_hfence_gvma_all();
	}
}

/* The boot hart is already running, so it is never started by the HSM. */
static void spacemit_k3_keep_boot_hart_powered(void)
{
	u32 value;

	value = readl((void *)(unsigned long)PMU_CX_CAPMP_IDLE_CFG0);
	value &= ~CLUSTER_PWR_DOWN_VALUE;
	writel(value, (void *)(unsigned long)PMU_CX_CAPMP_IDLE_CFG0);

	value = readl((void *)(unsigned long)PMU_AP_CORE0_IDLE_CFG);
	value &= ~PMU_AP_IDLE_PWRDOWN_K3_MASK;
	writel(value, (void *)(unsigned long)PMU_AP_CORE0_IDLE_CFG);
}

/* Program global state before the HSM starts any secondary hart. */
static void spacemit_k3_pre_init(void)
{
	unsigned long entry = (unsigned long)&_spacemit_k3_warm_start;
	int i;

	writel((u32)entry, (void *)(unsigned long)C0_RVBADDR_LO_ADDR);
	writel((u32)(entry >> 32), (void *)(unsigned long)C0_RVBADDR_HI_ADDR);

	writel((u32)entry, (void *)(unsigned long)C1_RVBADDR_LO_ADDR);
	writel((u32)(entry >> 32), (void *)(unsigned long)C1_RVBADDR_HI_ADDR);

	writel((u32)entry, (void *)(unsigned long)C2_RVBADDR_LO_ADDR);
	writel((u32)(entry >> 32), (void *)(unsigned long)C2_RVBADDR_HI_ADDR);

	writel((u32)entry, (void *)(unsigned long)C3_RVBADDR_LO_ADDR);
	writel((u32)(entry >> 32), (void *)(unsigned long)C3_RVBADDR_HI_ADDR);

	for (i = 0; i < array_size(cci_map); i++)
		cci_enable_snoop_dvm_reqs(cci_map, i);

	spacemit_k3_keep_boot_hart_powered();
}

static int spacemit_k3_early_init(bool cold_boot)
{
	if (cold_boot)
		spacemit_k3_pre_init();

	return generic_early_init(cold_boot);
}

static int spacemit_k3_platform_init(const void *fdt, int nodeoff,
				     const struct fdt_match *match)
{
	spacemit_k3_hart_init(current_hartid());
	generic_platform_ops.early_init = spacemit_k3_early_init;

	return 0;
}

static const struct fdt_match spacemit_k3_match[] = {
	{ .compatible = "spacemit,k3" },
	{ /* sentinel */ }
};

const struct fdt_driver spacemit_k3 = {
	.match_table = spacemit_k3_match,
	.init = spacemit_k3_platform_init,
};
