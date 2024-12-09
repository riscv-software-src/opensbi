/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024 SiFive Inc.
 *
 * Authors:
 *   Andrew Waterman <andrew@sifive.com>
 *   Nylon Chen <nylon.chen@sifive.com>
 *   Zong Li <nylon.chen@sifive.com>
 */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_encoding.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_trap_ldst.h>
#include <sbi/sbi_trap.h>
#include <sbi/sbi_unpriv.h>
#include <sbi/sbi_trap.h>

#ifdef OPENSBI_CC_SUPPORT_VECTOR

#define VLEN_MAX 65536

static inline void set_vreg(ulong vlenb, ulong which,
			    ulong pos, ulong size, const uint8_t *bytes)
{
	pos += (which % 8) * vlenb;
	bytes -= pos;

	asm volatile (
		"	.option push\n\t"
		"	.option arch, +v\n\t"
		"	vsetvli x0, %0, e8, m8, tu, ma\n\t"
		"	.option pop\n\t"
		:: "r" (pos + size));

	csr_write(CSR_VSTART, pos);

	switch (which / 8) {
	case 0:
		asm volatile (
			"	.option push\n\t"
			"	.option arch, +v\n\t"
			"	vle8.v v0,  (%0)\n\t"
			"	.option pop\n\t"
			:: "r" (bytes) : "memory");
		break;
	case 1:
		asm volatile (
			"	.option push\n\t"
			"	.option arch, +v\n\t"
			"	vle8.v v8,  (%0)\n\t"
			"	.option pop\n\t"
			:: "r" (bytes) : "memory");
		break;
	case 2:
		asm volatile (
			"	.option push\n\t"
			"	.option arch, +v\n\t"
			"	vle8.v v16,  (%0)\n\t"
			"	.option pop\n\t"
			:: "r" (bytes) : "memory");
		break;
	case 3:
		asm volatile (
			"	.option push\n\t"
			"	.option arch, +v\n\t"
			"	vle8.v v24,  (%0)\n\t"
			"	.option pop\n\t"
			:: "r" (bytes) : "memory");
		break;
	default:
		break;
	}
}

static inline void get_vreg(ulong vlenb, ulong which,
			    ulong pos, ulong size, uint8_t *bytes)
{
	pos += (which % 8) * vlenb;
	bytes -= pos;

	asm volatile (
		"	.option push\n\t"
		"	.option arch, +v\n\t"
		"	vsetvli x0, %0, e8, m8, tu, ma\n\t"
		"	.option pop\n\t"
		:: "r" (pos + size));

	csr_write(CSR_VSTART, pos);

	switch (which / 8) {
	case 0:
		asm volatile (
			"	.option push\n\t"
			"	.option arch, +v\n\t"
			"	vse8.v v0,  (%0)\n\t"
			"	.option pop\n\t"
			:: "r" (bytes) : "memory");
		break;
	case 1:
		asm volatile (
			"	.option push\n\t"
			"	.option arch, +v\n\t"
			"	vse8.v v8,  (%0)\n\t"
			"	.option pop\n\t"
			:: "r" (bytes) : "memory");
		break;
	case 2:
		asm volatile (
			"	.option push\n\t"
			"	.option arch, +v\n\t"
			"	vse8.v v16, (%0)\n\t"
			"	.option pop\n\t"
			:: "r" (bytes) : "memory");
		break;
	case 3:
		asm volatile (
				".option push\n\t"
				".option arch, +v\n\t"
				"vse8.v v24, (%0)\n\t"
				".option pop\n\t"
				:: "r" (bytes) : "memory");
		break;
	default:
		break;
	}
}

static inline void vsetvl(ulong vl, ulong vtype)
{
	asm volatile (
		"	.option push\n\t"
		"	.option arch, +v\n\t"
		"	vsetvl x0, %0, %1\n\t"
		"	.option pop\n\t"
			:: "r" (vl), "r" (vtype));
}

