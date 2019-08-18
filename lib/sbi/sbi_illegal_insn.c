/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_encoding.h>
#include <sbi/riscv_unpriv.h>
#include <sbi/sbi_bits.h>
#include <sbi/sbi_emulate_csr.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_illegal_insn.h>
#include <sbi/sbi_trap.h>

typedef int (*illegal_insn_func)(ulong insn, u32 hartid, ulong mcause,
				 struct sbi_trap_regs *regs,
				 struct sbi_scratch *scratch);

static int truly_illegal_insn(ulong insn, u32 hartid, ulong mcause,
			      struct sbi_trap_regs *regs,
			      struct sbi_scratch *scratch)
{
	return sbi_trap_redirect(regs, scratch, regs->mepc, mcause, insn);
}

static int system_opcode_insn(ulong insn, u32 hartid, ulong mcause,
			      struct sbi_trap_regs *regs,
			      struct sbi_scratch *scratch)
{
	int do_write, rs1_num = (insn >> 15) & 0x1f;
	ulong rs1_val = GET_RS1(insn, regs);
	int csr_num   = (u32)insn >> 20;
	ulong csr_val, new_csr_val;

	/*
	 * WFI always traps as illegal instruction when executed from
	 * VS/VU mode so we just forward it to HS-mode.
	 */
#if __riscv_xlen == 32
	if ((regs->mstatusH & MSTATUSH_MPV) &&
#else
	if ((regs->mstatus & MSTATUS_MPV) &&
#endif
	    (insn & INSN_MASK_WFI) == INSN_MATCH_WFI)
		return sbi_trap_redirect(regs, scratch,
					 regs->mepc, mcause, insn);

	if (sbi_emulate_csr_read(csr_num, hartid, regs, scratch, &csr_val))
		return truly_illegal_insn(insn, hartid, mcause,
					  regs, scratch);

	do_write = rs1_num;
	switch (GET_RM(insn)) {
	case 1:
		new_csr_val = rs1_val;
		do_write    = 1;
		break;
	case 2:
		new_csr_val = csr_val | rs1_val;
		break;
	case 3:
		new_csr_val = csr_val & ~rs1_val;
		break;
	case 5:
		new_csr_val = rs1_num;
		do_write    = 1;
		break;
	case 6:
		new_csr_val = csr_val | rs1_num;
		break;
	case 7:
		new_csr_val = csr_val & ~rs1_num;
		break;
	default:
		return truly_illegal_insn(insn, hartid, mcause, regs, scratch);
	};

	if (do_write && sbi_emulate_csr_write(csr_num, hartid, regs,
					      scratch, new_csr_val))
		return truly_illegal_insn(insn, hartid, mcause, regs, scratch);

	SET_RD(insn, regs, csr_val);

	regs->mepc += 4;

	return 0;
}

static illegal_insn_func illegal_insn_table[32] = {
	truly_illegal_insn, /* 0 */
	truly_illegal_insn, /* 1 */
	truly_illegal_insn, /* 2 */
	truly_illegal_insn, /* 3 */
	truly_illegal_insn, /* 4 */
	truly_illegal_insn, /* 5 */
	truly_illegal_insn, /* 6 */
	truly_illegal_insn, /* 7 */
	truly_illegal_insn, /* 8 */
	truly_illegal_insn, /* 9 */
	truly_illegal_insn, /* 10 */
	truly_illegal_insn, /* 11 */
	truly_illegal_insn, /* 12 */
	truly_illegal_insn, /* 13 */
	truly_illegal_insn, /* 14 */
	truly_illegal_insn, /* 15 */
	truly_illegal_insn, /* 16 */
	truly_illegal_insn, /* 17 */
	truly_illegal_insn, /* 18 */
	truly_illegal_insn, /* 19 */
	truly_illegal_insn, /* 20 */
	truly_illegal_insn, /* 21 */
	truly_illegal_insn, /* 22 */
	truly_illegal_insn, /* 23 */
	truly_illegal_insn, /* 24 */
	truly_illegal_insn, /* 25 */
	truly_illegal_insn, /* 26 */
	truly_illegal_insn, /* 27 */
	system_opcode_insn, /* 28 */
	truly_illegal_insn, /* 29 */
	truly_illegal_insn, /* 30 */
	truly_illegal_insn  /* 31 */
};

int sbi_illegal_insn_handler(u32 hartid, ulong mcause,
			     struct sbi_trap_regs *regs,
			     struct sbi_scratch *scratch)
{
	ulong insn = csr_read(CSR_MTVAL);
#if __riscv_xlen == 32
	bool virt = (regs->mstatusH & MSTATUSH_MPV) ? TRUE : FALSE;
#else
	bool virt = (regs->mstatus & MSTATUS_MPV) ? TRUE : FALSE;
#endif
	struct unpriv_trap uptrap;

	if (unlikely((insn & 3) != 3)) {
		if (insn == 0) {
			insn = get_insn(regs->mepc, virt, scratch, &uptrap);
			if (uptrap.cause)
				return sbi_trap_redirect(regs, scratch,
					regs->mepc, uptrap.cause, uptrap.tval);
		}
		if ((insn & 3) != 3)
			return truly_illegal_insn(insn, hartid, mcause, regs,
						  scratch);
	}

	return illegal_insn_table[(insn & 0x7c) >> 2](insn, hartid, mcause,
						      regs, scratch);
}
