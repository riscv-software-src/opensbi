/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 Mobileye
 *
 */

#include <platform_override.h>
#include <sbi/riscv_barrier.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_hsm.h>
#include <sbi/sbi_timer.h>
#include <sbi/sbi_hart_pmp.h>
#include <sbi/riscv_io.h>
#include <libfdt.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/fdt/fdt_fixup.h>
#include <mips/p8700.h>
#include <mips/mips-cm.h>

#define MIPS_OLB1		0x67046000
#define MIPS_OLB2		0x67047000
#define MIPS_CM_CTL0		(0x14)
#define MIPS_CTL0_CM_PWR_UP	BIT(0)
#define MIPS_CTL0_DBU_PWR_UP	BIT(1)
#define MIPS_CTL0_CM_RST_HOLD	BIT(2)
#define MIPS_CTL0_DBU_RST_HOLD	BIT(3)
#define MIPS_CTL0_DBU_COLD_PWR_UP	GENMASK(5, 4) /* after cold rst: 00 - pwr down, 01 -clk off */
#define MIPS_CTL0_PARITY_EN	BIT(6)
#define MIPS_CTL0_DBG_RST_DASRT	BIT(7)
#define MIPS_CTL0_CACHE_HW_INIT_INHIBIT	BIT(16)
#define MIPS_CTL0_SW_RESET_N	BIT(17)
#define MIPS_CTL0_CORE_CLK_STS(n)	BIT(28 + (n)) /* n = 0..3 */

#define OLB_ACC0		0x45000000
#define OLB_ACC1		0x65000000
#define OLB_XNN0		0x43600000
#define OLB_XNN1		0x63600000
#define NCORE			0x67800000
#define OLB_WEST		0x48600000
#define OLB_WEST_TSTCSR		0x60
#define TSTCSR_PALLADIUM	BIT(0)
#define TSTCSR_DDR_STUB		BIT(1)
#define TSTCSR_MIPS12_PRESENT	GENMASK(3, 2)
#define TSTCSR_ACC_PRESENT	GENMASK(5, 4)

#define OLB_WEST_CFG		0x68
#define WEST_CFG_MIPS_MTIME_START	BIT(8)

/* Use in nascent init - not have DTB yet */
#define DRAM_ADDRESS		0x800000000UL
#define DRAM_SIZE		0x800000000UL
#define DRAM_PMP_ADDR		((DRAM_ADDRESS >> 2) | ((DRAM_SIZE - 1) >> 3))

#define MMIO_BASE 0x00000000
#define MMIO_SIZE 0x80000000

static int eyeq7h_active_clusters = 1;

static long MIPS_OLB_ADDR[3] = {0, MIPS_OLB1, MIPS_OLB2};

static void eyeq7h_powerup_olb(u32 hartid)
{
	int cl = cpu_cluster(hartid);
	volatile void *cmd;
	u32 temp;

	if (cl < 1 || cl >= p8700_cm_info->num_cm || cl >= array_size(MIPS_OLB_ADDR))
		return;

	/* Get the MIPS_CM_CTL0 address */
	cmd = (volatile void *)(MIPS_OLB_ADDR[cl] + MIPS_CM_CTL0);

	temp = readl(cmd);
	/* Enable HW cache init */
	temp = temp & ~MIPS_CTL0_CACHE_HW_INIT_INHIBIT;
	/* deassert reset */
	temp = temp | MIPS_CTL0_SW_RESET_N;
	writel(temp, cmd);
	wmb();
	/* TODO: using CPU clock ready as reset complete indication, is it correct? */
	while(!(readl(cmd) & MIPS_CTL0_CORE_CLK_STS(0)))
			cpu_relax();
}

static void eyeq7h_power_up_other_cluster(u32 hartid)
{
	unsigned int cl = cpu_cluster(hartid);
	unsigned long cm_base = p8700_cm_info->gcr_base[cl];

	/* Power up MIPS OLB */
	eyeq7h_powerup_olb(hartid);
	/* remap local cluster address to its global address */
	writeq(cm_base, (void*)cm_base + GCR_BASE_OFFSET);
	wmb();
	mips_p8700_power_up_other_cluster(hartid);
}