int sbi_misaligned_v_ld_emulator(int rlen, union sbi_ldst_data *out_val,
				 struct sbi_trap_context *tcntx)
{
	const struct sbi_trap_info *orig_trap = &tcntx->trap;
	struct sbi_trap_regs *regs = &tcntx->regs;
	struct sbi_trap_info uptrap;
	ulong insn = sbi_get_insn(regs->mepc, &uptrap);
	ulong vl = csr_read(CSR_VL);
	ulong vtype = csr_read(CSR_VTYPE);
	ulong vlenb = csr_read(CSR_VLENB);
	ulong vstart = csr_read(CSR_VSTART);
	ulong base = GET_RS1(insn, regs);
	ulong stride = GET_RS2(insn, regs);
	ulong vd = GET_VD(insn);
	ulong vs2 = GET_VS2(insn);
	ulong view = GET_VIEW(insn);
	ulong vsew = GET_VSEW(vtype);
	ulong vlmul = GET_VLMUL(vtype);
	bool illegal = GET_MEW(insn);
	bool masked = IS_MASKED(insn);
	uint8_t mask[VLEN_MAX / 8];
	uint8_t bytes[8 * sizeof(uint64_t)];
	ulong len = GET_LEN(view);
	ulong nf = GET_NF(insn);
	ulong vemul = GET_VEMUL(vlmul, view, vsew);
	ulong emul = GET_EMUL(vemul);

	if (IS_UNIT_STRIDE_LOAD(insn) || IS_FAULT_ONLY_FIRST_LOAD(insn)) {
		stride = nf * len;
	} else if (IS_WHOLE_REG_LOAD(insn)) {
		vl = (nf * vlenb) >> view;
		nf = 1;
		vemul = 0;
		emul = 1;
		stride = nf * len;
	} else if (IS_INDEXED_LOAD(insn)) {
		len = 1 << vsew;
		vemul = (vlmul + vsew - vsew) & 7;
		emul = 1 << ((vemul & 4) ? 0 : vemul);
		stride = nf * len;
	}

	if (illegal || vlenb > VLEN_MAX / 8) {
		struct sbi_trap_info trap = {
			uptrap.cause = CAUSE_ILLEGAL_INSTRUCTION,
			uptrap.tval = insn,
		};
		return sbi_trap_redirect(regs, &trap);
	}

	if (masked)
		get_vreg(vlenb, 0, 0, vlenb, mask);

	do {
		if (!masked || ((mask[vstart / 8] >> (vstart % 8)) & 1)) {
			/* compute element address */
			ulong addr = base + vstart * stride;

			if (IS_INDEXED_LOAD(insn)) {
				ulong offset = 0;

				get_vreg(vlenb, vs2, vstart << view, 1 << view, (uint8_t *)&offset);
				addr = base + offset;
			}

			csr_write(CSR_VSTART, vstart);

			/* obtain load data from memory */
			for (ulong seg = 0; seg < nf; seg++) {
				for (ulong i = 0; i < len; i++) {
					bytes[seg * len + i] =
						sbi_load_u8((void *)(addr + seg * len + i),
							    &uptrap);

					if (uptrap.cause) {
						if (IS_FAULT_ONLY_FIRST_LOAD(insn) && vstart != 0) {
							vl = vstart;
							break;
						}
						vsetvl(vl, vtype);
						uptrap.tinst = sbi_misaligned_tinst_fixup(
							orig_trap->tinst, uptrap.tinst, i);
						return sbi_trap_redirect(regs, &uptrap);
					}
				}
			}

			/* write load data to regfile */
			for (ulong seg = 0; seg < nf; seg++)
				set_vreg(vlenb, vd + seg * emul, vstart * len,
					 len, &bytes[seg * len]);
		}
	} while (++vstart < vl);

	/* restore clobbered vl/vtype */
	vsetvl(vl, vtype);

	return vl;
}

