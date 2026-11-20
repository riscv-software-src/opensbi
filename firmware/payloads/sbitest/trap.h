/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Trap handling of the S-mode test payload.
 */

#ifndef __SBITEST_TRAP_H__
#define __SBITEST_TRAP_H__

/*
 * Layout of the trap frame that trap_entry pushes on the stack. The numbers
 * are slots of one register (unsigned long). Keep struct trap_regs in sync,
 * trap.c checks it at compile time.
 */
#define TRAP_RA 0
#define TRAP_SP 1
#define TRAP_GP 2
#define TRAP_TP 3
#define TRAP_T0 4
#define TRAP_T1 5
#define TRAP_T2 6
#define TRAP_S0 7
#define TRAP_S1 8
#define TRAP_A0 9
#define TRAP_A1 10
#define TRAP_A2 11
#define TRAP_A3 12
#define TRAP_A4 13
#define TRAP_A5 14
#define TRAP_A6 15
#define TRAP_A7 16
#define TRAP_S2 17
#define TRAP_S3 18
#define TRAP_S4 19
#define TRAP_S5 20
#define TRAP_S6 21
#define TRAP_S7 22
#define TRAP_S8 23
#define TRAP_S9 24
#define TRAP_S10 25
#define TRAP_S11 26
#define TRAP_T3 27
#define TRAP_T4 28
#define TRAP_T5 29
#define TRAP_T6 30
#define TRAP_SEPC 31
#define TRAP_SSTATUS 32
#define TRAP_SCAUSE 33
#define TRAP_STVAL 34
#define TRAP_NREGS 35

/* The stack pointer must stay 16 byte aligned */
#define TRAP_FRAME_SIZE (((TRAP_NREGS * __SIZEOF_LONG__) + 15) & ~15)

#ifndef __ASSEMBLER__

/* Interrupt flag of scause */
#define SCAUSE_INTERRUPT (1UL << (__riscv_xlen - 1))

struct trap_regs {
	unsigned long ra;
	unsigned long sp;
	unsigned long gp;
	unsigned long tp;
	unsigned long t0;
	unsigned long t1;
	unsigned long t2;
	unsigned long s0;
	unsigned long s1;
	unsigned long a0;
	unsigned long a1;
	unsigned long a2;
	unsigned long a3;
	unsigned long a4;
	unsigned long a5;
	unsigned long a6;
	unsigned long a7;
	unsigned long s2;
	unsigned long s3;
	unsigned long s4;
	unsigned long s5;
	unsigned long s6;
	unsigned long s7;
	unsigned long s8;
	unsigned long s9;
	unsigned long s10;
	unsigned long s11;
	unsigned long t3;
	unsigned long t4;
	unsigned long t5;
	unsigned long t6;
	/* Written back to sepc when the trap returns */
	unsigned long sepc;
	/* Informational, not written back */
	unsigned long sstatus;
	unsigned long scause;
	unsigned long stval;
};

/* Assembly entry point, see trap_entry.S */
void trap_entry(void);

/*
 * An exception hook returns non-zero if it handled the exception and the
 * hart can continue at regs->sepc. Without a hook, or if the hook returns
 * zero, an exception is reported and the test payload powers off.
 */
typedef int (*trap_hook_t)(struct trap_regs *regs);
extern trap_hook_t trap_exception_hook;

/*
 * Install trap_entry as trap vector of the calling hart, enable the
 * supervisor software interrupt and enable interrupts in S-mode. The
 * interrupts and exceptions have to be delegated to S-mode by OpenSBI, the
 * payload cannot do this itself.
 *
 * The frame is pushed on the current stack. Call it only on a hart that has
 * a valid stack.
 */
void trap_init(void);

/* Number of supervisor software interrupts (IPIs) handled so far */
unsigned long trap_ssip_count(void);

/* Let the trap return behind the instruction that caused the trap */
void trap_skip_insn(struct trap_regs *regs);

#endif /* __ASSEMBLER__ */

#endif /* __SBITEST_TRAP_H__ */
