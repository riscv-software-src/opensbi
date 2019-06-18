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
#include <sbi/sbi_system.h>
#include <sbi/sbi_timer.h>
#include <sbi/sbi_tlb.h>
#include <sbi/sbi_trap.h>

#define SBI_ECALL_VERSION_MAJOR 0
#define SBI_ECALL_VERSION_MINOR 1

u16 sbi_ecall_version_major(void)
{
	return SBI_ECALL_VERSION_MAJOR;
}

u16 sbi_ecall_version_minor(void)
{
	return SBI_ECALL_VERSION_MINOR;
}

int sbi_ecall_handler(u32 hartid, ulong mcause, struct sbi_trap_regs *regs,
		      struct sbi_scratch *scratch)
{
	int ret = SBI_ENOTSUPP;
	struct unpriv_trap uptrap;
	struct sbi_tlb_info tlb_info;

	switch (regs->a7) {
	case SBI_ECALL_SET_TIMER:
#if __riscv_xlen == 32
		sbi_timer_event_start(scratch,
				      (((u64)regs->a1 << 32) | (u64)regs->a0));
#else
		sbi_timer_event_start(scratch, (u64)regs->a0);
#endif
		ret = 0;
		break;
	case SBI_ECALL_CONSOLE_PUTCHAR:
		sbi_putc(regs->a0);
		ret = 0;
		break;
	case SBI_ECALL_CONSOLE_GETCHAR:
		regs->a0 = sbi_getc();
		ret	 = 0;
		break;
	case SBI_ECALL_CLEAR_IPI:
		sbi_ipi_clear_smode(scratch);
		ret = 0;
		break;
	case SBI_ECALL_SEND_IPI:
		ret = sbi_ipi_send_many(scratch, &uptrap, (ulong *)regs->a0,
					SBI_IPI_EVENT_SOFT, NULL);
		break;
	case SBI_ECALL_REMOTE_FENCE_I:
		ret = sbi_ipi_send_many(scratch, &uptrap, (ulong *)regs->a0,
					SBI_IPI_EVENT_FENCE_I, NULL);
		break;
	case SBI_ECALL_REMOTE_SFENCE_VMA:
		tlb_info.start = (unsigned long)regs->a1;
		tlb_info.size  = (unsigned long)regs->a2;
		tlb_info.type  = SBI_TLB_FLUSH_VMA;

		ret = sbi_ipi_send_many(scratch, &uptrap, (ulong *)regs->a0,
					SBI_IPI_EVENT_SFENCE_VMA, &tlb_info);
		break;
	case SBI_ECALL_REMOTE_SFENCE_VMA_ASID:
		tlb_info.start = (unsigned long)regs->a1;
		tlb_info.size  = (unsigned long)regs->a2;
		tlb_info.asid  = (unsigned long)regs->a3;
		tlb_info.type  = SBI_TLB_FLUSH_VMA_ASID;

		ret = sbi_ipi_send_many(scratch, &uptrap, (ulong *)regs->a0,
					SBI_IPI_EVENT_SFENCE_VMA_ASID,
					&tlb_info);
		break;
	case SBI_ECALL_SHUTDOWN:
		sbi_system_shutdown(scratch, 0);
		ret = 0;
		break;
	default:
		regs->a0 = SBI_ENOTSUPP;
		ret	 = 0;
		break;
	};

	if (!ret) {
		regs->mepc += 4;
	} else if (ret == SBI_ETRAP) {
		ret = 0;
		sbi_trap_redirect(regs, scratch, regs->mepc,
				  uptrap.cause, uptrap.tval);
	}

	return ret;
}
