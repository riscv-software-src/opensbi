/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 *   Nick Kossifidis <mick@ics.forth.gr>
 */

#include <sbi/riscv_asm.h>
#include <sbi/sbi_bitops.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_hsm.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_system.h>
#include <sbi/sbi_ipi.h>
#include <sbi/sbi_init.h>
#include <sbi/sbi_timer.h>

static SBI_LIST_HEAD(reset_devices_list);

const struct sbi_system_reset_device *sbi_system_reset_get_device(
					u32 reset_type, u32 reset_reason)
{
	struct sbi_system_reset_device *reset_dev = NULL;
	struct sbi_dlist *pos;
	/** lowest priority - any non zero is our candidate */
	int priority = 0;

	/* Check each reset device registered for supported reset type */
	sbi_list_for_each(pos, &(reset_devices_list)) {
		struct sbi_system_reset_device *dev =
			to_system_reset_device(pos);
		if (dev->system_reset_check) {
			int status = dev->system_reset_check(reset_type,
							     reset_reason);
			/** reset_type not supported */
			if (status == 0)
				continue;

			if (status > priority) {
				reset_dev = dev;
				priority = status;
			}
		}
	}

	return reset_dev;
}

void sbi_system_reset_add_device(struct sbi_system_reset_device *dev)
{
	if (!dev || !dev->system_reset_check)
		return;

	sbi_list_add(&(dev->node), &(reset_devices_list));
}

bool sbi_system_reset_supported(u32 reset_type, u32 reset_reason)
{
	return !!sbi_system_reset_get_device(reset_type, reset_reason);
}

void __noreturn sbi_system_reset(u32 reset_type, u32 reset_reason)
{
	ulong hbase = 0, hmask;
	u32 cur_hartid = current_hartid();
	struct sbi_domain *dom = sbi_domain_thishart_ptr();
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();

	/* Send HALT IPI to every hart other than the current hart */
	while (!sbi_hsm_hart_interruptible_mask(dom, hbase, &hmask)) {
		if (hbase <= cur_hartid)
			hmask &= ~(1UL << (cur_hartid - hbase));
		if (hmask)
			sbi_ipi_send_halt(hmask, hbase);
		hbase += BITS_PER_LONG;
	}

	/* Stop current HART */
	sbi_hsm_hart_stop(scratch, false);

	/* Platform specific reset if domain allowed system reset */
	if (dom->system_reset_allowed) {
		const struct sbi_system_reset_device *dev =
			sbi_system_reset_get_device(reset_type, reset_reason);
		if (dev)
			dev->system_reset(reset_type, reset_reason);
	}

	/* If platform specific reset did not work then do sbi_exit() */
	sbi_exit(scratch);
}

static const struct sbi_system_suspend_device *suspend_dev = NULL;

const struct sbi_system_suspend_device *sbi_system_suspend_get_device(void)
{
	return suspend_dev;
}

void sbi_system_suspend_set_device(struct sbi_system_suspend_device *dev)
{
	if (!dev || suspend_dev)
		return;

	suspend_dev = dev;
}

static int sbi_system_suspend_test_check(u32 sleep_type)
{
	return sleep_type == SBI_SUSP_SLEEP_TYPE_SUSPEND ? 0 : SBI_EINVAL;
}

static int sbi_system_suspend_test_suspend(u32 sleep_type,
					   unsigned long mmode_resume_addr)
{
	if (sleep_type != SBI_SUSP_SLEEP_TYPE_SUSPEND)
		return SBI_EINVAL;

	sbi_timer_mdelay(5000);

	/* Wait for interrupt */
	wfi();

	return SBI_OK;
}

static struct sbi_system_suspend_device sbi_system_suspend_test = {
	.name = "system-suspend-test",
	.system_suspend_check = sbi_system_suspend_test_check,
	.system_suspend = sbi_system_suspend_test_suspend,
};

void sbi_system_suspend_test_enable(void)
{
	sbi_system_suspend_set_device(&sbi_system_suspend_test);
}

bool sbi_system_suspend_supported(u32 sleep_type)
{
	return suspend_dev && suspend_dev->system_suspend_check &&
	       suspend_dev->system_suspend_check(sleep_type) == 0;
}

int sbi_system_suspend(u32 sleep_type, ulong resume_addr, ulong opaque)
{
	const struct sbi_domain *dom = sbi_domain_thishart_ptr();
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();
	void (*jump_warmboot)(void) = (void (*)(void))scratch->warmboot_addr;
	unsigned int hartid = current_hartid();
	unsigned long prev_mode;
	unsigned long i;
	int ret;

	if (!dom || !dom->system_suspend_allowed)
		return SBI_EFAIL;

	if (!suspend_dev || !suspend_dev->system_suspend ||
	    !suspend_dev->system_suspend_check)
		return SBI_EFAIL;

	ret = suspend_dev->system_suspend_check(sleep_type);
	if (ret != SBI_OK)
		return ret;

	prev_mode = (csr_read(CSR_MSTATUS) & MSTATUS_MPP) >> MSTATUS_MPP_SHIFT;
	if (prev_mode != PRV_S && prev_mode != PRV_U)
		return SBI_EFAIL;

	sbi_hartmask_for_each_hart(i, &dom->assigned_harts) {
		if (i == hartid)
			continue;
		if (__sbi_hsm_hart_get_state(i) != SBI_HSM_STATE_STOPPED)
			return SBI_EFAIL;
	}

	if (!sbi_domain_check_addr(dom, resume_addr, prev_mode,
				   SBI_DOMAIN_EXECUTE))
		return SBI_EINVALID_ADDR;

	if (!sbi_hsm_hart_change_state(scratch, SBI_HSM_STATE_STARTED,
				       SBI_HSM_STATE_SUSPENDED))
		return SBI_EFAIL;

	/* Prepare for resume */
	scratch->next_mode = prev_mode;
	scratch->next_addr = resume_addr;
	scratch->next_arg1 = opaque;

	__sbi_hsm_suspend_non_ret_save(scratch);

	/* Suspend */
	ret = suspend_dev->system_suspend(sleep_type, scratch->warmboot_addr);
	if (ret != SBI_OK) {
		if (!sbi_hsm_hart_change_state(scratch, SBI_HSM_STATE_SUSPENDED,
					       SBI_HSM_STATE_STARTED))
			sbi_hart_hang();
		return ret;
	}

	/* Resume */
	jump_warmboot();

	__builtin_unreachable();
}
