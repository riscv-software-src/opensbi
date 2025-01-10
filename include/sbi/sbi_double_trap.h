/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 Rivos Inc.
 *
 * Authors:
 *   Clément Léger <cleger@rivosinc.com>
 */

#ifndef __SBI_DOUBLE_TRAP_H__
#define __SBI_DOUBLE_TRAP_H__

#include <sbi/sbi_types.h>
#include <sbi/sbi_trap.h>

int sbi_double_trap_handler(struct sbi_trap_context *tcntx);

void sbi_double_trap_init(struct sbi_scratch *scratch);

#endif
