/*
 * Copyright (c) 2018 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_encoding.h>
#include <sbi/sbi_bits.h>
#include <sbi/sbi_emulate_csr.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_illegal_insn.h>
#include <sbi/sbi_trap.h>
#include <sbi/sbi_unpriv.h>

#define SH_RD			7
#define SH_RS1			15
#define SH_RS2			20
#define SH_RS2C			2

#define RV_X(x, s, n)		(((x) >> (s)) & ((1 << (n)) - 1))
#define RVC_LW_IMM(x)		((RV_X(x, 6, 1) << 2) | \
				 (RV_X(x, 10, 3) << 3) | \
				 (RV_X(x, 5, 1) << 6))
#define RVC_LD_IMM(x)		((RV_X(x, 10, 3) << 3) | \
				 (RV_X(x, 5, 2) << 6))
#define RVC_LWSP_IMM(x)		((RV_X(x, 4, 3) << 2) | \
				 (RV_X(x, 12, 1) << 5) | \
				 (RV_X(x, 2, 2) << 6))
#define RVC_LDSP_IMM(x)		((RV_X(x, 5, 2) << 3) | \
				 (RV_X(x, 12, 1) << 5) | \
				 (RV_X(x, 2, 3) << 6))
#define RVC_SWSP_IMM(x)		((RV_X(x, 9, 4) << 2) | \
				 (RV_X(x, 7, 2) << 6))
#define RVC_SDSP_IMM(x)		((RV_X(x, 10, 3) << 3) | \
				 (RV_X(x, 7, 3) << 6))
#define RVC_RS1S(insn)		(8 + RV_X(insn, SH_RD, 3))
#define RVC_RS2S(insn)		(8 + RV_X(insn, SH_RS2C, 3))
#define RVC_RS2(insn)		RV_X(insn, SH_RS2C, 5)

#define SHIFT_RIGHT(x, y)	((y) < 0 ? ((x) << -(y)) : ((x) >> (y)))

#define REG_MASK		\
((1 << (5 + LOG_REGBYTES)) - (1 << LOG_REGBYTES))

#define REG_OFFSET(insn, pos)	\
(SHIFT_RIGHT((insn), (pos) - LOG_REGBYTES) & REG_MASK)

#define REG_PTR(insn, pos, regs)\
(ulong *)((ulong)(regs) + REG_OFFSET(insn, pos))

#define GET_RM(insn)		(((insn) >> 12) & 7)

#define GET_RS1(insn, regs)	(*REG_PTR(insn, SH_RS1, regs))
#define GET_RS2(insn, regs)	(*REG_PTR(insn, SH_RS2, regs))
#define GET_RS1S(insn, regs)	(*REG_PTR(RVC_RS1S(insn), 0, regs))
#define GET_RS2S(insn, regs)	(*REG_PTR(RVC_RS2S(insn), 0, regs))
#define GET_RS2C(insn, regs)	(*REG_PTR(insn, SH_RS2C, regs))
#define GET_SP(regs)		(*REG_PTR(2, 0, regs))
#define SET_RD(insn, regs, val)	(*REG_PTR(insn, SH_RD, regs) = (val))
#define IMM_I(insn)		((s32)(insn) >> 20)
#define IMM_S(insn)		(((s32)(insn) >> 25 << 5) | \
				 (s32)(((insn) >> 7) & 0x1f))
#define MASK_FUNCT3		0x7000

typedef int (*illegal_insn_func)(ulong insn,
				 u32 hartid, ulong mcause,
				 struct sbi_trap_regs *regs,
				 struct sbi_scratch *scratch);

static int truly_illegal_insn(ulong insn,
			      u32 hartid, ulong mcause,
			      struct sbi_trap_regs *regs,
			      struct sbi_scratch *scratch)
{
	/* For now, always fails */
	return SBI_ENOTSUPP;
}

static int system_opcode_insn(ulong insn,
			      u32 hartid, ulong mcause,
			      struct sbi_trap_regs *regs,
			      struct sbi_scratch *scratch)
{
	int do_write, rs1_num = (insn >> 15) & 0x1f;
	ulong rs1_val = GET_RS1(insn, regs);
	int csr_num = (u32)insn >> 20;
	ulong csr_val, new_csr_val;

	if (sbi_emulate_csr_read(csr_num, hartid, regs->mstatus,
				 scratch, &csr_val))
		return truly_illegal_insn(insn, hartid, mcause,
					  regs, scratch);

	do_write = rs1_num;
	switch (GET_RM(insn)) {
	case 1:
		new_csr_val = rs1_val;
		do_write = 1;
		break;
	case 2:
		new_csr_val = csr_val | rs1_val;
		break;
	case 3: new_csr_val = csr_val & ~rs1_val;
		break;
	case 5:
		new_csr_val = rs1_num;
		do_write = 1;
		break;
	case 6:
		new_csr_val = csr_val | rs1_num;
		break;
	case 7:
		new_csr_val = csr_val & ~rs1_num;
		break;
	default:
		return truly_illegal_insn(insn, hartid, mcause,
					  regs, scratch);
	};

	if (do_write &&
	    sbi_emulate_csr_write(csr_num, hartid, regs->mstatus,
				  scratch, new_csr_val))
		return truly_illegal_insn(insn, hartid, mcause,
					  regs, scratch);

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
	ulong mstatus;
	ulong insn = csr_read(mbadaddr);

	if (unlikely((insn & 3) != 3)) {
		if (insn == 0) {
			mstatus = csr_read(mstatus);
			insn = get_insn(regs->mepc, &mstatus);
		}
		if ((insn & 3) != 3)
			return SBI_ENOTSUPP;
	}

	return illegal_insn_table[(insn & 0x7c) >> 2](insn, hartid, mcause,
						      regs, scratch);
}
