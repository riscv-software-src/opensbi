/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#ifndef __SBI_ILLEGAl_INSN_H__
#define __SBI_ILLEGAl_INSN_H__

#include <sbi/sbi_types.h>

struct sbi_trap_context;

typedef int (*illegal_insn_func)(ulong insn, struct sbi_trap_regs *regs);

int truly_illegal_insn(ulong insn, struct sbi_trap_regs *regs);

int sbi_illegal_insn_handler(struct sbi_trap_context *tcntx);

#endif
