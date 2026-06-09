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
 *   Refer to comments of `sbi_platform_emulate_load`.
 */
typedef int (*sbi_trap_ld_emulator)(ulong insn, int rlen, ulong raddr,
				    union sbi_ldst_data *out_val,
				    struct sbi_trap_context *tcntx);

/**
 * Store emulator callback:
 *   Refer to comments of `sbi_platform_emulate_store`.
 */
typedef int (*sbi_trap_st_emulator)(ulong insn, int wlen, ulong waddr,
				    union sbi_ldst_data in_val,
				    struct sbi_trap_context *tcntx);

/**
 * Handling of misaligned fault is done by a collection of smaller, but
 * aligned load/store(s). Another fault (load/store, page fault...) can
 * arise from any of them, then the handling gets aborted. We must fixup
 * the tinst to pretend the fault was rised from the original insn.
 * Specifically, fixup the offset field using the tval diff between the
 * new trap and the original one (if required).
 */
static inline void sbi_misaligned_tinst_fixup(
			const struct sbi_trap_info *orig_trap,
			      struct sbi_trap_info *uptrap)
{
	ulong offset = uptrap->tval - orig_trap->tval;

	/*
	 * The function is called in code path for handling a scalar
	 * load/store misaligned fault, thus the new uptrap can't have
	 * custom value of tinst
	 */
	if (uptrap->tinst == INSN_PSEUDO_VS_LOAD ||
	    uptrap->tinst == INSN_PSEUDO_VS_STORE)
		/* Use uptrap as-is for guest-page faults */
		return;
	/*
	 * Only fixup if orig tinst is valid. Otherwise, discard the
	 * new tinst to be on the safe side. Never use new tinst as-is!
	 * It's load/store width surely mismatches the original width.
	 * For vector, discard it regardless. It doesn't make sense to
	 * have a transformed tinst
	 */
	else if (orig_trap->tinst == 0)
		uptrap->tinst = 0;
	else
		uptrap->tinst = orig_trap->tinst | (offset << SH_RS1);
}

static inline bool sbi_trap_tinst_valid(ulong tinst)
{
	/*
	 * Bit[0] == 1 implies trapped instruction value is
	 * transformed instruction or custom instruction.
	 * Also do proper checking per Privileged ISA 19.6.3,
	 * and make sure high 32 bits of tinst is 0
	 */
	return tinst == (uint32_t)tinst && (tinst & 0x1);
}

