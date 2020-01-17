/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 *   Atish Patra <atish.patra@wdc.com>
 */

#include <sbi/sbi_console.h>
#include <sbi/sbi_ecall.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_ipi.h>
#include <sbi/sbi_system.h>
#include <sbi/sbi_timer.h>
#include <sbi/sbi_tlb.h>
#include <sbi/sbi_trap.h>
#include <sbi/sbi_unpriv.h>
#include <sbi/sbi_hart.h>

static int sbi_load_hart_mask_unpriv(struct sbi_scratch *scratch,
				     ulong *pmask, ulong *hmask,
				     struct sbi_trap_info *uptrap)
{
	ulong mask = 0;

	if (pmask) {
		mask = sbi_load_ulong(pmask, scratch, uptrap);
		if (uptrap->cause)
			return SBI_ETRAP;
	} else {
		mask = sbi_hart_available_mask();
	}
	*hmask = mask;
	return 0;
}

static int sbi_ecall_legacy_handler(struct sbi_scratch *scratch,
				    unsigned long extid, unsigned long funcid,
				    unsigned long *args, unsigned long *out_val,
				    struct sbi_trap_info *out_trap)
{
	int ret = 0;
	struct sbi_tlb_info tlb_info;
	u32 source_hart = sbi_current_hartid();
	ulong hmask = 0;

	switch (extid) {
	case SBI_EXT_0_1_SET_TIMER:
#if __riscv_xlen == 32
		sbi_timer_event_start(scratch,
				      (((u64)args[1] << 32) | (u64)args[0]));
#else
		sbi_timer_event_start(scratch, (u64)args[0]);
#endif
		break;
	case SBI_EXT_0_1_CONSOLE_PUTCHAR:
		sbi_putc(args[0]);
		break;
	case SBI_EXT_0_1_CONSOLE_GETCHAR:
		ret = sbi_getc();
		break;
	case SBI_EXT_0_1_CLEAR_IPI:
		sbi_ipi_clear_smode(scratch);
		break;
	case SBI_EXT_0_1_SEND_IPI:
		ret = sbi_load_hart_mask_unpriv(scratch, (ulong *)args[0],
						&hmask, out_trap);
		if (ret != SBI_ETRAP)
			ret = sbi_ipi_send_smode(scratch, hmask, 0);
		break;
	case SBI_EXT_0_1_REMOTE_FENCE_I:
		tlb_info.start  = 0;
		tlb_info.size  = 0;
		tlb_info.type  = SBI_ITLB_FLUSH;
		tlb_info.shart_mask = 1UL << source_hart;
		ret = sbi_load_hart_mask_unpriv(scratch, (ulong *)args[0],
						&hmask, out_trap);
		if (ret != SBI_ETRAP)
			ret = sbi_tlb_request(scratch, hmask, 0, &tlb_info);
		break;
	case SBI_EXT_0_1_REMOTE_SFENCE_VMA:
		tlb_info.start = (unsigned long)args[1];
		tlb_info.size  = (unsigned long)args[2];
		tlb_info.type  = SBI_TLB_FLUSH_VMA;
		tlb_info.shart_mask = 1UL << source_hart;
		ret = sbi_load_hart_mask_unpriv(scratch, (ulong *)args[0],
						&hmask, out_trap);
		if (ret != SBI_ETRAP)
			ret = sbi_tlb_request(scratch, hmask, 0, &tlb_info);
		break;
	case SBI_EXT_0_1_REMOTE_SFENCE_VMA_ASID:
		tlb_info.start = (unsigned long)args[1];
		tlb_info.size  = (unsigned long)args[2];
		tlb_info.asid  = (unsigned long)args[3];
		tlb_info.type  = SBI_TLB_FLUSH_VMA_ASID;
		tlb_info.shart_mask = 1UL << source_hart;
		ret = sbi_load_hart_mask_unpriv(scratch, (ulong *)args[0],
						&hmask, out_trap);
		if (ret != SBI_ETRAP)
			ret = sbi_tlb_request(scratch, hmask, 0, &tlb_info);
		break;
	case SBI_EXT_0_1_SHUTDOWN:
		sbi_system_shutdown(scratch, 0);
		break;
	default:
		ret = SBI_ENOTSUPP;
	};

	return ret;
}

struct sbi_ecall_extension ecall_legacy = {
	.extid_start = SBI_EXT_0_1_SET_TIMER,
	.extid_end = SBI_EXT_0_1_SHUTDOWN,
	.handle = sbi_ecall_legacy_handler,
};
