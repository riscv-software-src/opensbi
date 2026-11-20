/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * C trap handler of the S-mode test payload.
 *
 * The handler prints with sbi_printf() in a few error cases. If the trap
 * interrupted code that holds the console lock, for example in sbi_printf()
 * itself, the handler would wait for the lock forever. This is acceptable for
 * errors that end the test run anyway, but do not print in the handler of a
 * normal event.
 */

#include <sbi/riscv_asm.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_types.h>
#include "sbitest.h"
#include "trap.h"

/* The C structure must match the offsets used by trap_entry.S */
_Static_assert(sizeof(struct trap_regs) == TRAP_NREGS * sizeof(unsigned long),
	       "struct trap_regs size");
_Static_assert(offsetof(struct trap_regs, ra) ==
		       TRAP_RA * sizeof(unsigned long),
	       "ra");
_Static_assert(offsetof(struct trap_regs, sp) ==
		       TRAP_SP * sizeof(unsigned long),
	       "sp");
_Static_assert(offsetof(struct trap_regs, a0) ==
		       TRAP_A0 * sizeof(unsigned long),
	       "a0");
_Static_assert(offsetof(struct trap_regs, s11) ==
		       TRAP_S11 * sizeof(unsigned long),
	       "s11");
_Static_assert(offsetof(struct trap_regs, t6) ==
		       TRAP_T6 * sizeof(unsigned long),
	       "t6");
_Static_assert(offsetof(struct trap_regs, sepc) ==
		       TRAP_SEPC * sizeof(unsigned long),
	       "sepc");
_Static_assert(offsetof(struct trap_regs, sstatus) ==
		       TRAP_SSTATUS * sizeof(unsigned long),
	       "sstatus");
_Static_assert(offsetof(struct trap_regs, scause) ==
		       TRAP_SCAUSE * sizeof(unsigned long),
	       "scause");
_Static_assert(offsetof(struct trap_regs, stval) ==
		       TRAP_STVAL * sizeof(unsigned long),
	       "stval");

trap_hook_t trap_exception_hook;

static unsigned long ssip_count;

unsigned long trap_ssip_count(void)
{
	return __atomic_load_n(&ssip_count, __ATOMIC_ACQUIRE);
}

void trap_skip_insn(struct trap_regs *regs)
{
	unsigned short insn = *(unsigned short *)regs->sepc;

	/* The two lowest bits are 11 for instructions that are 32 bits long */
	regs->sepc += ((insn & 3) == 3) ? 4 : 2;
}

static void trap_fail(const char *what, struct trap_regs *regs)
{
	sbi_printf("\nscause=0x%lx sepc=0x%lx stval=0x%lx\n", regs->scause,
		   regs->sepc, regs->stval);

	test_abort(what);
}

static void handle_interrupt(struct trap_regs *regs)
{
	unsigned long irq = regs->scause & ~SCAUSE_INTERRUPT;

	switch (irq) {
	case IRQ_S_SOFT:
		/*
		 * An IPI from another hart, or from this one, through the SBI
		 * IPI extension. OpenSBI raised the pending bit, clear it or
		 * the trap would return and trigger again.
		 */
		csr_clear(CSR_SIP, MIP_SSIP);
		__atomic_fetch_add(&ssip_count, 1, __ATOMIC_RELEASE);
		break;
	default:
		/* Not enabled by trap_init(). Do not let it fire again. */
		csr_clear(CSR_SIE, 1UL << irq);
		trap_fail("unexpected interrupt", regs);
	}
}

void trap_handler(struct trap_regs *regs)
{
	if (regs->scause & SCAUSE_INTERRUPT) {
		handle_interrupt(regs);
		return;
	}

	if (trap_exception_hook && trap_exception_hook(regs))
		return;

	trap_fail("unexpected exception", regs);
}

void trap_init(void)
{
	csr_write(CSR_STVEC, (unsigned long)trap_entry);

	/*
	 * Delegation of interrupts and exceptions to S-mode is done by
	 * OpenSBI in M-mode (sbi_trap_init()), S-mode cannot write mideleg and
	 * medeleg. It delegates SSIP, STIP, SEIP, the breakpoint exception
	 * and others. Here the supervisor software interrupt is enabled.
	 */
	csr_write(CSR_SIE, MIP_SSIP);
	csr_set(CSR_SSTATUS, SSTATUS_SIE);
}