static int hart_start(u32 hartid, ulong saddr)
{
	if (cpu_cluster(hartid) >= eyeq7h_active_clusters) {
		sbi_printf("Requested CPU 0x%x in inactive/nonexistent cluster\n", hartid);
		return SBI_EINVALID_ADDR;
	}

	return mips_p8700_hart_start(hartid, saddr);
}

static const struct sbi_hsm_device eyeq7h_hsm = {
	.name		= "eyeq7h_hsm",
	.hart_start	= hart_start,
	.hart_stop	= mips_p8700_hart_stop,
};

static struct sbi_domain_memregion *find_last_memregion(const struct sbi_domain *dom)
{
	struct sbi_domain_memregion *reg;

	sbi_domain_for_each_memregion(dom, reg) {}
	return --reg;
}

static int fixup_dram_region(const struct sbi_domain *dom,
			     struct sbi_domain_memregion *reg)
{
	const void *fdt = fdt_get_address();
	int node;
	int ret;
	uint64_t mem_addr, mem_size;
	static const char mem_str[] = "memory";

	if (!reg || !fdt)
		return SBI_EINVAL;

	/* Find the memory range */
	node = fdt_node_offset_by_prop_value(fdt, -1, "device_type",
					     mem_str, sizeof(mem_str));
	ret = fdt_get_node_addr_size(fdt, node, 0, &mem_addr, &mem_size);
	if (ret)
		return ret;
	reg->flags = SBI_DOMAIN_MEMREGION_MMIO; /* disable cache & prefetch */
	return sbi_domain_root_add_memrange(mem_addr, mem_size, mem_size,
					    (SBI_DOMAIN_MEMREGION_SU_READABLE |
					     SBI_DOMAIN_MEMREGION_SU_WRITABLE |
					     SBI_DOMAIN_MEMREGION_SU_EXECUTABLE));
}

static void fdt_disable_by_compat(void *fdt, const char *compatible)
{
	int node = 0;

	while ((node = fdt_node_offset_by_compatible(fdt, node, compatible)) >= 0)
		fdt_setprop_string(fdt, node, "status", "disabled");
}

/**
 * p8700_acc_clusters_do_fixup() - detect present accelerator clusters
 *
 * Detect what accelerator clusters are actually present in design and
 * disable missed ones. Same bit indicates presence of the ACC and XNN
 * clusters
 */
static void eyeq7h_acc_clusters_do_fixup(struct fdt_general_fixup *f, void *fdt)
{
	u32 tstcsr = readl((void*)OLB_WEST + OLB_WEST_TSTCSR);
	u32 acc01_present = EXTRACT_FIELD(tstcsr, TSTCSR_ACC_PRESENT);
	static const char YN[2] = {'N', 'Y'};

	sbi_dprintf("OLB indicates ACC clusters[01] = [%c%c]\n",
		    YN[acc01_present & BIT(0)],
		    YN[(acc01_present >> 1) & BIT(0)]);

	/* if accelerators present, correspondend OLBS present too */
	/* deassert cluster resets for accelerators and XNN */
	if (!(acc01_present & BIT(0))) {
		sbi_dprintf("Disable ACC0\n");
		fdt_disable_by_compat(fdt, "mobileye,eyeq7h-acc0-olb");
		fdt_disable_by_compat(fdt, "mobileye,eyeq7h-xnn0-olb");
	} else {
		writel(0xff, (void*)OLB_ACC0 + 0x60);
		writel(0xff, (void*)OLB_ACC0 + 0x64);
		writel(0x7f, (void*)OLB_XNN0 + 0x60);
		writel(0x7f, (void*)OLB_XNN0 + 0x64);
	}
	if (!(acc01_present & BIT(1))) {
		sbi_dprintf("Disable ACC1\n");
		fdt_disable_by_compat(fdt, "mobileye,eyeq7h-acc1-olb");
		fdt_disable_by_compat(fdt, "mobileye,eyeq7h-xnn1-olb");
	} else {
		writel(0xff, (void*)OLB_ACC1 + 0x60);
		writel(0xff, (void*)OLB_ACC1 + 0x64);
		writel(0x7f, (void*)OLB_XNN1 + 0x60);
		writel(0x7f, (void*)OLB_XNN1 + 0x64);
	}
}

static struct fdt_general_fixup eyeq7h_acc_clusters_fixup = {
	.name = "acc-clusters-fixup",
	.do_fixup = eyeq7h_acc_clusters_do_fixup,
};

