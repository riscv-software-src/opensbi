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
#include <sbi/sbi_hart.h>
#include <sbi/sbi_scratch.h>

#define DEFINE_UNPRIVILEGED_LOAD_FUNCTION(type, insn, insnlen)                \
	type load_##type(const type *addr,                                    \
			struct sbi_scratch *scratch,                          \
			struct unpriv_trap *trap)                             \
	{                                                                     \
		register ulong __mstatus asm("a2");                           \
		type val = 0;                                                 \
		trap->ilen = insnlen;                                         \
		trap->cause = 0;                                              \
		trap->tval = 0;                                               \
		sbi_hart_set_trap_info(scratch, trap);                        \
		asm volatile(                                                 \
			"csrrs %0, " STR(CSR_MSTATUS) ", %3\n"                \
			#insn " %1, %2\n"                                     \
			"csrw " STR(CSR_MSTATUS) ", %0"                       \
		    : "+&r"(__mstatus), "=&r"(val)                            \
		    : "m"(*addr), "r"(MSTATUS_MPRV));                         \
		sbi_hart_set_trap_info(scratch, NULL);                        \
		return val;                                                   \
	}

#define DEFINE_UNPRIVILEGED_STORE_FUNCTION(type, insn, insnlen)               \
	void store_##type(type *addr, type val,                               \
			struct sbi_scratch *scratch,                          \
			struct unpriv_trap *trap)                             \
	{                                                                     \
		register ulong __mstatus asm("a3");                           \
		trap->ilen = insnlen;                                         \
		trap->cause = 0;                                              \
		trap->tval = 0;                                               \
		sbi_hart_set_trap_info(scratch, trap);                        \
		asm volatile(                                                 \
			"csrrs %0, " STR(CSR_MSTATUS) ", %3\n"                \
			#insn " %1, %2\n"                                     \
			"csrw " STR(CSR_MSTATUS) ", %0"                       \
			: "+&r"(__mstatus)                                    \
			: "r"(val), "m"(*addr), "r"(MSTATUS_MPRV));           \
		sbi_hart_set_trap_info(scratch, NULL);                        \
	}

DEFINE_UNPRIVILEGED_LOAD_FUNCTION(u8, lbu, 4)
DEFINE_UNPRIVILEGED_LOAD_FUNCTION(u16, lhu, 4)
DEFINE_UNPRIVILEGED_LOAD_FUNCTION(s8, lb, 4)
DEFINE_UNPRIVILEGED_LOAD_FUNCTION(s16, lh, 4)
DEFINE_UNPRIVILEGED_LOAD_FUNCTION(s32, lw, 2)
DEFINE_UNPRIVILEGED_STORE_FUNCTION(u8, sb, 4)
DEFINE_UNPRIVILEGED_STORE_FUNCTION(u16, sh, 4)
DEFINE_UNPRIVILEGED_STORE_FUNCTION(u32, sw, 2)
#if __riscv_xlen == 64
DEFINE_UNPRIVILEGED_LOAD_FUNCTION(u32, lwu, 4)
DEFINE_UNPRIVILEGED_LOAD_FUNCTION(u64, ld, 2)
DEFINE_UNPRIVILEGED_STORE_FUNCTION(u64, sd, 2)
DEFINE_UNPRIVILEGED_LOAD_FUNCTION(ulong, ld, 2)
#else
DEFINE_UNPRIVILEGED_LOAD_FUNCTION(u32, lw, 2)
DEFINE_UNPRIVILEGED_LOAD_FUNCTION(ulong, lw, 2)

u64 load_u64(const u64 *addr,
	     struct sbi_scratch *scratch, struct unpriv_trap *trap)
{
	u64 ret = load_u32((u32 *)addr, scratch, trap);

	if (trap->cause)
		return 0;
	ret |= ((u64)load_u32((u32 *)addr + 1, scratch, trap) << 32);
	if (trap->cause)
		return 0;

	return ret;
}

void store_u64(u64 *addr, u64 val,
	       struct sbi_scratch *scratch, struct unpriv_trap *trap)
{
	store_u32((u32 *)addr, val, scratch, trap);
	if (trap->cause)
		return;

	store_u32((u32 *)addr + 1, val >> 32, scratch, trap);
	if (trap->cause)
		return;
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
