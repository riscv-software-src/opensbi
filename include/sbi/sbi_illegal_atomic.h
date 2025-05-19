/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 MIPS
 *
 */

#ifndef __SBI_ILLEGAL_ATOMIC_H__
#define __SBI_ILLEGAL_ATOMIC_H__

#include <sbi/sbi_types.h>

struct sbi_trap_regs;

int sbi_illegal_atomic(ulong insn, struct sbi_trap_regs *regs);

#endif
