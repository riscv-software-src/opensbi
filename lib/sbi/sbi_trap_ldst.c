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
#include <sbi/riscv_fp.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_trap_ldst.h>
#include <sbi/sbi_trap.h>
#include <sbi/sbi_unpriv.h>
#include <sbi/sbi_platform.h>

/**
 * Load emulator callback:
 *
 * @return rlen=success, 0=success w/o regs modification, or negative error
 */
typedef int (*sbi_trap_ld_emulator)(int rlen, union sbi_ldst_data *out_val,
				    struct sbi_trap_context *tcntx);

/**
 * Store emulator callback:
 *
 * @return wlen=success, 0=success w/o regs modification, or negative error
 */
typedef int (*sbi_trap_st_emulator)(int wlen, union sbi_ldst_data in_val,
				    struct sbi_trap_context *tcntx);

ulong sbi_misaligned_tinst_fixup(ulong orig_tinst, ulong new_tinst,
					ulong addr_offset)
{
	if (new_tinst == INSN_PSEUDO_VS_LOAD ||
	    new_tinst == INSN_PSEUDO_VS_STORE)
		return new_tinst;
	else if (orig_tinst == 0)
		return 0UL;
	else
		return orig_tinst | (addr_offset << SH_RS1);
}

static int sbi_trap_emulate_load(struct sbi_trap_context *tcntx,
				 sbi_trap_ld_emulator emu)
{
	const struct sbi_trap_info *orig_trap = &tcntx->trap;
	struct sbi_trap_regs *regs = &tcntx->regs;
	ulong insn, insn_len;
	union sbi_ldst_data val = { 0 };
	struct sbi_trap_info uptrap;
	int rc, fp = 0, shift = 0, len = 0, vector = 0;

	if (orig_trap->tinst & 0x1) {
		/*
		 * Bit[0] == 1 implies trapped instruction value is
		 * transformed instruction or custom instruction.
		 */
		insn	 = orig_trap->tinst | INSN_16BIT_MASK;
		insn_len = (orig_trap->tinst & 0x2) ? INSN_LEN(insn) : 2;
	} else {
		/*
		 * Bit[0] == 0 implies trapped instruction value is
		 * zero or special value.
		 */
		insn = sbi_get_insn(regs->mepc, &uptrap);
		if (uptrap.cause) {
			return sbi_trap_redirect(regs, &uptrap);
		}
		insn_len = INSN_LEN(insn);
	}

	if ((insn & INSN_MASK_LB) == INSN_MATCH_LB) {
		len   = 1;
		shift = 8 * (sizeof(ulong) - len);
	} else if ((insn & INSN_MASK_LBU) == INSN_MATCH_LBU) {
		len = 1;
	} else if ((insn & INSN_MASK_LW) == INSN_MATCH_LW) {
		len   = 4;
		shift = 8 * (sizeof(ulong) - len);
#if __riscv_xlen == 64
	} else if ((insn & INSN_MASK_LD) == INSN_MATCH_LD) {
		len   = 8;
		shift = 8 * (sizeof(ulong) - len);
	} else if ((insn & INSN_MASK_LWU) == INSN_MATCH_LWU) {
		len = 4;
#endif
#ifdef __riscv_flen
	} else if ((insn & INSN_MASK_FLD) == INSN_MATCH_FLD) {
		fp  = 1;
		len = 8;
	} else if ((insn & INSN_MASK_FLW) == INSN_MATCH_FLW) {
		fp  = 1;
		len = 4;
#endif
	} else if ((insn & INSN_MASK_LH) == INSN_MATCH_LH) {
		len   = 2;
		shift = 8 * (sizeof(ulong) - len);
	} else if ((insn & INSN_MASK_LHU) == INSN_MATCH_LHU) {
		len = 2;
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
#ifdef __riscv_flen
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
	} else if ((insn & INSN_MASK_C_LHU) == INSN_MATCH_C_LHU) {
		len = 2;
		insn = RVC_RS2S(insn) << SH_RD;
	} else if ((insn & INSN_MASK_C_LH) == INSN_MATCH_C_LH) {
		len = 2;
		shift = 8 * (sizeof(ulong) - len);
		insn = RVC_RS2S(insn) << SH_RD;
	} else if (IS_VECTOR_LOAD_STORE(insn)) {
		vector = 1;
		emu = sbi_misaligned_v_ld_emulator;
	} else {
		return sbi_trap_redirect(regs, orig_trap);
	}

	rc = emu(len, &val, tcntx);
	if (rc <= 0)
		return rc;

	if (!vector) {
		if (!fp)
			SET_RD(insn, regs, ((long)(val.data_ulong << shift)) >> shift);
#ifdef __riscv_flen
		else if (len == 8)
			SET_F64_RD(insn, regs, val.data_u64);
		else
			SET_F32_RD(insn, regs, val.data_ulong);
#endif
	}

	regs->mepc += insn_len;

	return 0;
}

