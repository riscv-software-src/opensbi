/*
 * Copyright (c) 2018 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_encoding.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_ecall.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_illegal_insn.h>
#include <sbi/sbi_ipi.h>
#include <sbi/sbi_misaligned_ldst.h>
#include <sbi/sbi_timer.h>
#include <sbi/sbi_trap.h>

static void __attribute__((noreturn)) sbi_trap_error(const char *msg,
						int rc, u32 hartid,
						ulong mcause, ulong mtval,
						struct sbi_trap_regs *regs)
{
	sbi_printf("%s: hart%d: %s (error %d)\n",
		   __func__, hartid, msg, rc);
	sbi_printf("%s: hart%d: mcause=0x%lx mtval=0x%lx\n",
		   __func__, hartid, mcause, mtval);
	sbi_printf("%s: hart%d: mepc=0x%lx mstatus=0x%lx\n",
		   __func__, hartid, regs->mepc, regs->mstatus);
	sbi_printf("%s: hart%d: %s=0x%lx %s=0x%lx\n",
		   __func__, hartid, "ra", regs->ra, "sp", regs->sp);
	sbi_printf("%s: hart%d: %s=0x%lx %s=0x%lx\n",
		   __func__, hartid, "gp", regs->gp, "tp", regs->tp);
	sbi_printf("%s: hart%d: %s=0x%lx %s=0x%lx\n",
		   __func__, hartid, "s0", regs->s0, "s1", regs->s1);
	sbi_printf("%s: hart%d: %s=0x%lx %s=0x%lx\n",
		   __func__, hartid, "a0", regs->a0, "a1", regs->a1);
	sbi_printf("%s: hart%d: %s=0x%lx %s=0x%lx\n",
		   __func__, hartid, "a2", regs->a2, "a3", regs->a3);
	sbi_printf("%s: hart%d: %s=0x%lx %s=0x%lx\n",
		   __func__, hartid, "a4", regs->a4, "a5", regs->a5);
	sbi_printf("%s: hart%d: %s=0x%lx %s=0x%lx\n",
		   __func__, hartid, "a6", regs->a6, "a7", regs->a7);
	sbi_printf("%s: hart%d: %s=0x%lx %s=0x%lx\n",
		   __func__, hartid, "s2", regs->s2, "s3", regs->s3);
	sbi_printf("%s: hart%d: %s=0x%lx %s=0x%lx\n",
		   __func__, hartid, "s4", regs->s4, "s5", regs->s5);
	sbi_printf("%s: hart%d: %s=0x%lx %s=0x%lx\n",
		   __func__, hartid, "s6", regs->s6, "s7", regs->s7);
	sbi_printf("%s: hart%d: %s=0x%lx %s=0x%lx\n",
		   __func__, hartid, "s8", regs->s8, "s9", regs->s9);
	sbi_printf("%s: hart%d: %s=0x%lx %s=0x%lx\n",
		   __func__, hartid, "s10", regs->s10, "s11", regs->s11);
	sbi_printf("%s: hart%d: %s=0x%lx %s=0x%lx\n",
		   __func__, hartid, "t0", regs->t0, "t1", regs->t1);
	sbi_printf("%s: hart%d: %s=0x%lx %s=0x%lx\n",
		   __func__, hartid, "t2", regs->t2, "t3", regs->t3);
	sbi_printf("%s: hart%d: %s=0x%lx %s=0x%lx\n",
		   __func__, hartid, "t4", regs->t4, "t5", regs->t5);
	sbi_printf("%s: hart%d: %s=0x%lx\n",
		   __func__, hartid, "t6", regs->t6);

	sbi_hart_hang();
}

void sbi_trap_handler(struct sbi_trap_regs *regs,
		      struct sbi_scratch *scratch)
{
	int rc = SBI_ENOTSUPP;
	const char *msg = "trap handler failed";
	u32 hartid = sbi_current_hartid();
	ulong mcause = csr_read(mcause);

	if (mcause & (1UL << (__riscv_xlen - 1))) {
		mcause &= ~(1UL << (__riscv_xlen - 1));
		switch (mcause) {
		case IRQ_M_TIMER:
			sbi_timer_process(scratch, hartid);
			break;
		case IRQ_M_SOFT:
			sbi_ipi_process(scratch, hartid);
			break;
		default:
			msg = "unhandled external interrupt";
			goto trap_error;
		};
		return;
	}

	switch (mcause) {
	case CAUSE_ILLEGAL_INSTRUCTION:
		rc = sbi_illegal_insn_handler(hartid, mcause, regs, scratch);
		msg = "illegal instruction handler failed";
		break;
	case CAUSE_MISALIGNED_LOAD:
		rc = sbi_misaligned_load_handler(hartid, mcause, regs, scratch);
		msg = "misaligned load handler failed";
		break;
	case CAUSE_MISALIGNED_STORE:
		rc = sbi_misaligned_store_handler(hartid, mcause, regs, scratch);
		msg = "misaligned store handler failed";
		break;
	case CAUSE_SUPERVISOR_ECALL:
	case CAUSE_HYPERVISOR_ECALL:
		rc = sbi_ecall_handler(hartid, mcause, regs, scratch);
		msg = "ecall handler failed";
		break;
	default:
		break;
	};

trap_error:
	if (rc) {
		sbi_trap_error(msg, rc, hartid, mcause, csr_read(mtval), regs);
	}
}
