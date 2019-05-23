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
#include <sbi/riscv_unpriv.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_ecall.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_illegal_insn.h>
#include <sbi/sbi_ipi.h>
#include <sbi/sbi_misaligned_ldst.h>
#include <sbi/sbi_timer.h>
#include <sbi/sbi_trap.h>

static void __noreturn sbi_trap_error(const char *msg, int rc, u32 hartid,
				      ulong mcause, ulong mtval,
				      struct sbi_trap_regs *regs)
{
	sbi_printf("%s: hart%d: %s (error %d)\n", __func__, hartid, msg, rc);
	sbi_printf("%s: hart%d: mcause=0x%" PRILX " mtval=0x%" PRILX "\n",
		   __func__, hartid, mcause, mtval);
	sbi_printf("%s: hart%d: mepc=0x%" PRILX " mstatus=0x%" PRILX "\n",
		   __func__, hartid, regs->mepc, regs->mstatus);
	sbi_printf("%s: hart%d: %s=0x%" PRILX " %s=0x%" PRILX "\n", __func__,
		   hartid, "ra", regs->ra, "sp", regs->sp);
	sbi_printf("%s: hart%d: %s=0x%" PRILX " %s=0x%" PRILX "\n", __func__,
		   hartid, "gp", regs->gp, "tp", regs->tp);
	sbi_printf("%s: hart%d: %s=0x%" PRILX " %s=0x%" PRILX "\n", __func__,
		   hartid, "s0", regs->s0, "s1", regs->s1);
	sbi_printf("%s: hart%d: %s=0x%" PRILX " %s=0x%" PRILX "\n", __func__,
		   hartid, "a0", regs->a0, "a1", regs->a1);
	sbi_printf("%s: hart%d: %s=0x%" PRILX " %s=0x%" PRILX "\n", __func__,
		   hartid, "a2", regs->a2, "a3", regs->a3);
	sbi_printf("%s: hart%d: %s=0x%" PRILX " %s=0x%" PRILX "\n", __func__,
		   hartid, "a4", regs->a4, "a5", regs->a5);
	sbi_printf("%s: hart%d: %s=0x%" PRILX " %s=0x%" PRILX "\n", __func__,
		   hartid, "a6", regs->a6, "a7", regs->a7);
	sbi_printf("%s: hart%d: %s=0x%" PRILX " %s=0x%" PRILX "\n", __func__,
		   hartid, "s2", regs->s2, "s3", regs->s3);
	sbi_printf("%s: hart%d: %s=0x%" PRILX " %s=0x%" PRILX "\n", __func__,
		   hartid, "s4", regs->s4, "s5", regs->s5);
	sbi_printf("%s: hart%d: %s=0x%" PRILX " %s=0x%" PRILX "\n", __func__,
		   hartid, "s6", regs->s6, "s7", regs->s7);
	sbi_printf("%s: hart%d: %s=0x%" PRILX " %s=0x%" PRILX "\n", __func__,
		   hartid, "s8", regs->s8, "s9", regs->s9);
	sbi_printf("%s: hart%d: %s=0x%" PRILX " %s=0x%" PRILX "\n", __func__,
		   hartid, "s10", regs->s10, "s11", regs->s11);
	sbi_printf("%s: hart%d: %s=0x%" PRILX " %s=0x%" PRILX "\n", __func__,
		   hartid, "t0", regs->t0, "t1", regs->t1);
	sbi_printf("%s: hart%d: %s=0x%" PRILX " %s=0x%" PRILX "\n", __func__,
		   hartid, "t2", regs->t2, "t3", regs->t3);
	sbi_printf("%s: hart%d: %s=0x%" PRILX " %s=0x%" PRILX "\n", __func__,
		   hartid, "t4", regs->t4, "t5", regs->t5);
	sbi_printf("%s: hart%d: %s=0x%" PRILX "\n", __func__, hartid, "t6",
		   regs->t6);

	sbi_hart_hang();
}

/**
 * Redirect trap to lower privledge mode (S-mode or U-mode)
 *
 * @param regs pointer to register state
 * @param scratch pointer to sbi_scratch of current HART
 * @param epc error PC for lower privledge mode
 * @param cause exception cause for lower privledge mode
 * @param tval trap value for lower privledge mode
 *
 * @return 0 on success and negative error code on failure
 */
