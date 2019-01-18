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

int sbi_emulate_csr_read(int csr_num,
			 u32 hartid, ulong mstatus,
			 struct sbi_scratch *scratch,
			 ulong *csr_val)
{
	ulong cen = -1UL;

	if (EXTRACT_FIELD(mstatus, MSTATUS_MPP) == PRV_U)
		cen = csr_read(scounteren);

	switch (csr_num) {
	case CSR_CYCLE:
		if (!((cen >> (CSR_CYCLE - CSR_CYCLE)) & 1))
			return -1;
		*csr_val = csr_read(mcycle);
		break;
	case CSR_TIME:
		if (!((cen >> (CSR_TIME - CSR_CYCLE)) & 1))
			return -1;
		*csr_val = sbi_timer_value(scratch);
		break;
	case CSR_INSTRET:
		if (!((cen >> (CSR_INSTRET - CSR_CYCLE)) & 1))
			return -1;
		*csr_val = csr_read(minstret);
		break;
	case CSR_MHPMCOUNTER3:
		if (!((cen >> (3 + CSR_MHPMCOUNTER3 - CSR_MHPMCOUNTER3)) & 1))
			return -1;
		*csr_val = csr_read(mhpmcounter3);
		break;
	case CSR_MHPMCOUNTER4:
		if (!((cen >> (3 + CSR_MHPMCOUNTER4 - CSR_MHPMCOUNTER3)) & 1))
			return -1;
		*csr_val = csr_read(mhpmcounter4);
		break;
#if __riscv_xlen == 32
	case CSR_CYCLEH:
		if (!((cen >> (CSR_CYCLE - CSR_CYCLE)) & 1))
			return -1;
		*csr_val = csr_read(mcycleh);
		break;
	case CSR_TIMEH:
		if (!((cen >> (CSR_TIME - CSR_CYCLE)) & 1))
			return -1;
		*csr_val = sbi_timer_value(scratch) >> 32;
		break;
	case CSR_INSTRETH:
		if (!((cen >> (CSR_INSTRET - CSR_CYCLE)) & 1))
			return -1;
		*csr_val = csr_read(minstreth);
		break;
	case CSR_MHPMCOUNTER3H:
		if (!((cen >> (3 + CSR_MHPMCOUNTER3 - CSR_MHPMCOUNTER3)) & 1))
			return -1;
		*csr_val = csr_read(mhpmcounter3h);
		break;
	case CSR_MHPMCOUNTER4H:
		if (!((cen >> (3 + CSR_MHPMCOUNTER4 - CSR_MHPMCOUNTER3)) & 1))
			return -1;
		*csr_val = csr_read(mhpmcounter4h);
		break;
#endif
	case CSR_MHPMEVENT3:
		*csr_val = csr_read(mhpmevent3);
		break;
	case CSR_MHPMEVENT4:
		*csr_val = csr_read(mhpmevent4);
		break;
	default:
		sbi_printf("%s: hartid%d: invalid csr_num=0x%x\n",
			   __func__, hartid, csr_num);
		return SBI_ENOTSUPP;
	};

	return 0;
}

int sbi_emulate_csr_write(int csr_num,
			  u32 hartid, ulong mstatus,
			  struct sbi_scratch *scratch,
			  ulong csr_val)
{
	switch (csr_num) {
	case CSR_CYCLE:
		csr_write(mcycle, csr_val);
		break;
	case CSR_INSTRET:
		csr_write(minstret, csr_val);
		break;
	case CSR_MHPMCOUNTER3:
		csr_write(mhpmcounter3, csr_val);
		break;
	case CSR_MHPMCOUNTER4:
		csr_write(mhpmcounter4, csr_val);
		break;
#if __riscv_xlen == 32
	case CSR_CYCLEH:
		csr_write(mcycleh, csr_val);
		break;
	case CSR_INSTRETH:
		csr_write(minstreth, csr_val);
		break;
	case CSR_MHPMCOUNTER3H:
		csr_write(mhpmcounter3h, csr_val);
		break;
	case CSR_MHPMCOUNTER4H:
		csr_write(mhpmcounter4h, csr_val);
		break;
#endif
	case CSR_MHPMEVENT3:
		csr_write(mhpmevent3, csr_val);
		break;
	case CSR_MHPMEVENT4:
		csr_write(mhpmevent4, csr_val);
		break;
	default:
		sbi_printf("%s: hartid%d: invalid csr_num=0x%x\n",
			   __func__, hartid, csr_num);
		return SBI_ENOTSUPP;
	};

	return 0;
}
