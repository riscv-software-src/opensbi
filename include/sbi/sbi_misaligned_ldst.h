/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#ifndef __SBI_MISALIGNED_LDST_H__
#define __SBI_MISALIGNED_LDST_H__

#include <sbi/sbi_types.h>

struct sbi_trap_regs;
struct sbi_scratch;

int sbi_misaligned_load_handler(u32 hartid, ulong mcause,
				ulong addr, ulong tval2, ulong tinst,
				struct sbi_trap_regs *regs,
				struct sbi_scratch *scratch);

int sbi_misaligned_store_handler(u32 hartid, ulong mcause,
				 ulong addr, ulong tval2, ulong tinst,
				 struct sbi_trap_regs *regs,
				 struct sbi_scratch *scratch);

#endif
