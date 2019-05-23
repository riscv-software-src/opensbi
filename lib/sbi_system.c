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

int sbi_system_early_init(struct sbi_scratch *scratch, bool cold_boot)
{
	return sbi_platform_early_init(sbi_platform_ptr(scratch), cold_boot);
}

int sbi_system_final_init(struct sbi_scratch *scratch, bool cold_boot)
{
	return sbi_platform_final_init(sbi_platform_ptr(scratch), cold_boot);
}

void __attribute__((noreturn))
sbi_system_reboot(struct sbi_scratch *scratch, u32 type)

{
	sbi_platform_system_reboot(sbi_platform_ptr(scratch), type);
	sbi_hart_hang();
}

void __attribute__((noreturn))
sbi_system_shutdown(struct sbi_scratch *scratch, u32 type)
{
	/* First try the platform-specific method */
	sbi_platform_system_shutdown(sbi_platform_ptr(scratch), type);

	/* If that fails (or is not implemented) send an IPI on every
	 * hart to hang and then hang the current hart */
	sbi_ipi_send_many(scratch, NULL, NULL, SBI_IPI_EVENT_HALT, NULL);

	sbi_hart_hang();
}
