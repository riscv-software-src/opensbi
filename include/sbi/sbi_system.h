/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#ifndef __SBI_SYSTEM_H__
#define __SBI_SYSTEM_H__

#include <sbi/sbi_types.h>

struct sbi_scratch;

int sbi_system_early_init(struct sbi_scratch *scratch, bool cold_boot);

int sbi_system_final_init(struct sbi_scratch *scratch, bool cold_boot);

void sbi_system_early_exit(struct sbi_scratch *scratch);

void sbi_system_final_exit(struct sbi_scratch *scratch);

void __noreturn sbi_system_reboot(struct sbi_scratch *scratch, u32 type);

void __noreturn sbi_system_shutdown(struct sbi_scratch *scratch, u32 type);

#endif
