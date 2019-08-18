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
#include <sbi/sbi_bits.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_emulate_csr.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_timer.h>
#include <sbi/sbi_trap.h>

int sbi_emulate_csr_read(int csr_num, u32 hartid, struct sbi_trap_regs *regs,
			 struct sbi_scratch *scratch, ulong *csr_val)
{
	int ret = 0;
	ulong cen = -1UL;
	ulong prev_mode = (regs->mstatus & MSTATUS_MPP) >> MSTATUS_MPP_SHIFT;
#if __riscv_xlen == 32
	bool virt = (regs->mstatusH & MSTATUSH_MPV) ? TRUE : FALSE;
#else
	bool virt = (regs->mstatus & MSTATUS_MPV) ? TRUE : FALSE;
#endif

	if (prev_mode == PRV_U)
		cen = csr_read(CSR_SCOUNTEREN);

	switch (csr_num) {
	case CSR_HTIMEDELTA:
		if (prev_mode == PRV_S && !virt)
			*csr_val = sbi_timer_get_delta(scratch);
		else
			ret = SBI_ENOTSUPP;
		break;
	case CSR_CYCLE:
		if (!((cen >> (CSR_CYCLE - CSR_CYCLE)) & 1))
			return -1;
		*csr_val = csr_read(CSR_MCYCLE);
		break;
	case CSR_TIME:
		if (!((cen >> (CSR_TIME - CSR_CYCLE)) & 1))
			return -1;
		*csr_val = (virt) ? sbi_timer_virt_value(scratch):
				    sbi_timer_value(scratch);
		break;
	case CSR_INSTRET:
		if (!((cen >> (CSR_INSTRET - CSR_CYCLE)) & 1))
			return -1;
		*csr_val = csr_read(CSR_MINSTRET);
		break;
	case CSR_MHPMCOUNTER3:
		if (!((cen >> (3 + CSR_MHPMCOUNTER3 - CSR_MHPMCOUNTER3)) & 1))
			return -1;
		*csr_val = csr_read(CSR_MHPMCOUNTER3);
		break;
	case CSR_MHPMCOUNTER4:
		if (!((cen >> (3 + CSR_MHPMCOUNTER4 - CSR_MHPMCOUNTER3)) & 1))
			return -1;
		*csr_val = csr_read(CSR_MHPMCOUNTER4);
		break;
#if __riscv_xlen == 32
	case CSR_HTIMEDELTAH:
		if (prev_mode == PRV_S && !virt)
			*csr_val = sbi_timer_get_delta(scratch) >> 32;
		else
			ret = SBI_ENOTSUPP;
		break;
	case CSR_CYCLEH:
		if (!((cen >> (CSR_CYCLE - CSR_CYCLE)) & 1))
			return -1;
		*csr_val = csr_read(CSR_MCYCLEH);
		break;
	case CSR_TIMEH:
		if (!((cen >> (CSR_TIME - CSR_CYCLE)) & 1))
			return -1;
		*csr_val = (virt) ? sbi_timer_virt_value(scratch) >> 32:
				    sbi_timer_value(scratch) >> 32;
		break;
	case CSR_INSTRETH:
		if (!((cen >> (CSR_INSTRET - CSR_CYCLE)) & 1))
			return -1;
		*csr_val = csr_read(CSR_MINSTRETH);
		break;
	case CSR_MHPMCOUNTER3H:
		if (!((cen >> (3 + CSR_MHPMCOUNTER3 - CSR_MHPMCOUNTER3)) & 1))
			return -1;
		*csr_val = csr_read(CSR_MHPMCOUNTER3H);
		break;
	case CSR_MHPMCOUNTER4H:
		if (!((cen >> (3 + CSR_MHPMCOUNTER4 - CSR_MHPMCOUNTER3)) & 1))
			return -1;
		*csr_val = csr_read(CSR_MHPMCOUNTER4H);
		break;
#endif
	case CSR_MHPMEVENT3:
		*csr_val = csr_read(CSR_MHPMEVENT3);
		break;
	case CSR_MHPMEVENT4:
		*csr_val = csr_read(CSR_MHPMEVENT4);
		break;
	default:
		ret = SBI_ENOTSUPP;
		break;
	};

	if (ret)
		sbi_dprintf(scratch, "%s: hartid%d: invalid csr_num=0x%x\n",
			    __func__, hartid, csr_num);

	return ret;
}

int sbi_emulate_csr_write(int csr_num, u32 hartid, struct sbi_trap_regs *regs,
			  struct sbi_scratch *scratch, ulong csr_val)
{
	int ret = 0;
	ulong prev_mode = (regs->mstatus & MSTATUS_MPP) >> MSTATUS_MPP_SHIFT;
#if __riscv_xlen == 32
	bool virt = (regs->mstatusH & MSTATUSH_MPV) ? TRUE : FALSE;
#else
	bool virt = (regs->mstatus & MSTATUS_MPV) ? TRUE : FALSE;
#endif

	switch (csr_num) {
	case CSR_HTIMEDELTA:
		if (prev_mode == PRV_S && !virt)
			sbi_timer_set_delta(scratch, csr_val);
		else
			ret = SBI_ENOTSUPP;
		break;
	case CSR_CYCLE:
		csr_write(CSR_MCYCLE, csr_val);
		break;
	case CSR_INSTRET:
		csr_write(CSR_MINSTRET, csr_val);
		break;
	case CSR_MHPMCOUNTER3:
		csr_write(CSR_MHPMCOUNTER3, csr_val);
		break;
	case CSR_MHPMCOUNTER4:
		csr_write(CSR_MHPMCOUNTER4, csr_val);
		break;
#if __riscv_xlen == 32
	case CSR_HTIMEDELTAH:
		if (prev_mode == PRV_S && !virt)
			sbi_timer_set_delta_upper(scratch, csr_val);
		else
			ret = SBI_ENOTSUPP;
		break;
	case CSR_CYCLEH:
		csr_write(CSR_MCYCLEH, csr_val);
		break;
	case CSR_INSTRETH:
		csr_write(CSR_MINSTRETH, csr_val);
		break;
	case CSR_MHPMCOUNTER3H:
		csr_write(CSR_MHPMCOUNTER3H, csr_val);
		break;
	case CSR_MHPMCOUNTER4H:
		csr_write(CSR_MHPMCOUNTER4H, csr_val);
		break;
#endif
	case CSR_MHPMEVENT3:
		csr_write(CSR_MHPMEVENT3, csr_val);
		break;
	case CSR_MHPMEVENT4:
		csr_write(CSR_MHPMEVENT4, csr_val);
		break;
	default:
		ret = SBI_ENOTSUPP;
		break;
	};

	if (ret)
		sbi_dprintf(scratch, "%s: hartid%d: invalid csr_num=0x%x\n",
			    __func__, hartid, csr_num);

	return ret;
}
