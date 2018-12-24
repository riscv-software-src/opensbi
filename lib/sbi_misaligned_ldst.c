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
#include <sbi/sbi_error.h>
#include <sbi/sbi_misaligned_ldst.h>
#include <sbi/sbi_trap.h>
#include <sbi/sbi_unpriv.h>

union reg_data {
	u8 data_bytes[8];
	ulong data_ulong;
	u64 data_u64;
};

int sbi_misaligned_load_handler(u32 hartid, ulong mcause,
				struct sbi_trap_regs *regs,
				struct sbi_scratch *scratch)
{
	union reg_data val;
	ulong mstatus = csr_read(mstatus);
	ulong insn = get_insn(regs->mepc, &mstatus);
	ulong addr = csr_read(mtval);
	int i, shift = 0, len = 0;

	if ((insn & INSN_MASK_LW) == INSN_MATCH_LW) {
		len = 4;
		shift = 8 * (sizeof(ulong) - len);
#if __riscv_xlen == 64
	} else if ((insn & INSN_MASK_LD) == INSN_MATCH_LD) {
		len = 8;
		shift = 8 * (sizeof(ulong) - len);
	} else if ((insn & INSN_MASK_LWU) == INSN_MATCH_LWU) {
		len = 4;
#endif
	} else if ((insn & INSN_MASK_LH) == INSN_MATCH_LH) {
		len = 2;
		shift = 8 * (sizeof(ulong) - len);
	} else if ((insn & INSN_MASK_LHU) == INSN_MATCH_LHU) {
		len = 2;
#ifdef __riscv_compressed
# if __riscv_xlen >= 64
	} else if ((insn & INSN_MASK_C_LD) == INSN_MATCH_C_LD) {
		len = 8;
		shift = 8 * (sizeof(ulong) - len);
		insn = RVC_RS2S(insn) << SH_RD;
	} else if ((insn & INSN_MASK_C_LDSP) == INSN_MATCH_C_LDSP &&
		   ((insn >> SH_RD) & 0x1f)) {
		len = 8;
		shift = 8 * (sizeof(ulong) - len);
# endif
	} else if ((insn & INSN_MASK_C_LW) ==INSN_MATCH_C_LW) {
		len = 4;
		shift = 8 * (sizeof(ulong) - len);
		insn = RVC_RS2S(insn) << SH_RD;
	} else if ((insn & INSN_MASK_C_LWSP) == INSN_MATCH_C_LWSP &&
		   ((insn >> SH_RD) & 0x1f)) {
		len = 4;
		shift = 8 * (sizeof(ulong) - len);
#endif
	} else
		return SBI_EILL;

	val.data_u64 = 0;
	for (i = 0; i < len; i++)
		val.data_bytes[i] = load_u8((void *)(addr + i), regs->mepc);

	SET_RD(insn, regs, val.data_ulong << shift >> shift);

	regs->mepc += INSN_LEN(insn);

	return 0;
}

int sbi_misaligned_store_handler(u32 hartid, ulong mcause,
				 struct sbi_trap_regs *regs,
				 struct sbi_scratch *scratch)
{
	union reg_data val;
	ulong mstatus = csr_read(mstatus);
	ulong insn = get_insn(regs->mepc, &mstatus);
	ulong addr = csr_read(mtval);
	int i, len = 0;

	val.data_ulong = GET_RS2(insn, regs);

	if ((insn & INSN_MASK_SW) == INSN_MATCH_SW) {
		len = 4;
#if __riscv_xlen == 64
	} else if ((insn & INSN_MASK_SD) == INSN_MATCH_SD) {
		len = 8;
#endif
	} else if ((insn & INSN_MASK_SH) == INSN_MATCH_SH) {
		len = 2;
#ifdef __riscv_compressed
# if __riscv_xlen >= 64
	} else if ((insn & INSN_MASK_C_SD) == INSN_MATCH_C_SD) {
		len = 8;
		val.data_ulong = GET_RS2S(insn, regs);
	} else if ((insn & INSN_MASK_C_SDSP) == INSN_MATCH_C_SDSP &&
		   ((insn >> SH_RD) & 0x1f)) {
		len = 8;
		val.data_ulong = GET_RS2C(insn, regs);
# endif
	} else if ((insn & INSN_MASK_C_SW) == INSN_MATCH_C_SW) {
		len = 4;
		val.data_ulong = GET_RS2S(insn, regs);
	} else if ((insn & INSN_MASK_C_SWSP) == INSN_MATCH_C_SWSP &&
		   ((insn >> SH_RD) & 0x1f)) {
		len = 4;
		val.data_ulong = GET_RS2C(insn, regs);
#endif
	} else
		return SBI_EILL;

	for (i = 0; i < len; i++)
		store_u8((void *)(addr + i), val.data_bytes[i], regs->mepc);

	regs->mepc += INSN_LEN(insn);

	return 0;
}
