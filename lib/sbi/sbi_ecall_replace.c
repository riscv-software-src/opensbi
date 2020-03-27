/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 *   Atish Patra <atish.patra@wdc.com>
 */

#include <sbi/riscv_asm.h>
#include <sbi/sbi_ecall.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_ipi.h>
#include <sbi/sbi_timer.h>
#include <sbi/sbi_tlb.h>

static int sbi_ecall_time_handler(unsigned long extid, unsigned long funcid,
				  unsigned long *args, unsigned long *out_val,
				  struct sbi_trap_info *out_trap)
{
	int ret = 0;

	if (funcid == SBI_EXT_TIME_SET_TIMER) {
#if __riscv_xlen == 32
		sbi_timer_event_start((((u64)args[1] << 32) | (u64)args[0]));
#else
		sbi_timer_event_start((u64)args[0]);
#endif
	} else
		ret = SBI_ENOTSUPP;

	return ret;
}

struct sbi_ecall_extension ecall_time = {
	.extid_start = SBI_EXT_TIME,
	.extid_end = SBI_EXT_TIME,
	.handle = sbi_ecall_time_handler,
};

static int sbi_ecall_rfence_handler(unsigned long extid, unsigned long funcid,
				    unsigned long *args, unsigned long *out_val,
				    struct sbi_trap_info *out_trap)
{
	int ret = 0;
	struct sbi_tlb_info tlb_info;
	u32 source_hart = current_hartid();

	if (funcid >= SBI_EXT_RFENCE_REMOTE_HFENCE_GVMA &&
	    funcid <= SBI_EXT_RFENCE_REMOTE_HFENCE_VVMA_ASID)
		if (!misa_extension('H'))
			return SBI_ENOTSUPP;

	switch (funcid) {
	case SBI_EXT_RFENCE_REMOTE_FENCE_I:
		SBI_TLB_INFO_INIT(&tlb_info, 0, 0, 0,
				  SBI_ITLB_FLUSH, source_hart);
		ret = sbi_tlb_request(args[0], args[1], &tlb_info);
		break;
	case SBI_EXT_RFENCE_REMOTE_HFENCE_GVMA:
		SBI_TLB_INFO_INIT(&tlb_info, args[2], args[3], 0,
				  SBI_TLB_FLUSH_GVMA, source_hart);
		ret = sbi_tlb_request(args[0], args[1], &tlb_info);
		break;
	case SBI_EXT_RFENCE_REMOTE_HFENCE_GVMA_VMID:
		SBI_TLB_INFO_INIT(&tlb_info, args[2], args[3], args[4],
				  SBI_TLB_FLUSH_GVMA_VMID, source_hart);
		ret = sbi_tlb_request(args[0], args[1], &tlb_info);
		break;
	case SBI_EXT_RFENCE_REMOTE_HFENCE_VVMA:
		SBI_TLB_INFO_INIT(&tlb_info, args[2], args[3], 0,
				  SBI_TLB_FLUSH_VVMA, source_hart);
		ret = sbi_tlb_request(args[0], args[1], &tlb_info);
		break;
	case SBI_EXT_RFENCE_REMOTE_HFENCE_VVMA_ASID:
		SBI_TLB_INFO_INIT(&tlb_info, args[2], args[3], args[4],
				  SBI_TLB_FLUSH_VVMA_ASID, source_hart);
		ret = sbi_tlb_request(args[0], args[1], &tlb_info);
		break;
	case SBI_EXT_RFENCE_REMOTE_SFENCE_VMA:
		SBI_TLB_INFO_INIT(&tlb_info, args[2], args[3], 0,
				  SBI_TLB_FLUSH_VMA, source_hart);
		ret = sbi_tlb_request(args[0], args[1], &tlb_info);
		break;
	case SBI_EXT_RFENCE_REMOTE_SFENCE_VMA_ASID:
		SBI_TLB_INFO_INIT(&tlb_info, args[2], args[3], args[4],
				  SBI_TLB_FLUSH_VMA_ASID, source_hart);
		ret = sbi_tlb_request(args[0], args[1], &tlb_info);
		break;
	default:
		ret = SBI_ENOTSUPP;
	};

	return ret;
}

struct sbi_ecall_extension ecall_rfence = {
	.extid_start = SBI_EXT_RFENCE,
	.extid_end = SBI_EXT_RFENCE,
	.handle = sbi_ecall_rfence_handler,
};

static int sbi_ecall_ipi_handler(unsigned long extid, unsigned long funcid,
				 unsigned long *args, unsigned long *out_val,
				 struct sbi_trap_info *out_trap)
{
	int ret = 0;

	if (funcid == SBI_EXT_IPI_SEND_IPI)
		ret = sbi_ipi_send_smode(args[0], args[1]);
	else
		ret = SBI_ENOTSUPP;

	return ret;
}

struct sbi_ecall_extension ecall_ipi = {
	.extid_start = SBI_EXT_IPI,
	.extid_end = SBI_EXT_IPI,
	.handle = sbi_ecall_ipi_handler,
};