int sbi_misaligned_v_st_emulator(int wlen, union sbi_ldst_data in_val,
				 struct sbi_trap_context *tcntx)
{
	const struct sbi_trap_info *orig_trap = &tcntx->trap;
	struct sbi_trap_regs *regs = &tcntx->regs;
	struct sbi_trap_info uptrap;
	ulong insn = sbi_get_insn(regs->mepc, &uptrap);
	ulong vl = csr_read(CSR_VL);
	ulong vtype = csr_read(CSR_VTYPE);
	ulong vlenb = csr_read(CSR_VLENB);
	ulong vstart = csr_read(CSR_VSTART);
	ulong base = GET_RS1(insn, regs);
	ulong stride = GET_RS2(insn, regs);
	ulong vd = GET_VD(insn);
	ulong vs2 = GET_VS2(insn);
	ulong view = GET_VIEW(insn);
	ulong vsew = GET_VSEW(vtype);
	ulong vlmul = GET_VLMUL(vtype);
	bool illegal = GET_MEW(insn);
	bool masked = IS_MASKED(insn);
	uint8_t mask[VLEN_MAX / 8];
	uint8_t bytes[8 * sizeof(uint64_t)];
	ulong len = GET_LEN(view);
	ulong nf = GET_NF(insn);
	ulong vemul = GET_VEMUL(vlmul, view, vsew);
	ulong emul = GET_EMUL(vemul);

	if (IS_UNIT_STRIDE_STORE(insn)) {
		stride = nf * len;
	} else if (IS_WHOLE_REG_STORE(insn)) {
		vl = (nf * vlenb) >> view;
		nf = 1;
		vemul = 0;
		emul = 1;
		stride = nf * len;
	} else if (IS_INDEXED_STORE(insn)) {
		len = 1 << vsew;
		vemul = (vlmul + vsew - vsew) & 7;
		emul = 1 << ((vemul & 4) ? 0 : vemul);
		stride = nf * len;
	}

	if (illegal || vlenb > VLEN_MAX / 8) {
		struct sbi_trap_info trap = {
			uptrap.cause = CAUSE_ILLEGAL_INSTRUCTION,
			uptrap.tval = insn,
		};
		return sbi_trap_redirect(regs, &trap);
	}

	if (masked)
		get_vreg(vlenb, 0, 0, vlenb, mask);

	do {
		if (!masked || ((mask[vstart / 8] >> (vstart % 8)) & 1)) {
			/* compute element address */
			ulong addr = base + vstart * stride;

			if (IS_INDEXED_STORE(insn)) {
				ulong offset = 0;

				get_vreg(vlenb, vs2, vstart << view, 1 << view, (uint8_t *)&offset);
				addr = base + offset;
			}

			/* obtain store data from regfile */
			for (ulong seg = 0; seg < nf; seg++)
				get_vreg(vlenb, vd + seg * emul, vstart * len,
					 len, &bytes[seg * len]);

			csr_write(CSR_VSTART, vstart);

			/* write store data to memory */
			for (ulong seg = 0; seg < nf; seg++) {
				for (ulong i = 0; i < len; i++) {
					sbi_store_u8((void *)(addr + seg * len + i),
						     bytes[seg * len + i], &uptrap);
					if (uptrap.cause) {
						vsetvl(vl, vtype);
						uptrap.tinst = sbi_misaligned_tinst_fixup(
							orig_trap->tinst, uptrap.tinst, i);
						return sbi_trap_redirect(regs, &uptrap);
					}
				}
			}
		}
	} while (++vstart < vl);

	/* restore clobbered vl/vtype */
	vsetvl(vl, vtype);

	return vl;
}
#else
int sbi_misaligned_v_ld_emulator(int rlen, union sbi_ldst_data *out_val,
				 struct sbi_trap_context *tcntx)
{
	return 0;
}
int sbi_misaligned_v_st_emulator(int wlen, union sbi_ldst_data in_val,
				 struct sbi_trap_context *tcntx)
{
	return 0;
}
#endif /* OPENSBI_CC_SUPPORT_VECTOR */
