/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 MIPS
 *
 */

#include <platform_override.h>
#include <sbi/riscv_barrier.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_hsm.h>
#include <sbi/sbi_timer.h>
#include <sbi/riscv_io.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <mips/p8700.h>
#include <mips/mips-cm.h>

const struct p8700_cm_info *p8700_cm_info;

extern void mips_warm_boot(void);
#define MMIO_BASE 0x00000000
#define MMIO_SIZE 0x80000000

/* FIXME! Please change GLOBAL_CM_BASE for your platform */
long GLOBAL_CM_BASE[CLUSTERS_IN_PLATFORM] = {CM_BASE};

static void mips_p8700_pmp_set(unsigned int n, unsigned long flags,
			       unsigned long prot, unsigned long addr,
			       unsigned long log2len)
{
	int pmacfg_csr, pmacfg_shift;
	unsigned long cfgmask;
	unsigned long pmacfg, cca;

	pmacfg_csr = (CSR_MIPSPMACFG0 + (n >> 2)) & ~1;
	pmacfg_shift = (n & 7) << 3;
	cfgmask = ~(0xffUL << pmacfg_shift);

	/* Read pmacfg to change cacheability */
	pmacfg = (csr_read_num(pmacfg_csr) & cfgmask);
	cca = (flags & SBI_DOMAIN_MEMREGION_MMIO) ? CCA_CACHE_DISABLE :
				  CCA_CACHE_ENABLE | PMA_SPECULATION;
	pmacfg |= ((cca << pmacfg_shift) & ~cfgmask);
	csr_write_num(pmacfg_csr, pmacfg);
}

static void power_up_other_cluster(u32 hartid)
{
	unsigned int cl = cpu_cluster(hartid);

	/* remap local cluster address to its global address */
	writeq(GLOBAL_CM_BASE[cl], (void*)GLOBAL_CM_BASE[cl] + GCR_BASE_OFFSET);
	wmb();
	/* Power up CM in cluster */
	write_cpc_pwrup_ctl(hartid, 1);

	/* Wait for the CM to start up */
	for (int i = 100; i > 0; i--) {
		u32 stat = read_cpc_cm_stat_conf(hartid);

		stat = EXT(stat, CPC_Cx_STAT_CONF_SEQ_STATE);
		if (stat == CPC_Cx_STAT_CONF_SEQ_STATE_U5)
			return;
		cpu_relax();
	}
	sbi_printf("ERROR: Fail to power up cluster %u\n", cl);
}


struct mips_boot_params {
	u32 hartid;
	u32 target_state;
};

static bool mips_hart_reached_state(void *arg)
{
	struct mips_boot_params *p = arg;
	u32 stat = read_cpc_co_stat_conf(p->hartid);

	stat = EXT(stat, CPC_Cx_STAT_CONF_SEQ_STATE);
	return stat == p->target_state;
}

static int mips_hart_start(u32 hartid, ulong saddr)
{
	/* Hart 0 is the boot hart, and we don't use the CPC cmd to start.  */
	if (hartid == 0)
		return SBI_ENOTSUPP;

	/* Change reset base to mips_warm_boot */
	write_gcr_co_reset_base(hartid, (unsigned long)mips_warm_boot);

	if (cpu_hart(hartid) == 0) {
		unsigned int const timeout_ms = 10;
		bool booted;
		struct mips_boot_params p = {
			.hartid = hartid,
			.target_state = CPC_Cx_STAT_CONF_SEQ_STATE_U6,
		};

		/* Ensure its coherency is disabled */
		write_gcr_co_coherence(hartid, 0);

		/* Start cluster cl core co hart 0 */
		write_cpc_co_vp_run(hartid, 1 << cpu_hart(hartid));

		/* Reset cluster cl core co hart 0 */
		write_cpc_co_cmd(hartid, CPC_Cx_CMD_RESET);

		booted = sbi_timer_waitms_until(mips_hart_reached_state, &p, timeout_ms);
		if (!booted) {
			sbi_printf("ERROR: failed to boot hart 0x%x in %d ms\n",
				   hartid, timeout_ms);
			return -SBI_ETIMEDOUT;
		}
	} else {
		write_cpc_co_vp_run(hartid, 1 << cpu_hart(hartid));
	}

	return 0;
}

static int mips_hart_stop()
{
	u32 hartid = current_hartid();

	/* Hart 0 is the boot hart, and we don't use the CPC cmd to stop.  */
	if (hartid == 0)
		return SBI_ENOTSUPP;

	write_cpc_co_vp_stop(hartid, 1 << cpu_hart(hartid));

	return 0;
}

static const struct sbi_hsm_device mips_hsm = {
	.name		= "mips_hsm",
	.hart_start	= mips_hart_start,
	.hart_stop	= mips_hart_stop,
};

