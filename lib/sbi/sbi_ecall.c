/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <sbi/sbi_console.h>
#include <sbi/sbi_ecall.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_ipi.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_system.h>
#include <sbi/sbi_timer.h>
#include <sbi/sbi_tlb.h>
#include <sbi/sbi_trap.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_version.h>
#include <sbi/riscv_asm.h>

#define SBI_ECALL_VERSION_MAJOR 0
#define SBI_ECALL_VERSION_MINOR 2
#define SBI_OPENSBI_IMPID 1

u16 sbi_ecall_version_major(void)
{
	return SBI_ECALL_VERSION_MAJOR;
}

u16 sbi_ecall_version_minor(void)
{
	return SBI_ECALL_VERSION_MINOR;
}

int sbi_check_extension(struct sbi_scratch *scratch, unsigned long extid,
			unsigned long *out_val)
{
	/**
	 * Each extension apart from base & 0.1, will be implemented as
	 * platform specific feature. Thus, extension probing can be achieved
	 * by checking the feature bits of the platform. We can create a map
	 * between extension ID & feature and use a generic function to check
	 * or just use a switch case for every new extension support added
	 * TODO: Implement it.
	 */

	if ((extid >= SBI_EXT_0_1_SET_TIMER &&
	    extid <= SBI_EXT_0_1_SHUTDOWN) || (extid == SBI_EXT_BASE)) {
		*out_val = 1;
	} else if (extid >= SBI_EXT_VENDOR_START &&
		   extid <= SBI_EXT_VENDOR_END) {
		*out_val = sbi_platform_vendor_ext_check(
						sbi_platform_ptr(scratch),
						extid);
	} else
		*out_val = 0;

	return 0;
}

int sbi_ecall_vendor_ext_handler(struct sbi_scratch *scratch,
				 unsigned long extid, unsigned long funcid,
				 unsigned long *args, unsigned long *out_val,
				 unsigned long *out_tcause,
				 unsigned long *out_tval)
{
	return sbi_platform_vendor_ext_provider(sbi_platform_ptr(scratch),
					       extid, funcid, args, out_val,
					       out_tcause, out_tval);
}

int sbi_ecall_base_handler(struct sbi_scratch *scratch, unsigned long extid,
			   unsigned long funcid, unsigned long *args,
			   unsigned long *out_val, unsigned long *out_tcause,
			   unsigned long *out_tval)
{
	int ret = 0;

	switch (funcid) {
	case SBI_EXT_BASE_GET_SPEC_VERSION:
		*out_val = (SBI_ECALL_VERSION_MAJOR <<
			   SBI_SPEC_VERSION_MAJOR_OFFSET) &
			   (SBI_SPEC_VERSION_MAJOR_MASK <<
			    SBI_SPEC_VERSION_MAJOR_OFFSET);
		*out_val = *out_val | SBI_ECALL_VERSION_MINOR;
		break;
	case SBI_EXT_BASE_GET_IMP_ID:
		*out_val = SBI_OPENSBI_IMPID;
		break;
	case SBI_EXT_BASE_GET_IMP_VERSION:
		*out_val = OPENSBI_VERSION;
		break;
	case SBI_EXT_BASE_GET_MVENDORID:
		*out_val = csr_read(CSR_MVENDORID);
		break;
	case SBI_EXT_BASE_GET_MARCHID:
		*out_val = csr_read(CSR_MARCHID);
		break;
	case SBI_EXT_BASE_GET_MIMPID:
		*out_val = csr_read(CSR_MIMPID);
		break;
	case SBI_EXT_BASE_PROBE_EXT:
		ret = sbi_check_extension(scratch, args[0], out_val);
	default:
		ret = SBI_ENOTSUPP;
	}

	return ret;
}