static int sbi_trap_emulate_store(struct sbi_trap_context *tcntx,
				  sbi_trap_st_emulator emu)
{
	const struct sbi_trap_info *orig_trap = &tcntx->trap;
	struct sbi_trap_regs *regs = &tcntx->regs;
	ulong insn, insn_len;
	union sbi_ldst_data val;
	struct sbi_trap_info uptrap;
	int rc, len = 0;

	if (orig_trap->tinst & 0x1) {
		/*
		 * Bit[0] == 1 implies trapped instruction value is
		 * transformed instruction or custom instruction.
		 */
		insn	 = orig_trap->tinst | INSN_16BIT_MASK;
		insn_len = (orig_trap->tinst & 0x2) ? INSN_LEN(insn) : 2;
	} else {
		/*
		 * Bit[0] == 0 implies trapped instruction value is
		 * zero or special value.
		 */
		insn = sbi_get_insn(regs->mepc, &uptrap);
		if (uptrap.cause) {
			return sbi_trap_redirect(regs, &uptrap);
		}
		insn_len = INSN_LEN(insn);
	}

	val.data_ulong = GET_RS2(insn, regs);

	if ((insn & INSN_MASK_SB) == INSN_MATCH_SB) {
		len = 1;
	} else if ((insn & INSN_MASK_SW) == INSN_MATCH_SW) {
		len = 4;
#if __riscv_xlen == 64
	} else if ((insn & INSN_MASK_SD) == INSN_MATCH_SD) {
		len = 8;
#endif
#ifdef __riscv_flen
	} else if ((insn & INSN_MASK_FSD) == INSN_MATCH_FSD) {
		len	     = 8;
		val.data_u64 = GET_F64_RS2(insn, regs);
	} else if ((insn & INSN_MASK_FSW) == INSN_MATCH_FSW) {
		len	       = 4;
		val.data_ulong = GET_F32_RS2(insn, regs);
#endif
	} else if ((insn & INSN_MASK_SH) == INSN_MATCH_SH) {
		len = 2;
#if __riscv_xlen >= 64
	} else if ((insn & INSN_MASK_C_SD) == INSN_MATCH_C_SD) {
		len	       = 8;
		val.data_ulong = GET_RS2S(insn, regs);
	} else if ((insn & INSN_MASK_C_SDSP) == INSN_MATCH_C_SDSP) {
		len	       = 8;
		val.data_ulong = GET_RS2C(insn, regs);
#endif
	} else if ((insn & INSN_MASK_C_SW) == INSN_MATCH_C_SW) {
		len	       = 4;
		val.data_ulong = GET_RS2S(insn, regs);
	} else if ((insn & INSN_MASK_C_SWSP) == INSN_MATCH_C_SWSP) {
		len	       = 4;
		val.data_ulong = GET_RS2C(insn, regs);
#ifdef __riscv_flen
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
	} else if ((insn & INSN_MASK_C_SH) == INSN_MATCH_C_SH) {
		len		= 2;
		val.data_ulong = GET_RS2S(insn, regs);
	} else if (IS_VECTOR_LOAD_STORE(insn)) {
		emu = sbi_misaligned_v_st_emulator;
	} else {
		return sbi_trap_redirect(regs, orig_trap);
	}

	rc = emu(len, val, tcntx);
	if (rc <= 0)
		return rc;

	regs->mepc += insn_len;

	return 0;
}