static int sbi_trap_emulate_load(struct sbi_trap_context *tcntx,
				 sbi_trap_ld_emulator emu)
{
	const struct sbi_trap_info *orig_trap = &tcntx->trap;
	struct sbi_trap_regs *regs = &tcntx->regs;
	ulong insn, insn_len, imm = 0, shift = 0, off = 0;
	union sbi_ldst_data val = { 0 };
	struct sbi_trap_info uptrap;
	bool xform = false, fp = false, c_load = false, c_ldsp = false;
	int rc, len = 0, prev_xlen = 0;

	if (sbi_trap_tinst_valid(orig_trap->tinst)) {
		xform	 = true;
		insn	 = orig_trap->tinst | INSN_16BIT_MASK;
		insn_len = (orig_trap->tinst & 0x2) ? INSN_LEN(insn) : 2;
	} else {
		/* trapped instruction value is zero or special value */
		insn = sbi_get_insn(regs->mepc, &uptrap);
		if (uptrap.cause) {
			return sbi_trap_redirect(regs, &uptrap);
		}
		insn_len = INSN_LEN(insn);
	}

	/**
	 * Common for RV32/RV64:
	 *    lb, lbu, lh, lhu, lw, flw, flw
	 *    c.lbu, c.lh, c.lhu, c.lw, c.lwsp, c.fld, c.fldsp
	 */
	if ((insn & INSN_MASK_LB) == INSN_MATCH_LB) {
		len = -1;
	} else if ((insn & INSN_MASK_LBU) == INSN_MATCH_LBU) {
		len = 1;
	} else if ((insn & INSN_MASK_C_LBU) == INSN_MATCH_C_LBU) {
		/* Zcb */
		len = 1;
		imm = RVC_LB_IMM(insn);
		c_load = true;
	} else if ((insn & INSN_MASK_LH) == INSN_MATCH_LH) {
		len = -2;
	} else if ((insn & INSN_MASK_C_LH) == INSN_MATCH_C_LH) {
		/* Zcb */
		len = -2;
		imm = RVC_LH_IMM(insn);
		c_load = true;
	} else if ((insn & INSN_MASK_LHU) == INSN_MATCH_LHU) {
		len = 2;
	} else if ((insn & INSN_MASK_C_LHU) == INSN_MATCH_C_LHU) {
		/* Zcb */
		len = 2;
		imm = RVC_LH_IMM(insn);
		c_load = true;
	} else if ((insn & INSN_MASK_LW) == INSN_MATCH_LW) {
		len = -4;
	} else if ((insn & INSN_MASK_C_LW) == INSN_MATCH_C_LW) {
		/* Zca */
		len = -4;
		imm = RVC_LW_IMM(insn);
		c_load = true;
	} else if ((insn & INSN_MASK_C_LWSP) == INSN_MATCH_C_LWSP &&
		GET_RD_NUM(insn)) {
		/* Zca */
		len = -4;
		imm = RVC_LWSP_IMM(insn);
		c_ldsp = true;
#ifdef __riscv_flen
	} else if ((insn & INSN_MASK_FLW) == INSN_MATCH_FLW) {
		len = 4;
		fp = true;
	} else if ((insn & INSN_MASK_FLD) == INSN_MATCH_FLD) {
		len = 8;
		fp = true;
	} else if ((insn & INSN_MASK_C_FLD) == INSN_MATCH_C_FLD) {
		/* Zcd */
		len = 8;
		imm = RVC_LD_IMM(insn);
		c_load = true;
		fp = true;
	} else if ((insn & INSN_MASK_C_FLDSP) == INSN_MATCH_C_FLDSP) {
		/* Zcd */
		len = 8;
		imm = RVC_LDSP_IMM(insn);
		c_ldsp = true;
		fp = true;
#endif
	} else {
		prev_xlen = sbi_regs_prev_xlen(regs);
	}

	/**
	 * Must distinguish between rv64 and rv32, RVC instructions have
	 * overlapping encoding:
	 *     c.ld in rv64 == c.flw in rv32
	 *     c.ldsp in rv64 == c.flwsp in rv32
	 */
	if (prev_xlen == 64) {
		/* RV64 Only: lwu, ld, c.ld, c.ldsp  */
		if ((insn & INSN_MASK_LWU) == INSN_MATCH_LWU) {
			len = 4;
		} else if ((insn & INSN_MASK_LD) == INSN_MATCH_LD) {
			len = 8;
		} else if ((insn & INSN_MASK_C_LD) == INSN_MATCH_C_LD) {
			/* Zca */
			len = 8;
			imm = RVC_LD_IMM(insn);
			c_load = true;
		} else if ((insn & INSN_MASK_C_LDSP) == INSN_MATCH_C_LDSP &&
			GET_RD_NUM(insn)) {
			/* Zca */
			len = 8;
			imm = RVC_LDSP_IMM(insn);
			c_ldsp = true;
		}
#ifdef __riscv_flen
	} else if (prev_xlen == 32) {
		/* RV32 Only: c.flw, c.flwsp */
		if ((insn & INSN_MASK_C_FLW) == INSN_MATCH_C_FLW) {
			/* Zcf */
			len = 4;
			imm = RVC_LW_IMM(insn);
			c_load = true;
			fp = true;
		} else if ((insn & INSN_MASK_C_FLWSP) == INSN_MATCH_C_FLWSP) {
			/* Zcf */
			len = 4;
			imm = RVC_LWSP_IMM(insn);
			c_ldsp = true;
			fp = true;
		}
#endif
	}

	if (len < 0) {
		len = -len;
		shift = 8 * (sizeof(ulong) - len);
	}

	if (!len) // Unknown instruction
		goto do_emu;

#if !defined(OPENSBI_DEBUG)
	/**
	 * For misaligned faults. Skip offset calculation unless DEBUG
	 * builds. It helps validating OpenSBI and HW.
	 */
	if (orig_trap->cause == CAUSE_MISALIGNED_LOAD)
		goto do_emu;
#endif

	if (xform)
		/* Transformed insn */
		off = GET_RS1_NUM(insn);
	else if (c_load)
		/* non SP-based compressed load */
		off = orig_trap->tval - GET_RS1S(insn, regs) - imm;
	else if (c_ldsp)
		/* SP-based compressed load */
		off = orig_trap->tval - REG_VAL(2, regs) - imm;
	else
		/* I-type non-compressed load */
		off = orig_trap->tval - GET_RS1(insn, regs) - (ulong)IMM_I(insn);
	/**
	 * Normalize offset, in case the XLEN of unpriv mode is smaller,
	 * and/or pointer masking is in effect
	 */
	off &= (len - 1);

do_emu:
	rc = emu(insn, len, orig_trap->tval - off, &val, tcntx);
	if (rc <= 0)
		return rc;
	if (!len)
		goto epc_fixup;

	if (!fp) {
		ulong v = ((long)(val.data_ulong << shift)) >> shift;

		if (c_load)
			SET_RDS(insn, regs, v);
		else
			SET_RD(insn, regs, v);
#ifdef __riscv_flen
	} else if (len == 8) {
		if (c_load)
			SET_F64_RDS(insn, regs, val.data_u64);
		else
			SET_F64_RD(insn, regs, val.data_u64);
	} else {
		if (c_load)
			SET_F32_RDS(insn, regs, val.data_ulong);
		else
			SET_F32_RD(insn, regs, val.data_ulong);
#endif
	}

epc_fixup:
	regs->mepc += insn_len;

	return 0;
}

