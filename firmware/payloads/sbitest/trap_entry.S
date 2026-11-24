/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Trap entry of the S-mode test payload: save the registers on the stack,
 * call trap_handler() in trap.c, restore the registers and return.
 *
 * The frame is pushed on the stack that is in use when the trap occurs. The
 * hardware clears sstatus.SIE on a trap, so the handler runs with interrupts
 * disabled and is not interrupted by another interrupt.
 */

#include "trap.h"

#if __riscv_xlen == 64
#define REG_L		ld
#define REG_S		sd
#elif __riscv_xlen == 32
#define REG_L		lw
#define REG_S		sw
#else
#error "Unexpected __riscv_xlen"
#endif

	.macro	SAVE reg, slot
	REG_S	\reg, (\slot * __SIZEOF_LONG__)(sp)
	.endm

	.macro	LOAD reg, slot
	REG_L	\reg, (\slot * __SIZEOF_LONG__)(sp)
	.endm

	.section .text
	/* The base address of stvec in direct mode must be 4 byte aligned */
	.align 2
	.globl trap_entry
trap_entry:
	addi	sp, sp, -TRAP_FRAME_SIZE

	SAVE	ra, TRAP_RA
	SAVE	gp, TRAP_GP
	SAVE	tp, TRAP_TP
	SAVE	t0, TRAP_T0
	SAVE	t1, TRAP_T1
	SAVE	t2, TRAP_T2
	SAVE	s0, TRAP_S0
	SAVE	s1, TRAP_S1
	SAVE	a0, TRAP_A0
	SAVE	a1, TRAP_A1
	SAVE	a2, TRAP_A2
	SAVE	a3, TRAP_A3
	SAVE	a4, TRAP_A4
	SAVE	a5, TRAP_A5
	SAVE	a6, TRAP_A6
	SAVE	a7, TRAP_A7
	SAVE	s2, TRAP_S2
	SAVE	s3, TRAP_S3
	SAVE	s4, TRAP_S4
	SAVE	s5, TRAP_S5
	SAVE	s6, TRAP_S6
	SAVE	s7, TRAP_S7
	SAVE	s8, TRAP_S8
	SAVE	s9, TRAP_S9
	SAVE	s10, TRAP_S10
	SAVE	s11, TRAP_S11
	SAVE	t3, TRAP_T3
	SAVE	t4, TRAP_T4
	SAVE	t5, TRAP_T5
	SAVE	t6, TRAP_T6

	/* The stack pointer of the interrupted code, t0 is saved already */
	addi	t0, sp, TRAP_FRAME_SIZE
	SAVE	t0, TRAP_SP

	csrr	t0, sepc
	SAVE	t0, TRAP_SEPC
	csrr	t0, sstatus
	SAVE	t0, TRAP_SSTATUS
	csrr	t0, scause
	SAVE	t0, TRAP_SCAUSE
	csrr	t0, stval
	SAVE	t0, TRAP_STVAL

	/* trap_handler(struct trap_regs *regs) */
	mv	a0, sp
	call	trap_handler

	/* The handler may have changed the return address */
	LOAD	t0, TRAP_SEPC
	csrw	sepc, t0

	LOAD	ra, TRAP_RA
	LOAD	gp, TRAP_GP
	LOAD	tp, TRAP_TP
	LOAD	t0, TRAP_T0
	LOAD	t1, TRAP_T1
	LOAD	t2, TRAP_T2
	LOAD	s0, TRAP_S0
	LOAD	s1, TRAP_S1
	LOAD	a0, TRAP_A0
	LOAD	a1, TRAP_A1
	LOAD	a2, TRAP_A2
	LOAD	a3, TRAP_A3
	LOAD	a4, TRAP_A4
	LOAD	a5, TRAP_A5
	LOAD	a6, TRAP_A6
	LOAD	a7, TRAP_A7
	LOAD	s2, TRAP_S2
	LOAD	s3, TRAP_S3
	LOAD	s4, TRAP_S4
	LOAD	s5, TRAP_S5
	LOAD	s6, TRAP_S6
	LOAD	s7, TRAP_S7
	LOAD	s8, TRAP_S8
	LOAD	s9, TRAP_S9
	LOAD	s10, TRAP_S10
	LOAD	s11, TRAP_S11
	LOAD	t3, TRAP_T3
	LOAD	t4, TRAP_T4
	LOAD	t5, TRAP_T5
	LOAD	t6, TRAP_T6

	/* Last: sp itself, it pops the frame */
	LOAD	sp, TRAP_SP

	sret