static void eyeq7h_cache_do_fixup(struct fdt_general_fixup *f, void *fdt)
{
	struct p8700_cache_info l1d, l1i, l2;
	mips_p8700_cache_info(&l1d, &l1i, &l2);

	sbi_dprintf("Cache geometry:\n"
		    "  D: %4d Kbytes line %3d bytes %2d ways %5d sets\n"
		    "  I: %4d Kbytes line %3d bytes %2d ways %5d sets\n",
		    l1d.assoc_ways * l1d.line * l1d.sets / 1024,
		    l1d.line, l1d.assoc_ways, l1d.sets,
		    l1i.assoc_ways * l1i.line * l1i.sets / 1024,
		    l1i.line, l1i.assoc_ways, l1i.sets);
	if (l2.line) {
		sbi_dprintf(" L2: %4d Kbytes line %3d bytes %2d ways %5d sets\n",
			    l2.assoc_ways * l2.line * l2.sets / 1024,
			    l2.line, l2.assoc_ways, l2.sets);
	} else {
		sbi_dprintf(" L2: not present\n");
	}
}

static struct fdt_general_fixup eyeq7h_cache_fixup = {
	.name = "cache-fixup",
	.do_fixup = eyeq7h_cache_do_fixup,
};

static int eyeq7h_final_init(bool cold_boot)
{
	if (!cold_boot)
		return 0;

	sbi_hsm_set_device(&eyeq7h_hsm);
	fdt_register_general_fixup(&eyeq7h_acc_clusters_fixup);
	fdt_register_general_fixup(&eyeq7h_cache_fixup);

	return generic_final_init(cold_boot);
}

/**
 * There's 2 sources of information what clusters are present:
 * - GCR_CONFIG register from the cluster 0 GCR
 * - TSTCSR_MIPS12_PRESENT from the TSTCSR reg in OLB_WEST
 * check that both indicates presence of clusters
 */
static void eyeq7h_init_clusters(void)
{
	unsigned long cm_base = p8700_cm_info->gcr_base[0];
	u64 gcr_config = readq((void*)cm_base + GCR_GLOBAL_CONFIG);
	int num_clusters = EXTRACT_FIELD(gcr_config, GCR_GC_NUM_CLUSTERS);
	u32 tstcsr = readl((void*)OLB_WEST + OLB_WEST_TSTCSR);
	u32 mips12_present = EXTRACT_FIELD(tstcsr, TSTCSR_MIPS12_PRESENT);
	/**
	 * total clusters, by mips[12] encoding.
	 * Don't support only mips2 present, consider as 1 total cluster
	 */
	static const int olb_clusters[4] = {1, 2, 1, 3};
	int num_olb_clusters = olb_clusters[mips12_present];
	static const char YN[2] = {'N', 'Y'};

	sbi_dprintf("GCR_CONFIG reports %d clusters\n", num_clusters);
	sbi_dprintf("OLB indicates %d clusters, mips[12] = [%c%c]\n",
		    num_olb_clusters,
		    YN[mips12_present & BIT(0)],
		    YN[mips12_present >> 1 & BIT(0)]);
	if (num_clusters > num_olb_clusters)
		num_clusters = num_olb_clusters;
	sbi_dprintf("Use %d clusters\n", num_clusters);
	/* Power up other clusters in the platform. */
	for (int i = 1; i < num_clusters; i++) {
		eyeq7h_power_up_other_cluster(INSERT_FIELD(0, P8700_HARTID_CLUSTER, i));
	}
	eyeq7h_active_clusters = num_clusters;
	/**
	 * sync timers in all clusters. EQ7 have counters restart pins for clusters
	 * connected to the OLB.
	 * Stop/arm all counters, then restart all at once
	 */
	for (int i = 0; i < num_clusters; i++) {
		write_cpc_timectl(INSERT_FIELD(0, P8700_HARTID_CLUSTER, i),
				  TIMECTL_HARMED | TIMECTL_HSTOP | TIMECTL_MARMED | TIMECTL_MSTOP);
	}
	{
		u32 cfg = readl((void*)OLB_WEST + OLB_WEST_CFG);

		writel(cfg | WEST_CFG_MIPS_MTIME_START, (void*)OLB_WEST + OLB_WEST_CFG);
	}
}

