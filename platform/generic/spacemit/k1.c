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

/* only use 0-1 cluster in SpacemiT K1 */
static const int cci_map[] = {
	PLAT_CCI_CLUSTER0_IFACE_IX,
	PLAT_CCI_CLUSTER1_IFACE_IX,
};

static void cci_enable_snoop_dvm_reqs(unsigned int master_id)
{
	int slave_if_id = cci_map[master_id];

	/*
	 * Enable Snoops and DVM messages, no need for Read/Modify/Write as
	 * rest of bits are write ignore
	 */
	writel(CCI_550_SNOOP_CTRL_ENABLE_DVMS | CCI_550_SNOOP_CTRL_ENABLE_SNOOPS,
	       (void *)(u64)CCI_550_PLATFORM_CCI_ADDR +
	       CCI_550_SLAVE_IFACE_OFFSET(slave_if_id) + CCI_550_SNOOP_CTRL);

	/*
	 * Wait for the completion of the write to the Snoop Control Register
	 * before testing the change_pending bit
	 */
	mb();

	/* Wait for the dust to settle down */
	while ((readl((void *)(u64)CCI_550_PLATFORM_CCI_ADDR + CCI_550_STATUS) &
	       CCI_550_STATUS_CHANGE_PENDING))
		;
}

static void spacemit_k1_pre_init(void)
{
	unsigned int clusterid, cluster_enabled = 0;
	struct sbi_scratch *scratch;
	int i;

	scratch = sbi_scratch_thishart_ptr();

	writel(scratch->warmboot_addr, (unsigned int *)C0_RVBADDR_LO_ADDR);
	writel((u64)scratch->warmboot_addr >> 32, (unsigned int *)C0_RVBADDR_HI_ADDR);

	writel(scratch->warmboot_addr, (unsigned int *)C1_RVBADDR_LO_ADDR);
	writel((u64)scratch->warmboot_addr >> 32, (unsigned int *)C1_RVBADDR_HI_ADDR);

	for (i = 0; i < PLATFORM_MAX_CPUS; i++) {
		clusterid = CPU_TO_CLUSTER(i);

		if (!(cluster_enabled & (1 << clusterid))) {
			cluster_enabled |= 1 << clusterid;
			cci_enable_snoop_dvm_reqs(clusterid);
		}
	}
}

/*
 * Platform early initialization.
 */
static int spacemit_k1_early_init(bool cold_boot)
{
	int rc;

	rc = generic_early_init(cold_boot);
	if (rc)
		return rc;

	csr_set(CSR_MSETUP, MSETUP_DE | MSETUP_IE | MSETUP_BPE |
		MSETUP_PFE | MSETUP_MME | MSETUP_ECCE);

	if (cold_boot)
		spacemit_k1_pre_init();

	return 0;
}

static bool spacemit_cold_boot_allowed(u32 hartid)
{
	csr_set(CSR_ML2SETUP, 1 << (hartid % PLATFORM_MAX_CPUS_PER_CLUSTER));

	return !hartid;
}

static int spacemit_k1_platform_init(const void *fdt, int nodeoff,
				     const struct fdt_match *match)
{
	generic_platform_ops.early_init = spacemit_k1_early_init;
	generic_platform_ops.cold_boot_allowed = spacemit_cold_boot_allowed;

	return 0;
}

static const struct fdt_match spacemit_k1_match[] = {
	{ .compatible = "spacemit,k1" },
	{ /* sentinel */ }
};

const struct fdt_driver spacemit_k1 = {
	.match_table = spacemit_k1_match,
	.init = spacemit_k1_platform_init,
};