int sbi_trap_redirect(struct sbi_trap_regs *regs, struct sbi_scratch *scratch,
		      ulong epc, ulong cause, ulong tval)
{
	ulong new_mstatus, prev_mode;

	/* Sanity check on previous mode */
	prev_mode = (regs->mstatus & MSTATUS_MPP) >> MSTATUS_MPP_SHIFT;
	if (prev_mode != PRV_S && prev_mode != PRV_U)
		return SBI_ENOTSUPP;

	/* Update S-mode exception info */
	csr_write(CSR_STVAL, tval);
	csr_write(CSR_SEPC, epc);
	csr_write(CSR_SCAUSE, cause);

	/* Set MEPC to S-mode exception vector base */
	regs->mepc = csr_read(CSR_STVEC);

	/* Initial value of new MSTATUS */
	new_mstatus = regs->mstatus;

	/* Clear MPP, SPP, SPIE, and SIE */
	new_mstatus &=
		~(MSTATUS_MPP | MSTATUS_SPP | MSTATUS_SPIE | MSTATUS_SIE);

	/* Set SPP */
	if (prev_mode == PRV_S)
		new_mstatus |= (1UL << MSTATUS_SPP_SHIFT);

	/* Set SPIE */
	if (regs->mstatus & MSTATUS_SIE)
		new_mstatus |= (1UL << MSTATUS_SPIE_SHIFT);

	/* Set MPP */
	new_mstatus |= (PRV_S << MSTATUS_MPP_SHIFT);

	/* Set new value in MSTATUS */
	regs->mstatus = new_mstatus;

	return 0;
}

/**
 * Handle trap/interrupt
 *
 * This function is called by firmware linked to OpenSBI
 * library for handling trap/interrupt. It expects the
 * following:
 * 1. The 'mscratch' CSR is pointing to sbi_scratch of current HART
 * 2. The 'mcause' CSR is having exception/interrupt cause
 * 3. The 'mtval' CSR is having additional trap information
 * 4. Stack pointer (SP) is setup for current HART
 * 5. Interrupts are disabled in MSTATUS CSR
 *
 * @param regs pointer to register state
 * @param scratch pointer to sbi_scratch of current HART
 */
void sbi_trap_handler(struct sbi_trap_regs *regs, struct sbi_scratch *scratch)
{
	int rc = SBI_ENOTSUPP;
	const char *msg = "trap handler failed";
	u32 hartid = sbi_current_hartid();
	ulong mcause = csr_read(CSR_MCAUSE);
	ulong mtval = csr_read(CSR_MTVAL);
	struct unpriv_trap *uptrap;

	if (mcause & (1UL << (__riscv_xlen - 1))) {
		mcause &= ~(1UL << (__riscv_xlen - 1));
		switch (mcause) {
		case IRQ_M_TIMER:
			sbi_timer_process(scratch);
			break;
		case IRQ_M_SOFT:
			sbi_ipi_process(scratch);
			break;
		default:
			msg = "unhandled external interrupt";
			goto trap_error;
		};
		return;
	}

	switch (mcause) {
	case CAUSE_ILLEGAL_INSTRUCTION:
		rc  = sbi_illegal_insn_handler(hartid, mcause, regs, scratch);
		msg = "illegal instruction handler failed";
		break;
	case CAUSE_MISALIGNED_LOAD:
		rc = sbi_misaligned_load_handler(hartid, mcause, regs, scratch);
		msg = "misaligned load handler failed";
		break;
	case CAUSE_MISALIGNED_STORE:
		rc  = sbi_misaligned_store_handler(hartid, mcause, regs,
						   scratch);
		msg = "misaligned store handler failed";
		break;
	case CAUSE_SUPERVISOR_ECALL:
	case CAUSE_HYPERVISOR_ECALL:
		rc  = sbi_ecall_handler(hartid, mcause, regs, scratch);
		msg = "ecall handler failed";
		break;
	case CAUSE_LOAD_ACCESS:
	case CAUSE_STORE_ACCESS:
	case CAUSE_LOAD_PAGE_FAULT:
	case CAUSE_STORE_PAGE_FAULT:
		uptrap = sbi_hart_get_trap_info(scratch);
		if ((regs->mstatus & MSTATUS_MPRV) && uptrap) {
			rc = 0;
			regs->mepc += uptrap->ilen;
			uptrap->cause = mcause;
			uptrap->tval = mtval;
		} else {
			rc = sbi_trap_redirect(regs, scratch, regs->mepc,
					       mcause, mtval);
		}
		msg = "page/access fault handler failed";
		break;
	default:
		/* If the trap came from S or U mode, redirect it there */
		rc = sbi_trap_redirect(regs, scratch, regs->mepc, mcause, mtval);
		break;
	};

trap_error:
	if (rc) {
		sbi_trap_error(msg, rc, hartid, mcause, csr_read(CSR_MTVAL),
			       regs);
	}
}