static int eyeq7h_early_init(bool cold_boot)
{
	const struct sbi_domain *dom;
	struct sbi_domain_memregion *reg;
	int rc;
	unsigned long cm_base;

	rc = generic_early_init(cold_boot);
	if (rc)
		return rc;

	if (!cold_boot)
		return 0;

	cm_base = p8700_cm_info->gcr_base[0];
	sbi_dprintf("Remap Cluster %d CM 0x%lx -> 0x%lx\n", 0,
		    readq((void*)cm_base + GCR_BASE_OFFSET),
		    cm_base);
	writeq(cm_base, (void*)cm_base + GCR_BASE_OFFSET);
	wmb();
	eyeq7h_init_clusters();
/**
 * Memory map:
 * 0x00_20080000  0x00_20100000   M:IRW- S:---- GCR local access (CM_BASE)
 * 0x00_40000000  0x00_70000000   M:IRW- S:IRW- Peripherals
 *   0x00_48700000  0x00_48780000 M:IRW- S:---- GCR cluster 0
 *   0x00_67480000  0x00_67500000 M:IRW- S:---- GCR cluster 1
 *   0x00_67500000  0x00_67580000 M:IRW- S:---- GCR cluster 2
 *   0x00_67800000  0x00_67900000 M:IRW- S:---- Ncore
 * 0x00_70000000  0x00_80000000   M:---- S:IRW- PCI32 BARs, NOT USED - 32-bit mode
 * 0x01_00000000  0x08_00000000   M:---- S:IRW- PCI64 BARs, NOT USED - PCI2PCI
 * 0x08_00000000  0x10_00000000   M:---- S:-RWX DDR64
 * 0x10_00000000  0x20_00000000   M:---- S:IRW- PCI64 BARs
 */
	rc = mips_p8700_add_memranges();
	if (rc)
		return rc;
	/* the rest of MMIO - shared with S-mode */
	rc = sbi_domain_root_add_memrange(MMIO_BASE, MMIO_SIZE, MMIO_SIZE,
					  SBI_DOMAIN_MEMREGION_MMIO |
					  SBI_DOMAIN_MEMREGION_SHARED_SURW_MRW);
	if (rc)
		return rc;
	/* PCIE BARs - MMIO S-mode */
	rc = sbi_domain_root_add_memrange(0x1000000000UL, 0x1000000000UL, 0x1000000000UL,
					  SBI_DOMAIN_MEMREGION_MMIO |
					  SBI_DOMAIN_MEMREGION_SU_READABLE |
					  SBI_DOMAIN_MEMREGION_SU_WRITABLE);

	/*
	 * sbi_domain_init adds last "all-inclusive" memory region
	 * 0 .. ~0 RWX
	 * Find this region (it is the last one) and update size according to DRAM
	 */
	dom = sbi_domain_thishart_ptr();
	reg = find_last_memregion(dom);
	return fixup_dram_region(dom, reg);
}

