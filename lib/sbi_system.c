/*
 * Copyright (c) 2018 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sbi/sbi_hart.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_system.h>

int sbi_system_warm_early_init(struct sbi_scratch *scratch, u32 hartid)
{
	return sbi_platform_warm_early_init(sbi_platform_ptr(scratch), hartid);
}

int sbi_system_warm_final_init(struct sbi_scratch *scratch, u32 hartid)
{
	return sbi_platform_warm_final_init(sbi_platform_ptr(scratch), hartid);
}

int sbi_system_cold_early_init(struct sbi_scratch *scratch)
{
	return sbi_platform_cold_early_init(sbi_platform_ptr(scratch));
}

int sbi_system_cold_final_init(struct sbi_scratch *scratch)
{
	return sbi_platform_cold_final_init(sbi_platform_ptr(scratch));
}

void __attribute__((noreturn)) sbi_system_reboot(struct sbi_scratch *scratch,
						 u32 type)

{
	sbi_platform_system_reboot(sbi_platform_ptr(scratch), type);
	sbi_hart_hang();
}

void __attribute__((noreturn)) sbi_system_shutdown(struct sbi_scratch *scratch,
						   u32 type)
{
	sbi_platform_system_shutdown(sbi_platform_ptr(scratch), type);
	sbi_hart_hang();
}
