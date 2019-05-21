/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <sbi/riscv_encoding.h>
#include <sbi/riscv_unpriv.h>
#include <sbi/sbi_bits.h>

#define DEFINE_UNPRIVILEGED_LOAD_FUNCTION(type, insn)                         \
	type load_##type(const type *addr)                                    \
	{                                                                     \
		register ulong __mstatus asm("a2");                           \
		type val;                                                     \
		asm volatile(                                                 \
			"csrrs %0, " STR(CSR_MSTATUS) ", %3\n"                \
			#insn " %1, %2\n"                                     \
			"csrw " STR(CSR_MSTATUS) ", %0"                       \
		    : "+&r"(__mstatus), "=&r"(val)                            \
		    : "m"(*addr), "r"(MSTATUS_MPRV));                         \
		return val;                                                   \
	}

#define DEFINE_UNPRIVILEGED_STORE_FUNCTION(type, insn)                        \
	void store_##type(type *addr, type val)                               \
	{                                                                     \
		register ulong __mstatus asm("a3");                           \
		asm volatile(                                                 \
			"csrrs %0, " STR(CSR_MSTATUS) ", %3\n"                \
			#insn " %1, %2\n"                                     \
			"csrw " STR(CSR_MSTATUS) ", %0"                       \
			: "+&r"(__mstatus)                                    \
			: "r"(val), "m"(*addr), "r"(MSTATUS_MPRV));           \
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

u64 load_u64(const u64 *addr)
{
	return load_u32((u32 *)addr) + ((u64)load_u32((u32 *)addr + 1) << 32);
}

void store_u64(u64 *addr, u64 val)
{
	store_u32((u32 *)addr, val);
	store_u32((u32 *)addr + 1, val >> 32);
}
#endif

ulong get_insn(ulong mepc, ulong *mstatus)
{
	register ulong __mepc asm("a2") = mepc;
	register ulong __mstatus asm("a3");
	ulong val;
#ifndef __riscv_compressed
	asm("csrrs %[mstatus], " STR(CSR_MSTATUS) ", %[mprv]\n"
#if __riscv_xlen == 64
	    STR(LWU) " %[insn], (%[addr])\n"
#else
	    STR(LW) " %[insn], (%[addr])\n"
#endif
		     "csrw " STR(CSR_MSTATUS) ", %[mstatus]"
	    : [mstatus] "+&r"(__mstatus), [insn] "=&r"(val)
	    : [mprv] "r"(MSTATUS_MPRV | MSTATUS_MXR), [addr] "r"(__mepc));
#else
	ulong rvc_mask = 3, tmp;
	asm("csrrs %[mstatus], " STR(CSR_MSTATUS) ", %[mprv]\n"
						  "and %[tmp], %[addr], 2\n"
						  "bnez %[tmp], 1f\n"
#if __riscv_xlen == 64
	    STR(LWU) " %[insn], (%[addr])\n"
#else
	    STR(LW) " %[insn], (%[addr])\n"
#endif
		     "and %[tmp], %[insn], %[rvc_mask]\n"
		     "beq %[tmp], %[rvc_mask], 2f\n"
		     "sll %[insn], %[insn], %[xlen_minus_16]\n"
		     "srl %[insn], %[insn], %[xlen_minus_16]\n"
		     "j 2f\n"
		     "1:\n"
		     "lhu %[insn], (%[addr])\n"
		     "and %[tmp], %[insn], %[rvc_mask]\n"
		     "bne %[tmp], %[rvc_mask], 2f\n"
		     "lhu %[tmp], 2(%[addr])\n"
		     "sll %[tmp], %[tmp], 16\n"
		     "add %[insn], %[insn], %[tmp]\n"
		     "2: csrw " STR(CSR_MSTATUS) ", %[mstatus]"
	    : [mstatus] "+&r"(__mstatus), [insn] "=&r"(val), [tmp] "=&r"(tmp)
	    : [mprv] "r"(MSTATUS_MPRV | MSTATUS_MXR), [addr] "r"(__mepc),
	      [rvc_mask] "r"(rvc_mask), [xlen_minus_16] "i"(__riscv_xlen - 16));
#endif
	if (mstatus)
		*mstatus = __mstatus;
	return val;
}
