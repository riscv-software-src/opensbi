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
#include <sbi_utils/fdt/fdt_helper.h>
#include <mips/p8700.h>
#include <mips/mips-cm.h>

extern void mips_warm_boot(void);

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

#if CLUSTERS_IN_PLATFORM > 1
static void power_up_other_cluster(u32 hartid)
{
	unsigned int stat;
	unsigned int timeout;
	bool local_p = (cpu_cluster(current_hartid()) == cpu_cluster(hartid));

	/* Power up cluster cl core 0 hart 0 */
	write_cpc_pwrup_ctl(hartid, 1, local_p);

	/* Wait for the CM to start up */
	timeout = 100;
	while (true) {
		stat = read_cpc_cm_stat_conf(hartid, local_p);
		stat = EXT(stat, CPC_Cx_STAT_CONF_SEQ_STATE);
		if (stat == CPC_Cx_STAT_CONF_SEQ_STATE_U5)
			break;

		/* Delay a little while before we start warning */
		if (timeout) {
			sbi_dprintf("Delay a little while before we start warning\n");
			timeout--;
		}
		else {
			sbi_printf("Waiting for cluster %u CM to power up... STAT_CONF=0x%x\n",
				   cpu_cluster(hartid), stat);
			break;
		}
	}
}
#endif

static int mips_hart_start(u32 hartid, ulong saddr)
{
	unsigned int stat;
	unsigned int timeout;
	bool local_p = (cpu_cluster(current_hartid()) == cpu_cluster(hartid));

	/* Hart 0 is the boot hart, and we don't use the CPC cmd to start.  */
	if (hartid == 0)
		return SBI_ENOTSUPP;

	/* Change reset base to mips_warm_boot */
	write_gcr_co_reset_base(hartid, (unsigned long)mips_warm_boot, local_p);

	if (cpu_hart(hartid) == 0) {
		/* Ensure its coherency is disabled */
		write_gcr_co_coherence(hartid, 0, local_p);

		/* Start cluster cl core co hart 0 */
		write_cpc_co_vp_run(hartid, 1 << cpu_hart(hartid), local_p);

		/* Reset cluster cl core co hart 0 */
		write_cpc_co_cmd(hartid, CPC_Cx_CMD_RESET, local_p);

		timeout = 100;
		while (true) {
			stat = read_cpc_co_stat_conf(hartid, local_p);
			stat = EXT(stat, CPC_Cx_STAT_CONF_SEQ_STATE);
			if (stat == CPC_Cx_STAT_CONF_SEQ_STATE_U6)
				break;

			/* Delay a little while before we start warning */
			if (timeout) {
				sbi_timer_mdelay(10);
				timeout--;
			}
			else {
				sbi_printf("Waiting for cluster %u core %u hart %u to start... STAT_CONF=0x%x\n",
					   cpu_cluster(hartid),
					   cpu_core(hartid), cpu_hart(hartid),
					   stat);
				break;
			}
		}
	}
	else {
		write_cpc_co_vp_run(hartid, 1 << cpu_hart(hartid), local_p);
	}

	return 0;
}

static int mips_hart_stop()
{
	u32 hartid = current_hartid();
	bool local_p = (cpu_cluster(current_hartid()) == cpu_cluster(hartid));

	/* Hart 0 is the boot hart, and we don't use the CPC cmd to stop.  */
	if (hartid == 0)
		return SBI_ENOTSUPP;

	write_cpc_co_vp_stop(hartid, 1 << cpu_hart(hartid), local_p);

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

	rc = generic_early_init(cold_boot);
	if (rc)
		return rc;

	if (cold_boot) {
#if CLUSTERS_IN_PLATFORM > 1
		int i;
		/* Power up other clusters in the platform. */
		for (i = 1; i < CLUSTERS_IN_PLATFORM; i++) {
			power_up_other_cluster(i << NEW_CLUSTER_SHIFT);
		}
#endif

		/* For the CPC mtime region, the minimum size is 0x10000. */
		rc = sbi_domain_root_add_memrange(CM_BASE, SIZE_FOR_CPC_MTIME,
						  P8700_ALIGN,
						  (SBI_DOMAIN_MEMREGION_MMIO |
						   SBI_DOMAIN_MEMREGION_M_READABLE |
						   SBI_DOMAIN_MEMREGION_M_WRITABLE));
		if (rc)
			return rc;

		/* For the APLIC and ACLINT m-mode region */
		rc = sbi_domain_root_add_memrange(AIA_BASE, SIZE_FOR_AIA_M_MODE,
						  P8700_ALIGN,
						  (SBI_DOMAIN_MEMREGION_MMIO |
						   SBI_DOMAIN_MEMREGION_M_READABLE |
						   SBI_DOMAIN_MEMREGION_M_WRITABLE));
		if (rc)
			return rc;

#if CLUSTERS_IN_PLATFORM > 1
		for (i = 0; i < CLUSTERS_IN_PLATFORM; i++) {
			/* For the CPC mtime region, the minimum size is 0x10000. */
			rc = sbi_domain_root_add_memrange(GLOBAL_CM_BASE[i], SIZE_FOR_CPC_MTIME,
							  P8700_ALIGN,
							  (SBI_DOMAIN_MEMREGION_MMIO |
							   SBI_DOMAIN_MEMREGION_M_READABLE |
							   SBI_DOMAIN_MEMREGION_M_WRITABLE));
			if (rc)
				return rc;

			/* For the APLIC and ACLINT m-mode region */
			rc = sbi_domain_root_add_memrange(AIA_BASE - CM_BASE + GLOBAL_CM_BASE[i], SIZE_FOR_AIA_M_MODE,
							  P8700_ALIGN,
							  (SBI_DOMAIN_MEMREGION_MMIO |
							   SBI_DOMAIN_MEMREGION_M_READABLE |
							   SBI_DOMAIN_MEMREGION_M_WRITABLE));
			if (rc)
				return rc;
		}
#endif
	}

	return 0;
}

static int mips_p8700_nascent_init(void)
{
	u64 hartid = current_hartid();
	u64 cm_base = CM_BASE;
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
	generic_platform_ops.early_init = mips_p8700_early_init;
	generic_platform_ops.final_init = mips_p8700_final_init;
	generic_platform_ops.nascent_init = mips_p8700_nascent_init;
	generic_platform_ops.pmp_set = mips_p8700_pmp_set;

	return 0;
}

static const struct fdt_match mips_p8700_match[] = {
	{ .compatible = "mips,p8700" },
	{ },
};

const struct fdt_driver mips_p8700 = {
	.match_table = mips_p8700_match,
	.init = mips_p8700_platform_init,
};
