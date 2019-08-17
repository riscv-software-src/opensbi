/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#ifndef __RISCV_UNPRIV_H__
#define __RISCV_UNPRIV_H__

#include <sbi/sbi_types.h>

struct sbi_scratch;

struct unpriv_trap {
	unsigned long ilen;
	unsigned long cause;
	unsigned long tval;
};

#define DECLARE_UNPRIVILEGED_LOAD_FUNCTION(type)       \
	type load_##type(const type *addr,             \
			 struct sbi_scratch *scratch,  \
			 struct unpriv_trap *trap);

#define DECLARE_UNPRIVILEGED_STORE_FUNCTION(type)      \
	void store_##type(type *addr, type val,        \
			  struct sbi_scratch *scratch, \
			  struct unpriv_trap *trap);

DECLARE_UNPRIVILEGED_LOAD_FUNCTION(u8)
DECLARE_UNPRIVILEGED_LOAD_FUNCTION(u16)
DECLARE_UNPRIVILEGED_LOAD_FUNCTION(s8)
DECLARE_UNPRIVILEGED_LOAD_FUNCTION(s16)
DECLARE_UNPRIVILEGED_LOAD_FUNCTION(s32)
DECLARE_UNPRIVILEGED_STORE_FUNCTION(u8)
DECLARE_UNPRIVILEGED_STORE_FUNCTION(u16)
DECLARE_UNPRIVILEGED_STORE_FUNCTION(u32)
DECLARE_UNPRIVILEGED_LOAD_FUNCTION(u32)
DECLARE_UNPRIVILEGED_LOAD_FUNCTION(u64)
DECLARE_UNPRIVILEGED_STORE_FUNCTION(u64)
DECLARE_UNPRIVILEGED_LOAD_FUNCTION(ulong)

ulong get_insn(ulong mepc, bool virt, struct sbi_scratch *scratch,
	       struct unpriv_trap *trap);

#endif