static int mips_p8700_final_init(bool cold_boot)
{
	if (cold_boot)
		sbi_hsm_set_device(&mips_hsm);

	return generic_final_init(cold_boot);
}

static int mips_p8700_early_init(bool cold_boot)
{
	int rc;
	int i;

	rc = generic_early_init(cold_boot);
	if (rc)
		return rc;

	if (!cold_boot)
		return 0;

	sbi_dprintf("Remap Cluster %d CM 0x%lx -> 0x%lx\n", 0,
		    readq((void*)GLOBAL_CM_BASE[0] + GCR_BASE_OFFSET),
		    GLOBAL_CM_BASE[0]);
	writeq(GLOBAL_CM_BASE[0], (void*)GLOBAL_CM_BASE[0] + GCR_BASE_OFFSET);
	wmb();
	/* Power up other clusters in the platform. */
	for (i = 1; i < CLUSTERS_IN_PLATFORM; i++) {
		power_up_other_cluster(i << NEW_CLUSTER_SHIFT);
	}

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

	for (i = 0; i < CLUSTERS_IN_PLATFORM; i++) {
		unsigned long cm_base = GLOBAL_CM_BASE[i];

		/* CM and MTIMER */
		rc = sbi_domain_root_add_memrange(cm_base, SIZE_FOR_CPC_MTIME,
						  SIZE_FOR_CPC_MTIME,
						  (SBI_DOMAIN_MEMREGION_MMIO |
						   SBI_DOMAIN_MEMREGION_M_READABLE |
						   SBI_DOMAIN_MEMREGION_M_WRITABLE));
		if (rc)
			return rc;

		/* For the APLIC and ACLINT m-mode region */
		rc = sbi_domain_root_add_memrange(cm_base + AIA_OFFSET, SIZE_FOR_AIA_M_MODE,
						  SIZE_FOR_AIA_M_MODE,
						  (SBI_DOMAIN_MEMREGION_MMIO |
						   SBI_DOMAIN_MEMREGION_M_READABLE |
						   SBI_DOMAIN_MEMREGION_M_WRITABLE));
		if (rc)
			return rc;
	}
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

	return 0;
}

static int mips_p8700_nascent_init(void)
{
	u64 hartid = current_hartid();
	int cl = cpu_cluster(hartid);
	u64 cm_base = GLOBAL_CM_BASE[cl];
	int i;

	/* Coherence enable for every core */
	if (cpu_hart(hartid) == 0) {
		cm_base += (cpu_core(hartid) << CM_BASE_CORE_SHIFT);
		__raw_writeq(GCR_CORE_COH_EN_EN,
			     (void *)(cm_base + GCR_OFF_LOCAL +
				      GCR_CORE_COH_EN));
		mb();
	}

	/* Set up pmp for DRAM */
	csr_write(CSR_PMPADDR14, DRAM_PMP_ADDR);
	/* All from 0x0 */
	csr_write(CSR_PMPADDR15, 0x1fffffffffffffff);
	csr_write(CSR_PMPCFG2, ((PMP_A_NAPOT|PMP_R|PMP_W|PMP_X)<<56)|
		  ((PMP_A_NAPOT|PMP_R|PMP_W|PMP_X)<<48));
	/* Set cacheable for pmp6, uncacheable for pmp7 */
	csr_write(CSR_MIPSPMACFG2, ((u64)CCA_CACHE_DISABLE << 56)|
		  ((u64)CCA_CACHE_ENABLE << 48));
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

	return 0;
}

static int mips_p8700_platform_init(const void *fdt, int nodeoff, const struct fdt_match *match)
{
	const struct p8700_cm_info *data = match->data;

	if (!data) {
		sbi_printf("Missing CM info for %s\n", match->compatible);
		return SBI_EINVAL;
	}

	p8700_cm_info = data;
	generic_platform_ops.early_init = mips_p8700_early_init;
	generic_platform_ops.final_init = mips_p8700_final_init;
	generic_platform_ops.nascent_init = mips_p8700_nascent_init;
	generic_platform_ops.pmp_set = mips_p8700_pmp_set;

	return 0;
}

static unsigned long mips_p8700_gcr_base[] = {
	0x16100000,
};

static struct p8700_cm_info mips_p8700_cm_info = {
	.num_cm = array_size(mips_p8700_gcr_base),
	.gcr_base = mips_p8700_gcr_base,
};

static const struct fdt_match mips_p8700_match[] = {
	{ .compatible = "mips,p8700", .data = &mips_p8700_cm_info },
	{ },
};

const struct fdt_driver mips_p8700 = {
	.match_table = mips_p8700_match,
	.init = mips_p8700_platform_init,
};
