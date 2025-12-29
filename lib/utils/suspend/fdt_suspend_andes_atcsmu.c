/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 Andes Technology Corporation
 */

#include <andes/andes.h>
#include <sbi/riscv_asm.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_system.h>
#include <sbi_utils/cache/fdt_cmo_helper.h>
#include <sbi_utils/fdt/fdt_driver.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/hsm/fdt_hsm_andes_atcsmu.h>

static int check_secondary_harts_sleep(u32 hartid, bool deep_sleep)
{
	const struct sbi_domain *dom = &root;
	unsigned long i;
	u32 target;

	/* Ensure the secondary harts entering the corresponding sleep state */
	sbi_hartmask_for_each_hartindex(i, dom->possible_harts) {
		target = sbi_hartindex_to_hartid(i);
		if (target != hartid && !atcsmu_pcs_is_sleep(target, deep_sleep))
			return SBI_EFAIL;
	}

	return SBI_OK;
}

static int ae350_system_suspend_check(u32 sleep_type)
{
	return (sleep_type == SBI_SUSP_SLEEP_TYPE_SUSPEND ||
		sleep_type == SBI_SUSP_AE350_LIGHT_SLEEP) ? SBI_OK : SBI_EINVAL;
}

static int ae350_system_suspend(u32 sleep_type, unsigned long addr)
{
	u32 hartid = current_hartid();
	int rc;

	/* Prevent the core leaving the WFI mode unexpectedly */
	csr_write(CSR_MIE, 0);

	/*
	 * Only allow the S-mode external interrupts (UART2 and RTC alarm) to
	 * wake up the primary hart
	 */
	csr_set(CSR_SIE, MIP_SEIP);
	atcsmu_set_wakeup_events(PCS_WAKEUP_RTC_ALARM_MASK | PCS_WAKEUP_UART2_MASK, hartid);

	if (sleep_type == SBI_SUSP_AE350_LIGHT_SLEEP) {
		rc = check_secondary_harts_sleep(hartid, false);
		if (rc)
			return rc;

		atcsmu_set_command(LIGHT_SLEEP_CMD, hartid);
	} else if (sleep_type == SBI_SUSP_SLEEP_TYPE_SUSPEND) {
		rc = check_secondary_harts_sleep(hartid, true);
		if (rc)
			return rc;

		atcsmu_set_command(DEEP_SLEEP_CMD, hartid);
		rc = atcsmu_set_reset_vector((ulong)ae350_enable_coherency_warmboot, hartid);
		if (rc)
			return rc;

		ae350_non_ret_save(sbi_scratch_thishart_ptr());
		fdt_cmo_llc_enable(false);
		rc = fdt_cmo_llc_flush_all();
		if (rc)
			return rc;
	}

	ae350_disable_coherency();
	wfi();

	/* Light sleep resumes here */
	ae350_enable_coherency();

	return SBI_OK;
}

static void ae350_system_resume(void)
{
	u32 hartid = current_hartid();
	u32 sleep_type = atcsmu_get_sleep_type(hartid);

	if (sleep_type == SBI_SUSP_SLEEP_TYPE_SUSPEND)
		fdt_cmo_llc_enable(true);
}

static struct sbi_system_suspend_device suspend_andes_atcsmu = {
	.name = "andes_atcsmu",
	.system_suspend_check = ae350_system_suspend_check,
	.system_suspend = ae350_system_suspend,
	.system_resume = ae350_system_resume,
};

static int suspend_andes_atcsmu_probe(const void *fdt, int nodeoff, const struct fdt_match *match)
{
	sbi_system_suspend_set_device(&suspend_andes_atcsmu);
	return 0;
}

static const struct fdt_match suspend_andes_atcsmu_match[] = {
	{ .compatible = "andestech,atcsmu-sys" },
	{ },
};

const struct fdt_driver fdt_suspend_andes_atcsmu = {
	.match_table = suspend_andes_atcsmu_match,
	.init = suspend_andes_atcsmu_probe,
};