static int eyeq7h_nascent_init(void)
{
	unsigned hartid = current_hartid();
	unsigned cl = cpu_cluster(hartid);
	unsigned long cm_base = p8700_cm_info->gcr_base[cl];
	int i;

	/* Coherence enable for every core */
	if (cpu_hart(hartid) == 0) {
		cm_base += (cpu_core(hartid) << CM_BASE_CORE_SHIFT);
		__raw_writeq(GCR_CORE_COH_EN_EN,
			     (void *)(cm_base + GCR_OFF_LOCAL +
				      GCR_CORE_COH_EN));
		mb();
	}

	/**
	 * Boot code set PMP14 and PMP15 to allow basic cacheable and uncacheable
	 * access.
	 * To avoid hang during PMP count detection, set up PMP13 same as PMP14
	 * Point is, PMP count detection procedure tries to write every PMP
	 * entry with maximum allowed value, then return original value.
	 * If memory covered only by PMP14, when it is written, next instruction
	 * fetch will fail. Use PMP13 as a back-up for PMP14. When PMP13 tested,
	 * PMP14 will serve the memory access; when PMP14 tested, PMP13 will
	 * provide memory access
	 */
	/* Set up pmp for DRAM */
	csr_write(CSR_PMPADDR13, DRAM_PMP_ADDR);
	csr_write(CSR_PMPADDR14, DRAM_PMP_ADDR);
	/*
	 * FIXME: for unknown reason if I copy PMP14 to PMP13 using
	 * csr_write(CSR_PMPADDR13, csr_read(CSR_PMPADDR14));
	 * instead of writing as above, system hangs on late Linux boot
	 */
	/* All from 0x0 */
	csr_write(CSR_PMPADDR15, 0x1fffffffffffffff);
	csr_write(CSR_PMPCFG2, ((PMP_A_NAPOT|PMP_R|PMP_W|PMP_X)<<56) |
		  ((PMP_A_NAPOT|PMP_R|PMP_W|PMP_X)<<48) |
		  ((PMP_A_NAPOT|PMP_R|PMP_W|PMP_X)<<40));
	/* Set cacheable for pmp13/pmp14, uncacheable for pmp15 */
	csr_write(CSR_MIPSPMACFG2, ((u64)CCA_CACHE_DISABLE << 56) |
		  ((u64)CCA_CACHE_ENABLE << 48) |
		  ((u64)CCA_CACHE_ENABLE << 40));
	/* Reset pmpcfg0 */
	csr_write(CSR_PMPCFG0, 0);
	/* Reset pmacfg0 */
	csr_write(CSR_MIPSPMACFG0, 0);
	mb();

	/* Per cluster set up */
	if (cpu_core(hartid) == 0 && cpu_hart(hartid) == 0) {
		/* Enable L2 prefetch */
		__raw_writel(0xfffff110,
			     (void *)(cm_base + L2_PFT_CONTROL_OFFSET));
		__raw_writel(0x15ff,
			     (void *)(cm_base + L2_PFT_CONTROL_B_OFFSET));
		/*
		 * Remove access to NCORE CSRs from mmio region 1
		 * which is routed to AUX. NCORE to use default route through MEM.
		 */
		__raw_writeq(NCORE-1, (void *)(cm_base + GCR_MMIO_TOP(1)));
		mb();
		mips_p8700_dump_mmio();
	}

	/* Per core set up */
	if (cpu_hart(hartid) == 0) {
		/* Enable load pair, store pair, and HTW */
		csr_clear(CSR_MIPSCONFIG7, (1<<12)|(1<<13)|(1<<7));

		/* Disable noRFO, misaligned load/store */
		csr_set(CSR_MIPSCONFIG7, (1<<25)|(1<<9));

		/* Enable L1-D$ Prefetch */
		csr_write(CSR_MIPSCONFIG11, 0xff);

		for (i = 0; i < 8; i++) {
			csr_set(CSR_MIPSCONFIG8, 4 + 0x100 * i);
			csr_set(CSR_MIPSCONFIG9, 8);
			mb();
			RISCV_FENCE_I;
		}
	}

	/* Per hart set up */
	/* Enable AMO and RDTIME illegal instruction exceptions. */
	csr_set(CSR_MIPSCONFIG6, (1<<2)|(1<<1));
	/* enable ECC for L1 I/D and FTLB */
	csr_set(CSR_MIPSERRCTL, MIPSERRCTL_PE);

	return 0;
}

static int eyeq7h_platform_init(const void *fdt, int nodeoff, const struct fdt_match *match)
{
	int rc = mips_p8700_platform_init(fdt, nodeoff, match);

	if (rc)
		return rc;
	generic_platform_ops.early_init = eyeq7h_early_init;
	generic_platform_ops.final_init = eyeq7h_final_init;
	generic_platform_ops.nascent_init = eyeq7h_nascent_init;
	generic_platform_ops.pmp_set = mips_p8700_pmp_set;

	return 0;
}

static unsigned long eyeq7h_gcr_base[] = {
	0x48700000,
	0x67480000,
	0x67500000,
};

static struct p8700_cm_info eyeq7h_cm_info = {
	.num_cm = array_size(eyeq7h_gcr_base),
	.gcr_base = eyeq7h_gcr_base,
};

static const struct fdt_match eyeq7h_match[] = {
	{ .compatible = "mobileye,eyeq7h", .data = &eyeq7h_cm_info },
	{ },
};

const struct fdt_driver mips_p8700_eyeq7h = {
	.match_table = eyeq7h_match,
	.init = eyeq7h_platform_init,
};
