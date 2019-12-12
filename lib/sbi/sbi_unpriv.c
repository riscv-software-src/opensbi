/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <sbi/riscv_encoding.h>
#include <sbi/sbi_bits.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_trap.h>
#include <sbi/sbi_unpriv.h>

#define DEFINE_UNPRIVILEGED_LOAD_FUNCTION(type, insn)                         \
	type sbi_load_##type(const type *addr,                                \
			     struct sbi_scratch *scratch,                     \
			     struct sbi_trap_info *trap)                      \
	{                                                                     \
		register ulong __mstatus asm("a2");                           \
		type val = 0;                                                 \
		trap->epc = 0;                                                \
		trap->cause = 0;                                              \
		trap->tval = 0;                                               \
		trap->tval2 = 0;                                              \
		trap->tinst = 0;                                              \
		sbi_hart_set_trap_info(scratch, trap);                        \
		asm volatile(                                                 \
			"csrrs %0, " STR(CSR_MSTATUS) ", %3\n"                \
			".option push\n"                                      \
			".option norvc\n"                                     \
			#insn " %1, %2\n"                                     \
			".option pop\n"                                       \
			"csrw " STR(CSR_MSTATUS) ", %0"                       \
		    : "+&r"(__mstatus), "=&r"(val)                            \
		    : "m"(*addr), "r"(MSTATUS_MPRV));                         \
		sbi_hart_set_trap_info(scratch, NULL);                        \
		return val;                                                   \
	}

#define DEFINE_UNPRIVILEGED_STORE_FUNCTION(type, insn)                        \
	void sbi_store_##type(type *addr, type val,                           \
			      struct sbi_scratch *scratch,                    \
			      struct sbi_trap_info *trap)                     \
	{                                                                     \
		register ulong __mstatus asm("a3");                           \
		trap->epc = 0;                                                \
		trap->cause = 0;                                              \
		trap->tval = 0;                                               \
		trap->tval2 = 0;                                              \
		trap->tinst = 0;                                              \
		sbi_hart_set_trap_info(scratch, trap);                        \
		asm volatile(                                                 \
			"csrrs %0, " STR(CSR_MSTATUS) ", %3\n"                \
			".option push\n"                                      \
			".option norvc\n"                                     \
			#insn " %1, %2\n"                                     \
			".option pop\n"                                       \
			"csrw " STR(CSR_MSTATUS) ", %0"                       \
			: "+&r"(__mstatus)                                    \
			: "r"(val), "m"(*addr), "r"(MSTATUS_MPRV));           \
		sbi_hart_set_trap_info(scratch, NULL);                        \
	}

DEFINE_UNPRIVILEGED_LOAD_FUNCTION(u8, lbu)
DEFINE_UNPRIVILEGED_LOAD_FUNCTION(u16, lhu)
DEFINE_UNPRIVILEGED_LOAD_FUNCTION(s8, lb)
DEFINE_UNPRIVILEGED_LOAD_FUNCTION(s16, lh)
DEFINE_UNPRIVILEGED_LOAD_FUNCTION(s32, lw)
DEFINE_UNPRIVILEGED_STORE_FUNCTION(u8, sb)
DEFINE_UNPRIVILEGED_STORE_FUNCTION(u16, sh)
DEFINE_UNPRIVILEGED_STORE_FUNCTION(u32, sw)
#if __riscv_xlen == 64
DEFINE_UNPRIVILEGED_LOAD_FUNCTION(u32, lwu)
DEFINE_UNPRIVILEGED_LOAD_FUNCTION(u64, ld)
DEFINE_UNPRIVILEGED_STORE_FUNCTION(u64, sd)
DEFINE_UNPRIVILEGED_LOAD_FUNCTION(ulong, ld)
#else
DEFINE_UNPRIVILEGED_LOAD_FUNCTION(u32, lw)
DEFINE_UNPRIVILEGED_LOAD_FUNCTION(ulong, lw)

u64 sbi_load_u64(const u64 *addr,
		 struct sbi_scratch *scratch,
		 struct sbi_trap_info *trap)
{
	u64 ret = sbi_load_u32((u32 *)addr, scratch, trap);

	if (trap->cause)
		return 0;
	ret |= ((u64)sbi_load_u32((u32 *)addr + 1, scratch, trap) << 32);
	if (trap->cause)
		return 0;

	return ret;
}

void sbi_store_u64(u64 *addr, u64 val,
		   struct sbi_scratch *scratch,
		   struct sbi_trap_info *trap)
{
	sbi_store_u32((u32 *)addr, val, scratch, trap);
	if (trap->cause)
		return;

	sbi_store_u32((u32 *)addr + 1, val >> 32, scratch, trap);
	if (trap->cause)
		return;
}
#endif

ulong sbi_get_insn(ulong mepc, struct sbi_scratch *scratch,
		   struct sbi_trap_info *trap)
{
	ulong __mstatus = 0, val = 0;
#ifdef __riscv_compressed
	ulong rvc_mask = 3, tmp;
#endif

	trap->epc = 0;
	trap->cause = 0;
	trap->tval = 0;
	trap->tval2 = 0;
	trap->tinst = 0;
	sbi_hart_set_trap_info(scratch, trap);

#ifndef __riscv_compressed
	asm("csrrs %[mstatus], " STR(CSR_MSTATUS) ", %[mprv]\n"
	    ".option push\n"
	    ".option norvc\n"
#if __riscv_xlen == 64
	    STR(LWU) " %[insn], (%[addr])\n"
#else
	    STR(LW) " %[insn], (%[addr])\n"
#endif
	    ".option pop\n"
	    "csrw " STR(CSR_MSTATUS) ", %[mstatus]"
	    : [mstatus] "+&r"(__mstatus), [insn] "=&r"(val)
	    : [mprv] "r"(MSTATUS_MPRV | MSTATUS_MXR), [addr] "r"(mepc));
#else
	asm("csrrs %[mstatus], " STR(CSR_MSTATUS) ", %[mprv]\n"
	    ".option push\n"
	    ".option norvc\n"
	    "lhu %[insn], (%[addr])\n"
	    ".option pop\n"
	    "and %[tmp], %[insn], %[rvc_mask]\n"
	    "bne %[tmp], %[rvc_mask], 2f\n"
	    ".option push\n"
	    ".option norvc\n"
	    "lhu %[tmp], 2(%[addr])\n"
	    ".option pop\n"
	    "sll %[tmp], %[tmp], 16\n"
	    "add %[insn], %[insn], %[tmp]\n"
	    "2: csrw " STR(CSR_MSTATUS) ", %[mstatus]"
	    : [mstatus] "+&r"(__mstatus), [insn] "=&r"(val), [tmp] "=&r"(tmp)
	    : [mprv] "r"(MSTATUS_MPRV | MSTATUS_MXR), [addr] "r"(mepc),
	      [rvc_mask] "r"(rvc_mask));
#endif

	sbi_hart_set_trap_info(scratch, NULL);

	switch (trap->cause) {
	case CAUSE_LOAD_ACCESS:
		trap->cause = CAUSE_FETCH_ACCESS;
		trap->tval = mepc;
		break;
	case CAUSE_LOAD_PAGE_FAULT:
		trap->cause = CAUSE_FETCH_PAGE_FAULT;
		trap->tval = mepc;
		break;
	case CAUSE_LOAD_GUEST_PAGE_FAULT:
		trap->cause = CAUSE_FETCH_GUEST_PAGE_FAULT;
		trap->tval = mepc;
		break;
	default:
		break;
	};

	return val;
}