static int sbi_trap_emulate_store(struct sbi_trap_context *tcntx,
				  sbi_trap_st_emulator emu)
{
	const struct sbi_trap_info *orig_trap = &tcntx->trap;
	struct sbi_trap_regs *regs = &tcntx->regs;
	ulong insn, insn_len, imm = 0, off = 0;
	union sbi_ldst_data val;
	struct sbi_trap_info uptrap;
	bool xform = false, fp = false, c_store = false, c_stsp = false;
	int rc, len = 0, prev_xlen = 0;

	if (sbi_trap_tinst_valid(orig_trap->tinst)) {
		xform	 = true;
		insn	 = orig_trap->tinst | INSN_16BIT_MASK;
		insn_len = (orig_trap->tinst & 0x2) ? INSN_LEN(insn) : 2;
	} else {
		/* trapped instruction value is zero or special value */
		insn = sbi_get_insn(regs->mepc, &uptrap);
		if (uptrap.cause) {
			return sbi_trap_redirect(regs, &uptrap);
		}
		insn_len = INSN_LEN(insn);
	}

	/**
	 * Common for RV32/RV64:
	 *    sb, sh, sw, fsw, fsd
	 *    c.sb, c.sh, c.sw, c.swsp, c.fsd, c.fsdsp
	 */
	if ((insn & INSN_MASK_SB) == INSN_MATCH_SB) {
		len = 1;
	} else if ((insn & INSN_MASK_C_SB) == INSN_MATCH_C_SB) {
		/* Zcb */
		len = 1;
		imm = RVC_SB_IMM(insn);
		c_store = true;
	} else if ((insn & INSN_MASK_SH) == INSN_MATCH_SH) {
		len = 2;
	} else if ((insn & INSN_MASK_C_SH) == INSN_MATCH_C_SH) {
		/* Zcb */
		len = 2;
		imm = RVC_SH_IMM(insn);
		c_store = true;
	} else if ((insn & INSN_MASK_SW) == INSN_MATCH_SW) {
		len = 4;
	} else if ((insn & INSN_MASK_C_SW) == INSN_MATCH_C_SW) {
		/* Zca */
		len = 4;
		imm = RVC_SW_IMM(insn);
		c_store = true;
	} else if ((insn & INSN_MASK_C_SWSP) == INSN_MATCH_C_SWSP) {
		/* Zca */
		len = 4;
		imm = RVC_SWSP_IMM(insn);
		c_stsp = true;
#ifdef __riscv_flen
	} else if ((insn & INSN_MASK_FSW) == INSN_MATCH_FSW) {
		len = 4;
		fp = true;
	} else if ((insn & INSN_MASK_FSD) == INSN_MATCH_FSD) {
		len = 8;
		fp = true;
	} else if ((insn & INSN_MASK_C_FSD) == INSN_MATCH_C_FSD) {
		/* Zcd */
		len = 8;
		imm = RVC_SD_IMM(insn);
		c_store = true;
		fp = true;
	} else if ((insn & INSN_MASK_C_FSDSP) == INSN_MATCH_C_FSDSP) {
		/* Zcd */
		len = 8;
		imm = RVC_SDSP_IMM(insn);
		c_stsp = true;
		fp = true;
#endif
	} else {
		prev_xlen = sbi_regs_prev_xlen(regs);
	}

	/**
	 * Must distinguish between rv64 and rv32, RVC instructions have
	 * overlapping encoding:
	 *     c.sd in rv64 == c.fsw in rv32
	 *     c.sdsp in rv64 == c.fswsp in rv32
	 */
	if (prev_xlen == 64) {
		/* RV64 Only: sd, c.sd, c.sdsp */
		if ((insn & INSN_MASK_SD) == INSN_MATCH_SD) {
			len = 8;
		} else if ((insn & INSN_MASK_C_SD) == INSN_MATCH_C_SD) {
			/* Zca */
			len = 8;
			imm = RVC_SD_IMM(insn);
			c_store = true;
		} else if ((insn & INSN_MASK_C_SDSP) == INSN_MATCH_C_SDSP) {
			/* Zca */
			len = 8;
			imm = RVC_SDSP_IMM(insn);
			c_stsp = true;
		}
#ifdef __riscv_flen
	} else if (prev_xlen == 32) {
		/* RV32 Only: c.fsw, c.fswsp */
		if ((insn & INSN_MASK_C_FSW) == INSN_MATCH_C_FSW) {
			/* Zcf */
			len = 4;
			imm = RVC_SW_IMM(insn);
			c_store = true;
			fp = true;
		} else if ((insn & INSN_MASK_C_FSWSP) == INSN_MATCH_C_FSWSP) {
			/* Zcf */
			len = 4;
			imm = RVC_SWSP_IMM(insn);
			c_stsp = true;
			fp = true;
		}
#endif
	}

	if (!fp) {
		if (c_store)
			val.data_ulong = GET_RS2S(insn, regs);
		else if (c_stsp)
			val.data_ulong = GET_RS2C(insn, regs);
		else
			val.data_ulong = GET_RS2(insn, regs);
#ifdef __riscv_flen
	} else if (len == 8) {
		if (c_store)
			val.data_u64 = GET_F64_RS2S(insn, regs);
		else if (c_stsp)
			val.data_u64 = GET_F64_RS2C(insn, regs);
		else
			val.data_u64 = GET_F64_RS2(insn, regs);
	} else {
		if (c_store)
			val.data_ulong = GET_F32_RS2S(insn, regs);
		else if (c_stsp)
			val.data_ulong = GET_F32_RS2C(insn, regs);
		else
			val.data_ulong = GET_F32_RS2(insn, regs);
#endif
	}

	if (!len) // Unknown instruction
		goto do_emu;

#if !defined(OPENSBI_DEBUG)
	/**
	 * For misaligned faults. Skip offset calculation unless DEBUG
	 * builds. It helps validating OpenSBI and HW.
	 */
	if (orig_trap->cause == CAUSE_MISALIGNED_STORE)
		goto do_emu;
#endif

	if (xform)
		/* Transformed insn */
		off = GET_RS1_NUM(insn);
	else if (c_store)
		/* non SP-based compressed store */
		off = orig_trap->tval - GET_RS1S(insn, regs) - imm;
	else if (c_stsp)
		/* SP-based compressed store */
		off = orig_trap->tval - REG_VAL(2, regs) - imm;
	else
		/* S-type non-compressed store */
		off = orig_trap->tval - GET_RS1(insn, regs) - (ulong)IMM_S(insn);
	/**
	 * Normalize offset, in case the XLEN of unpriv mode is smaller,
	 * and/or pointer masking is in effect
	 */
	off &= (len - 1);

do_emu:
	rc = emu(insn, len, orig_trap->tval - off, val, tcntx);
	if (rc <= 0)
		return rc;

	regs->mepc += insn_len;

	return 0;
}

