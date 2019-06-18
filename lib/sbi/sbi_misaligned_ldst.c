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
#include <sbi/riscv_fp.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_misaligned_ldst.h>
#include <sbi/sbi_trap.h>

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
	struct unpriv_trap uptrap;
	ulong insn = get_insn(regs->mepc, NULL);
	ulong addr = csr_read(CSR_MTVAL);
	int i, fp = 0, shift = 0, len = 0;

	if ((insn & INSN_MASK_LW) == INSN_MATCH_LW) {
		len   = 4;
		shift = 8 * (sizeof(ulong) - len);
#if __riscv_xlen == 64
	} else if ((insn & INSN_MASK_LD) == INSN_MATCH_LD) {
		len   = 8;
		shift = 8 * (sizeof(ulong) - len);
	} else if ((insn & INSN_MASK_LWU) == INSN_MATCH_LWU) {
		len = 4;
#endif
	} else if ((insn & INSN_MASK_FLD) == INSN_MATCH_FLD) {
		fp  = 1;
		len = 8;
	} else if ((insn & INSN_MASK_FLW) == INSN_MATCH_FLW) {
		fp  = 1;
		len = 4;
	} else if ((insn & INSN_MASK_LH) == INSN_MATCH_LH) {
		len   = 2;
		shift = 8 * (sizeof(ulong) - len);
	} else if ((insn & INSN_MASK_LHU) == INSN_MATCH_LHU) {
		len = 2;
#ifdef __riscv_compressed
#if __riscv_xlen >= 64
	} else if ((insn & INSN_MASK_C_LD) == INSN_MATCH_C_LD) {
		len   = 8;
		shift = 8 * (sizeof(ulong) - len);
		insn  = RVC_RS2S(insn) << SH_RD;
	} else if ((insn & INSN_MASK_C_LDSP) == INSN_MATCH_C_LDSP &&
		   ((insn >> SH_RD) & 0x1f)) {
		len   = 8;
		shift = 8 * (sizeof(ulong) - len);
#endif
	} else if ((insn & INSN_MASK_C_LW) == INSN_MATCH_C_LW) {
		len   = 4;
		shift = 8 * (sizeof(ulong) - len);
		insn  = RVC_RS2S(insn) << SH_RD;
	} else if ((insn & INSN_MASK_C_LWSP) == INSN_MATCH_C_LWSP &&
		   ((insn >> SH_RD) & 0x1f)) {
		len   = 4;
		shift = 8 * (sizeof(ulong) - len);
	} else if ((insn & INSN_MASK_C_FLD) == INSN_MATCH_C_FLD) {
		fp   = 1;
		len  = 8;
		insn = RVC_RS2S(insn) << SH_RD;
	} else if ((insn & INSN_MASK_C_FLDSP) == INSN_MATCH_C_FLDSP) {
		fp  = 1;
		len = 8;
#if __riscv_xlen == 32
	} else if ((insn & INSN_MASK_C_FLW) == INSN_MATCH_C_FLW) {
		fp   = 1;
		len  = 4;
		insn = RVC_RS2S(insn) << SH_RD;
	} else if ((insn & INSN_MASK_C_FLWSP) == INSN_MATCH_C_FLWSP) {
		fp  = 1;
		len = 4;
#endif
#endif
	} else
		return SBI_EILL;

	val.data_u64 = 0;
	for (i = 0; i < len; i++) {
		val.data_bytes[i] = load_u8((void *)(addr + i),
					    scratch, &uptrap);
		if (uptrap.cause) {
			sbi_trap_redirect(regs, scratch, regs->mepc,
					  uptrap.cause, uptrap.tval);
			return 0;
		}
	}

	if (!fp)
		SET_RD(insn, regs, val.data_ulong << shift >> shift);
	else if (len == 8)
		SET_F64_RD(insn, regs, val.data_u64);
	else
		SET_F32_RD(insn, regs, val.data_ulong);

	regs->mepc += INSN_LEN(insn);

	return 0;
}

int sbi_misaligned_store_handler(u32 hartid, ulong mcause,
				 struct sbi_trap_regs *regs,
				 struct sbi_scratch *scratch)
{
	union reg_data val;
	struct unpriv_trap uptrap;
	ulong insn = get_insn(regs->mepc, NULL);
	ulong addr = csr_read(CSR_MTVAL);
	int i, len = 0;

	val.data_ulong = GET_RS2(insn, regs);

	if ((insn & INSN_MASK_SW) == INSN_MATCH_SW) {
		len = 4;
#if __riscv_xlen == 64
	} else if ((insn & INSN_MASK_SD) == INSN_MATCH_SD) {
		len = 8;
#endif
	} else if ((insn & INSN_MASK_FSD) == INSN_MATCH_FSD) {
		len	     = 8;
		val.data_u64 = GET_F64_RS2(insn, regs);
	} else if ((insn & INSN_MASK_FSW) == INSN_MATCH_FSW) {
		len	       = 4;
		val.data_ulong = GET_F32_RS2(insn, regs);
	} else if ((insn & INSN_MASK_SH) == INSN_MATCH_SH) {
		len = 2;
#ifdef __riscv_compressed
#if __riscv_xlen >= 64
	} else if ((insn & INSN_MASK_C_SD) == INSN_MATCH_C_SD) {
		len	       = 8;
		val.data_ulong = GET_RS2S(insn, regs);
	} else if ((insn & INSN_MASK_C_SDSP) == INSN_MATCH_C_SDSP &&
		   ((insn >> SH_RD) & 0x1f)) {
		len	       = 8;
		val.data_ulong = GET_RS2C(insn, regs);
#endif
	} else if ((insn & INSN_MASK_C_SW) == INSN_MATCH_C_SW) {
		len	       = 4;
		val.data_ulong = GET_RS2S(insn, regs);
	} else if ((insn & INSN_MASK_C_SWSP) == INSN_MATCH_C_SWSP &&
		   ((insn >> SH_RD) & 0x1f)) {
		len	       = 4;
		val.data_ulong = GET_RS2C(insn, regs);
	} else if ((insn & INSN_MASK_C_FSD) == INSN_MATCH_C_FSD) {
		len	     = 8;
		val.data_u64 = GET_F64_RS2S(insn, regs);
	} else if ((insn & INSN_MASK_C_FSDSP) == INSN_MATCH_C_FSDSP) {
		len	     = 8;
		val.data_u64 = GET_F64_RS2C(insn, regs);
#if __riscv_xlen == 32
	} else if ((insn & INSN_MASK_C_FSW) == INSN_MATCH_C_FSW) {
		len	       = 4;
		val.data_ulong = GET_F32_RS2S(insn, regs);
	} else if ((insn & INSN_MASK_C_FSWSP) == INSN_MATCH_C_FSWSP) {
		len	       = 4;
		val.data_ulong = GET_F32_RS2C(insn, regs);
#endif
#endif
	} else
		return SBI_EILL;

	for (i = 0; i < len; i++) {
		store_u8((void *)(addr + i), val.data_bytes[i],
			 scratch, &uptrap);
		if (uptrap.cause) {
			sbi_trap_redirect(regs, scratch, regs->mepc,
					  uptrap.cause, uptrap.tval);
			return 0;
		}
	}

	regs->mepc += INSN_LEN(insn);

	return 0;
}
