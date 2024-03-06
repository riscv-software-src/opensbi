/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#ifndef __SBI_TRAP_LDST_H__
#define __SBI_TRAP_LDST_H__

#include <sbi/sbi_types.h>
#include <sbi/sbi_trap.h>

struct sbi_trap_regs;

int sbi_misaligned_load_handler(struct sbi_trap_regs *regs,
				const struct sbi_trap_info *orig_trap);

int sbi_misaligned_store_handler(struct sbi_trap_regs *regs,
				 const struct sbi_trap_info *orig_trap);

#endif
