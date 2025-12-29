/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 Andes Technology Corporation
 */

#include <andes/andes.h>
#include <libfdt.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_hsm.h>
#include <sbi/sbi_init.h>
#include <sbi/sbi_ipi.h>
#include <sbi_utils/fdt/fdt_driver.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/hsm/fdt_hsm_andes_atcsmu.h>

static unsigned long atcsmu_base;

void atcsmu_set_wakeup_events(u32 events, u32 hartid)
{
	writel_relaxed(events, (char *)atcsmu_base + PCSm_WE_OFFSET(hartid));
}

bool atcsmu_support_sleep_mode(u32 sleep_type, u32 hartid)
{
	u32 pcs_cfg;
	u32 mask;
	const char *sleep_mode;

	pcs_cfg = readl_relaxed((char *)atcsmu_base + PCSm_CFG_OFFSET(hartid));
	switch (sleep_type) {
	case SBI_SUSP_AE350_LIGHT_SLEEP:
		mask = PCS_CFG_LIGHT_SLEEP;
		sleep_mode = "light sleep";
		break;
	case SBI_SUSP_SLEEP_TYPE_SUSPEND:
		mask = PCS_CFG_DEEP_SLEEP;
		sleep_mode = "deep sleep";
		break;
	default:
		return false;
	}

	if (!EXTRACT_FIELD(pcs_cfg, mask)) {
		sbi_printf("ATCSMU: hart%d (PCS%d) does not support %s mode\n",
			   hartid, hartid + 3, sleep_mode);
		return false;
	}

	return true;
}

void atcsmu_set_command(u32 pcs_ctl, u32 hartid)
{
	writel_relaxed(pcs_ctl, (char *)atcsmu_base + PCSm_CTL_OFFSET(hartid));
}

int atcsmu_set_reset_vector(u64 wakeup_addr, u32 hartid)
{
	u32 vec_lo;
	u32 vec_hi;
	u64 reset_vector;

	writel((u32)wakeup_addr, (char *)atcsmu_base + HARTn_RESET_VEC_LO(hartid));
	writel((u32)(wakeup_addr >> 32), (char *)atcsmu_base + HARTn_RESET_VEC_HI(hartid));
	vec_lo = readl((char *)atcsmu_base + HARTn_RESET_VEC_LO(hartid));
	vec_hi = readl((char *)atcsmu_base + HARTn_RESET_VEC_HI(hartid));
	reset_vector = (u64)vec_hi << 32 | vec_lo;
	if (reset_vector != wakeup_addr) {
		sbi_printf("ATCSMU: hart%d (PCS%d): failed to program the reset vector\n",
			   hartid, hartid + 3);
		return SBI_EFAIL;
	}

	return SBI_OK;
}

u32 atcsmu_get_sleep_type(u32 hartid)
{
	return readl_relaxed((char *)atcsmu_base + PCSm_SCRATCH_OFFSET(hartid));
}

static int ae350_hart_start(u32 hartid, ulong saddr)
{
	u32 hartindex = sbi_hartid_to_hartindex(hartid);

	/*
	 * Don't send wakeup command when:
	 * 1) boot time
	 * 2) the target hart is non-sleepable 25-series hart0
	 */
	if (!sbi_init_count(hartindex) || (is_andes(25) && hartid == 0))
		return sbi_ipi_raw_send(hartindex, false);

	atcsmu_set_command(WAKEUP_CMD, hartid);
	return 0;
}

static int ae350_hart_stop(void)
{
	u32 hartid = current_hartid();
	u32 sleep_type = atcsmu_get_sleep_type(hartid);
	int rc;

	/*
	 * For Andes AX25MP, the hart0 shares power domain with the last level
	 * cache. Instead of turning it off, it should fall through and jump to
	 * warmboot_addr.
	 */
	if (is_andes(25) && hartid == 0)
		return SBI_ENOTSUPP;

	if (!atcsmu_support_sleep_mode(sleep_type, hartid))
		return SBI_ENOTSUPP;

	/* Prevent the core leaving the WFI mode unexpectedly */
	csr_write(CSR_MIE, 0);

	atcsmu_set_wakeup_events(0x0, hartid);
	atcsmu_set_command(DEEP_SLEEP_CMD, hartid);
	rc = atcsmu_set_reset_vector((ulong)ae350_enable_coherency_warmboot, hartid);
	if (rc)
		return SBI_EFAIL;

	ae350_non_ret_save(sbi_scratch_thishart_ptr());
	ae350_disable_coherency();
	wfi();
	return 0;
}

static const struct sbi_hsm_device hsm_andes_atcsmu = {
	.name = "andes_atcsmu",
	.hart_start = ae350_hart_start,
	.hart_stop = ae350_hart_stop,
};

static int hsm_andes_atcsmu_probe(const void *fdt, int nodeoff, const struct fdt_match *match)
{
	int poff, rc;
	u64 addr;

	/* Need to find the parent for the address property  */
	poff = fdt_parent_offset(fdt, nodeoff);
	if (poff < 0)
		return SBI_EINVAL;

	rc = fdt_get_node_addr_size(fdt, poff, 0, &addr, NULL);
	if (rc < 0 || !addr)
		return SBI_ENODEV;
	atcsmu_base = addr;

	sbi_hsm_set_device(&hsm_andes_atcsmu);
	return 0;
}

static const struct fdt_match hsm_andes_atcsmu_match[] = {
	{ .compatible = "andestech,atcsmu-hsm" },
	{ },
};

const struct fdt_driver fdt_hsm_andes_atcsmu = {
	.match_table = hsm_andes_atcsmu_match,
	.init = hsm_andes_atcsmu_probe,
};