static int sbi_misaligned_ld_emulator(int rlen, union sbi_ldst_data *out_val,
				      struct sbi_trap_context *tcntx)
{
	const struct sbi_trap_info *orig_trap = &tcntx->trap;
	struct sbi_trap_regs *regs = &tcntx->regs;
	struct sbi_trap_info uptrap;
	int i;

	for (i = 0; i < rlen; i++) {
		out_val->data_bytes[i] =
			sbi_load_u8((void *)(orig_trap->tval + i), &uptrap);
		if (uptrap.cause) {
			uptrap.tinst = sbi_misaligned_tinst_fixup(
				orig_trap->tinst, uptrap.tinst, i);
			return sbi_trap_redirect(regs, &uptrap);
		}
	}
	return rlen;
}

int sbi_misaligned_load_handler(struct sbi_trap_context *tcntx)
{
	return sbi_trap_emulate_load(tcntx, sbi_misaligned_ld_emulator);
}

static int sbi_misaligned_st_emulator(int wlen, union sbi_ldst_data in_val,
				      struct sbi_trap_context *tcntx)
{
	const struct sbi_trap_info *orig_trap = &tcntx->trap;
	struct sbi_trap_regs *regs = &tcntx->regs;
	struct sbi_trap_info uptrap;
	int i;

	for (i = 0; i < wlen; i++) {
		sbi_store_u8((void *)(orig_trap->tval + i),
			     in_val.data_bytes[i], &uptrap);
		if (uptrap.cause) {
			uptrap.tinst = sbi_misaligned_tinst_fixup(
				orig_trap->tinst, uptrap.tinst, i);
			return sbi_trap_redirect(regs, &uptrap);
		}
	}
	return wlen;
}

int sbi_misaligned_store_handler(struct sbi_trap_context *tcntx)
{
	return sbi_trap_emulate_store(tcntx, sbi_misaligned_st_emulator);
}

static int sbi_ld_access_emulator(int rlen, union sbi_ldst_data *out_val,
				  struct sbi_trap_context *tcntx)
{
	const struct sbi_trap_info *orig_trap = &tcntx->trap;
	struct sbi_trap_regs *regs = &tcntx->regs;

	/* If fault came from M mode, just fail */
	if (sbi_mstatus_prev_mode(regs->mstatus) == PRV_M)
		return SBI_EINVAL;

	/* If platform emulator failed, we redirect instead of fail */
	if (sbi_platform_emulate_load(sbi_platform_thishart_ptr(), rlen,
				      orig_trap->tval, out_val))
		return sbi_trap_redirect(regs, orig_trap);

	return rlen;
}

int sbi_load_access_handler(struct sbi_trap_context *tcntx)
{
	return sbi_trap_emulate_load(tcntx, sbi_ld_access_emulator);
}

static int sbi_st_access_emulator(int wlen, union sbi_ldst_data in_val,
				  struct sbi_trap_context *tcntx)
{
	const struct sbi_trap_info *orig_trap = &tcntx->trap;
	struct sbi_trap_regs *regs = &tcntx->regs;

	/* If fault came from M mode, just fail */
	if (sbi_mstatus_prev_mode(regs->mstatus) == PRV_M)
		return SBI_EINVAL;

	/* If platform emulator failed, we redirect instead of fail */
	if (sbi_platform_emulate_store(sbi_platform_thishart_ptr(), wlen,
				       orig_trap->tval, in_val))
		return sbi_trap_redirect(regs, orig_trap);

	return wlen;
}

int sbi_store_access_handler(struct sbi_trap_context *tcntx)
{
	return sbi_trap_emulate_store(tcntx, sbi_st_access_emulator);
}
