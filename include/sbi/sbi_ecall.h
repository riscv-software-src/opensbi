/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#ifndef __SBI_ECALL_H__
#define __SBI_ECALL_H__

#include <sbi/sbi_types.h>

struct sbi_trap_regs;
struct sbi_scratch;

u16 sbi_ecall_version_major(void);

u16 sbi_ecall_version_minor(void);

int sbi_ecall_handler(u32 hartid, ulong mcause, struct sbi_trap_regs *regs,
		      struct sbi_scratch *scratch);

#endif
