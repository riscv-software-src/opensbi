/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 MIPS
 *
 */

#include <sbi/riscv_io.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_hsm.h>
#include <sbi/sbi_timer.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <mips/p8700.h>
#include <mips/mips-cm.h>

const struct p8700_cm_info *p8700_cm_info;

void mips_p8700_pmp_set(unsigned int n, unsigned long flags,
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

void mips_p8700_power_up_other_cluster(u32 hartid)
{
	unsigned int cl = cpu_cluster(hartid);

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

extern void mips_warm_boot(void);

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

int mips_p8700_hart_start(u32 hartid, ulong saddr)
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

int mips_p8700_hart_stop()
{
	u32 hartid = current_hartid();

	/* Hart 0 is the boot hart, and we don't use the CPC cmd to stop.  */
	if (hartid == 0)
		return SBI_ENOTSUPP;

	write_cpc_co_vp_stop(hartid, 1 << cpu_hart(hartid));

	return 0;
}

void mips_p8700_cache_info(struct p8700_cache_info *l1d, struct p8700_cache_info *l1i,
			   struct p8700_cache_info *l2)
{
	u32 mipsconfig1 = csr_read(CSR_MIPSCONFIG1);

	if (l1d) {
		u32 da = EXTRACT_FIELD(mipsconfig1, MIPSCONFIG1_DA);
		u32 dl = EXTRACT_FIELD(mipsconfig1, MIPSCONFIG1_DL);
		u32 ds = EXTRACT_FIELD(mipsconfig1, MIPSCONFIG1_DS);

		l1d->line = dl ? 1 << (dl + 1) : 0;
		l1d->assoc_ways = da +1;
		l1d->sets = ds == 7 ? 32 : 1 << (ds + 6);
	}
	if (l1i) {
		u32 ia = EXTRACT_FIELD(mipsconfig1, MIPSCONFIG1_IA);
		u32 il = EXTRACT_FIELD(mipsconfig1, MIPSCONFIG1_IL);
		u32 is = EXTRACT_FIELD(mipsconfig1, MIPSCONFIG1_IS);

		l1i->line = il ? 1 << (il + 1) : 0;
		l1i->assoc_ways = ia +1;
		l1i->sets = is == 7 ? 32 : 1 << (is + 6);
	}
	if (l2) {
		if (mipsconfig1 & MIPSCONFIG1_L2C) {
			void *cm_base = (void *)p8700_cm_info->gcr_base[0];
			u32 l2_config = readl(cm_base + GCR_L2_CONFIG);

			if (l2_config & GCR_L2_REG_EXISTS) {
				u32 l2a = EXTRACT_FIELD(l2_config, GCR_L2_ASSOC);
				u32 l2l = EXTRACT_FIELD(l2_config, GCR_L2_LINE_SIZE);
				u32 l2s = EXTRACT_FIELD(l2_config, GCR_L2_SET_SIZE);

				l2->assoc_ways = l2a + 1;
				l2->line = 1 << (l2l + 1);
				l2->sets = 1 << (l2s + 6);
				return;
			}
		}
		l2->line = 0;
		l2->assoc_ways = 0;
		l2->sets = 0;
	}
}

int mips_p8700_platform_init(const void *fdt, int nodeoff, const struct fdt_match *match)
{
	const struct p8700_cm_info *data = match->data;

	if (!data) {
		sbi_printf("Missing CM info for %s\n", match->compatible);
		return SBI_EINVAL;
	}

	p8700_cm_info = data;
	return SBI_OK;
}