static int sbi_misaligned_ld_emulator(ulong insn, int rlen, ulong addr,
				      union sbi_ldst_data *out_val,
				      struct sbi_trap_context *tcntx)
{
	const struct sbi_trap_info *orig_trap = &tcntx->trap;
	struct sbi_trap_regs *regs = &tcntx->regs;
	struct sbi_trap_info uptrap;
	int i;

	if (!rlen) {
		if (IS_VECTOR_LOAD_STORE(insn))
			return sbi_misaligned_v_ld_emulator(insn, tcntx);
		else
			/* Unrecognized instruction. Can't emulate it. */
			return sbi_trap_redirect(regs, orig_trap);
	}
	/* For misaligned fault, addr must be the same as orig_trap->tval */
	if (addr != orig_trap->tval)
		return SBI_EFAIL;

	for (i = 0; i < rlen; i++) {
		out_val->data_bytes[i] =
			sbi_load_u8((void *)(addr + i), &uptrap);
		if (uptrap.cause) {
			sbi_misaligned_tinst_fixup(orig_trap, &uptrap);
			return sbi_trap_redirect(regs, &uptrap);
		}
	}
	return rlen;
}

int sbi_misaligned_load_handler(struct sbi_trap_context *tcntx)
{
	return sbi_trap_emulate_load(tcntx, sbi_misaligned_ld_emulator);
}

