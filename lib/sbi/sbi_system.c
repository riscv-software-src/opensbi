/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 *   Nick Kossifidis <mick@ics.forth.gr>
 */

#include <sbi/sbi_hart.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_system.h>
#include <sbi/sbi_ipi.h>
#include <sbi/sbi_init.h>

int sbi_system_early_init(struct sbi_scratch *scratch, bool cold_boot)
{
	return sbi_platform_early_init(sbi_platform_ptr(scratch), cold_boot);
}

int sbi_system_final_init(struct sbi_scratch *scratch, bool cold_boot)
{
	return sbi_platform_final_init(sbi_platform_ptr(scratch), cold_boot);
}

void sbi_system_early_exit(struct sbi_scratch *scratch)
{
	sbi_platform_early_exit(sbi_platform_ptr(scratch));
}

void sbi_system_final_exit(struct sbi_scratch *scratch)
{
	sbi_platform_final_exit(sbi_platform_ptr(scratch));
}

void __noreturn sbi_system_reboot(struct sbi_scratch *scratch, u32 type)
{
	u32 current_hartid_mask = 1UL << sbi_current_hartid();

	/* Send HALT IPI to every hart other than the current hart */
	sbi_ipi_send_halt(scratch,
			  sbi_hart_available_mask() & ~current_hartid_mask, 0);

	/* Platform specific reooot */
	sbi_platform_system_reboot(sbi_platform_ptr(scratch), type);

	/* If platform specific reboot did not work then do sbi_exit() */
	sbi_exit(scratch);
}

void __noreturn sbi_system_shutdown(struct sbi_scratch *scratch, u32 type)
{
	u32 current_hartid_mask = 1UL << sbi_current_hartid();

	/* Send HALT IPI to every hart other than the current hart */
	sbi_ipi_send_halt(scratch,
			  sbi_hart_available_mask() & ~current_hartid_mask, 0);

	/* Platform specific shutdown */
	sbi_platform_system_shutdown(sbi_platform_ptr(scratch), type);

	/* If platform specific shutdown did not work then do sbi_exit() */
	sbi_exit(scratch);
}
