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
#include <sbi/sbi_hart.h>
#include <sbi/sbi_hsm.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_system.h>
#include <sbi/sbi_ipi.h>
#include <sbi/sbi_init.h>

void __noreturn sbi_system_reboot(u32 type)
{
	ulong hbase = 0, hmask;
	u32 cur_hartid = current_hartid();
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();

	/* Send HALT IPI to every hart other than the current hart */
	while (!sbi_hsm_hart_started_mask(hbase, &hmask)) {
		if (hbase <= cur_hartid)
			hmask &= ~(1UL << (cur_hartid - hbase));
		if (hmask)
			sbi_ipi_send_halt(hmask, hbase);
		hbase += BITS_PER_LONG;
	}

	/* Stop current HART */
	sbi_hsm_hart_stop(scratch, FALSE);

	/* Platform specific reooot */
	sbi_platform_system_reboot(sbi_platform_ptr(scratch), type);

	/* If platform specific reboot did not work then do sbi_exit() */
	sbi_exit(scratch);
}

void __noreturn sbi_system_shutdown(u32 type)
{
	ulong hbase = 0, hmask;
	u32 cur_hartid = current_hartid();
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();

	/* Send HALT IPI to every hart other than the current hart */
	while (!sbi_hsm_hart_started_mask(hbase, &hmask)) {
		if (hbase <= cur_hartid)
			hmask &= ~(1UL << (cur_hartid - hbase));
		if (hmask)
			sbi_ipi_send_halt(hmask, hbase);
		hbase += BITS_PER_LONG;
	}

	/* Stop current HART */
	sbi_hsm_hart_stop(scratch, FALSE);

	/* Platform specific shutdown */
	sbi_platform_system_shutdown(sbi_platform_ptr(scratch), type);

	/* If platform specific shutdown did not work then do sbi_exit() */
	sbi_exit(scratch);
}