int sbi_ecall_0_1_handler(struct sbi_scratch *scratch, unsigned long extid,
			   unsigned long *args, unsigned long *tval,
			   unsigned long *tcause)
{
	int ret = 0;
	struct sbi_tlb_info tlb_info;
	u32 source_hart = sbi_current_hartid();
	struct unpriv_trap uptrap = {0};

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
		ret = sbi_ipi_send_many(scratch, &uptrap, (ulong *)args[0],
					SBI_IPI_EVENT_SOFT, NULL);
		break;
	case SBI_EXT_0_1_REMOTE_FENCE_I:
		tlb_info.start  = 0;
		tlb_info.size  = 0;
		tlb_info.type  = SBI_ITLB_FLUSH;
		tlb_info.shart_mask = 1UL << source_hart;
		ret = sbi_ipi_send_many(scratch, &uptrap, (ulong *)args[0],
					SBI_IPI_EVENT_FENCE_I, &tlb_info);
		break;
	case SBI_EXT_0_1_REMOTE_SFENCE_VMA:
		tlb_info.start = (unsigned long)args[1];
		tlb_info.size  = (unsigned long)args[2];
		tlb_info.type  = SBI_TLB_FLUSH_VMA;
		tlb_info.shart_mask = 1UL << source_hart;

		ret = sbi_ipi_send_many(scratch, &uptrap, (ulong *)args[0],
					SBI_IPI_EVENT_SFENCE_VMA, &tlb_info);
		break;
	case SBI_EXT_0_1_REMOTE_SFENCE_VMA_ASID:
		tlb_info.start = (unsigned long)args[1];
		tlb_info.size  = (unsigned long)args[2];
		tlb_info.asid  = (unsigned long)args[3];
		tlb_info.type  = SBI_TLB_FLUSH_VMA_ASID;
		tlb_info.shart_mask = 1UL << source_hart;

		ret = sbi_ipi_send_many(scratch, &uptrap, (ulong *)args[0],
					SBI_IPI_EVENT_SFENCE_VMA_ASID,
					&tlb_info);
		break;
	case SBI_EXT_0_1_SHUTDOWN:
		sbi_system_shutdown(scratch, 0);
		break;
	default:
		ret = SBI_ENOTSUPP;
	};

	if (ret == SBI_ETRAP) {
		*tcause = uptrap.cause;
		*tval = uptrap.tval;
	}
	return ret;
}

int sbi_ecall_handler(u32 hartid, ulong mcause, struct sbi_trap_regs *regs,
		      struct sbi_scratch *scratch)
{
	int ret = 0;
	unsigned long extension_id = regs->a7;
	unsigned long func_id = regs->a6;
	unsigned long out_val;
	unsigned long out_tval;
	unsigned long out_tcause;
	bool is_0_1_spec = 0;
	unsigned long args[6];

	args[0] = regs->a0;
	args[1] = regs->a1;
	args[2] = regs->a2;
	args[3] = regs->a3;
	args[4] = regs->a4;
	args[5] = regs->a5;

	if (extension_id >= SBI_EXT_0_1_SET_TIMER &&
	    extension_id <= SBI_EXT_0_1_SHUTDOWN) {
		ret = sbi_ecall_0_1_handler(scratch, extension_id, args,
					    &out_tval, &out_tcause);
		is_0_1_spec = 1;
	} else if (extension_id == SBI_EXT_BASE)
		ret = sbi_ecall_base_handler(scratch, extension_id, func_id,
					     args, &out_val,
					     &out_tval, &out_tcause);
	else if (extension_id >= SBI_EXT_VENDOR_START &&
		extension_id <= SBI_EXT_VENDOR_END) {
		ret = sbi_ecall_vendor_ext_handler(scratch, extension_id,
						   func_id, args, &out_val,
						   &out_tval, &out_tcause);
	} else {
		ret = SBI_ENOTSUPP;
	}

	if (ret == SBI_ETRAP) {
		sbi_trap_redirect(regs, scratch, regs->mepc,
				  out_tcause, out_tval);
	} else {
		/* This function should return non-zero value only in case of
		 * fatal error. However, there is no good way to distinguish
		 * between a fatal and non-fatal errors yet. That's why we treat
		 * every return value except ETRAP as non-fatal and just return
		 * accordingly for now. Once fatal errors are defined, that
		 * case should be handled differently.
		 */
		regs->mepc += 4;
		regs->a0 = ret;
		if (!is_0_1_spec)
			regs->a1 = out_val;
	}

	return 0;
}