static int sbi_misaligned_st_emulator(ulong insn, int wlen, ulong addr,
				      union sbi_ldst_data in_val,
				      struct sbi_trap_context *tcntx)
{
	const struct sbi_trap_info *orig_trap = &tcntx->trap;
	struct sbi_trap_regs *regs = &tcntx->regs;
	struct sbi_trap_info uptrap;
	int i;

	if (!wlen) {
		if (IS_VECTOR_LOAD_STORE(insn))
			return sbi_misaligned_v_st_emulator(insn, tcntx);
		else
			/* Unrecognized instruction. Can't emulate it. */
			return sbi_trap_redirect(regs, orig_trap);
	}
	/* For misaligned fault, addr must be the same as orig_trap->tval */
	if (addr != orig_trap->tval)
		return SBI_EFAIL;

	for (i = 0; i < wlen; i++) {
		sbi_store_u8((void *)(addr + i),
			     in_val.data_bytes[i], &uptrap);
		if (uptrap.cause) {
			sbi_misaligned_tinst_fixup(orig_trap, &uptrap);
			return sbi_trap_redirect(regs, &uptrap);
		}
	}
	return wlen;
}

int sbi_misaligned_store_handler(struct sbi_trap_context *tcntx)
{
	return sbi_trap_emulate_store(tcntx, sbi_misaligned_st_emulator);
}

static int sbi_ld_access_emulator(ulong insn, int rlen, ulong addr,
				  union sbi_ldst_data *out_val,
				  struct sbi_trap_context *tcntx)
{
	const struct sbi_trap_info *orig_trap = &tcntx->trap;
	struct sbi_trap_regs *regs = &tcntx->regs;
	int rc;

	/* If fault came from M mode, just fail */
	if (sbi_mstatus_prev_mode(regs->mstatus) == PRV_M)
		return SBI_EINVAL;

	rc = sbi_platform_emulate_load(sbi_platform_thishart_ptr(),
				       insn, rlen, addr, out_val, tcntx);

	/* If platform emulator failed, we redirect instead of fail */
	if (rc < 0)
		return sbi_trap_redirect(regs, orig_trap);

	return rc;
}

int sbi_load_access_handler(struct sbi_trap_context *tcntx)
{
	return sbi_trap_emulate_load(tcntx, sbi_ld_access_emulator);
}

static int sbi_st_access_emulator(ulong insn, int wlen, ulong addr,
				  union sbi_ldst_data in_val,
				  struct sbi_trap_context *tcntx)
{
	const struct sbi_trap_info *orig_trap = &tcntx->trap;
	struct sbi_trap_regs *regs = &tcntx->regs;
	int rc;

	/* If fault came from M mode, just fail */
	if (sbi_mstatus_prev_mode(regs->mstatus) == PRV_M)
		return SBI_EINVAL;

	rc = sbi_platform_emulate_store(sbi_platform_thishart_ptr(),
					insn, wlen, addr, in_val, tcntx);

	/* If platform emulator failed, we redirect instead of fail */
	if (rc < 0)
		return sbi_trap_redirect(regs, orig_trap);

	return rc;
}

int sbi_store_access_handler(struct sbi_trap_context *tcntx)
{
	return sbi_trap_emulate_store(tcntx, sbi_st_access_emulator);
}
