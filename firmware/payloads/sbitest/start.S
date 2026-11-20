/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Entry of the S-mode test payload.
 *
 * OpenSBI jumps here in S-mode with a0 = hart ID and a1 = address of the
 * device tree. Only the boot hart arrives here at first; the other harts
 * stay stopped until the payload starts them through the HSM extension.
 *
 * The first hart that arrives wins the lottery and runs main(). Every other
 * hart that arrives, usually because main() started it, is sent to a wait
 * loop. Before it waits, a secondary hart prints a single '*' and counts
 * itself in secondary_count. This is only a marker for testing.
 *
 * A parked hart has no stack. It waits for the boot hart to write the top of
 * a stack into its slot of secondary_stack[] and to send it an IPI. The hart
 * then switches to that stack and calls secondary_main(). Only the boot hart
 * writes the slots, so assigning the initial stacks needs no locking.
 *
 * The payload is linked at address 0 and can be loaded anywhere. The boot hart
 * applies the relative relocations to the address it was loaded at. The
 * lottery and the code of the secondary harts use only pc-relative addresses
 * and the temporary registers, so they work before and without relocation.
 */

#include <sbi/riscv_elf.h>
#include <sbi/riscv_encoding.h>
#include "sbitest.h"

/*
 * sbi_ecall_interface.h cannot be included here, it has C type definitions.
 * Same values as SBI_EXT_DBCN and SBI_EXT_DBCN_CONSOLE_WRITE_BYTE.
 */
#define SBI_EXT_DBCN				0x4442434E
#define SBI_EXT_DBCN_CONSOLE_WRITE_BYTE		0x2

#if __riscv_xlen == 64
#define REG_L		ld
#define REG_S		sd
#define LOG_REGBYTES	3
#elif __riscv_xlen == 32
#define REG_L		lw
#define REG_S		sw
#define LOG_REGBYTES	2
#else
#error "Unexpected __riscv_xlen"
#endif

/* Atomically add inc to the 32-bit word at addr, rd = old value */
	.macro	amoadd_w rd, inc, addr, tmp
#if defined(__riscv_atomic) || defined(__riscv_zaamo)
	amoadd.w \rd, \inc, (\addr)
#elif defined(__riscv_zalrsc)
9:	lr.w	\rd, (\addr)
	add	\tmp, \rd, \inc
	sc.w	\tmp, \tmp, (\addr)
	bnez	\tmp, 9b
#else
#error "need a or zalrsc"
#endif
	.endm

	.section .entry, "ax", %progbits
	.align 3
	.globl _start
_start:
	/* Pick one hart to run the main boot sequence */
	lla	a3, _hart_lottery
	li	a2, 1
	amoadd_w a4, a2, a3, a5
	bnez	a4, _secondary

	/* Load offset: the link address of _payload_start is 0 */
	lla	t0, _payload_start

	/* Apply the relative relocations */
	lla	t1, __rela_dyn_start
	lla	t2, __rela_dyn_end
	beq	t1, t2, _relocate_done
1:
	REG_L	t3, __SIZEOF_LONG__(t1)		/* r_info */
	li	t4, R_RISCV_RELATIVE
	bne	t3, t4, 2f
	REG_L	t3, 0(t1)			/* r_offset */
	REG_L	t4, (__SIZEOF_LONG__ * 2)(t1)	/* r_addend */
	add	t3, t3, t0
	add	t4, t4, t0
	REG_S	t4, 0(t3)			/* runtime address */
2:
	addi	t1, t1, (__SIZEOF_LONG__ * 3)
	bltu	t1, t2, 1b
_relocate_done:

	/* Zero the BSS */
	lla	t1, _bss_start
	lla	t2, _bss_end
3:
	bgeu	t1, t2, 4f
	REG_S	zero, 0(t1)
	addi	t1, t1, __SIZEOF_POINTER__
	j	3b
4:

	/* Stack behind the payload */
	lla	t1, _payload_end
	li	t2, 0x4000
	add	sp, t1, t2

	call	main

	/* main() does not return. Hang if it does. */
5:
	wfi
	j	5b

	/*
	 * A secondary hart: print '*' (42), count it, and wait for a stack.
	 * a0 is the hart ID. s1 keeps it over the SBI call, which only changes
	 * a0 and a1. There is no stack, only registers are used.
	 */
_secondary:
	mv	s1, a0
	li	a7, SBI_EXT_DBCN
	li	a6, SBI_EXT_DBCN_CONSOLE_WRITE_BYTE
	li	a0, 42
	ecall

	lla	a3, secondary_count
	li	a2, 1
	amoadd_w a4, a2, a3, a5

	/*
	 * Let a supervisor software interrupt wake up wfi. sstatus.SIE stays
	 * clear, so the hart does not take a trap: there is no vector yet.
	 */
	li	t0, MIP_SSIP
	csrw	sie, t0

	/* A hart ID without a slot never gets a stack */
	li	t0, MAX_HARTS
	bgeu	s1, t0, _secondary_hang

	lla	s2, secondary_stack
	slli	t0, s1, LOG_REGBYTES
	add	s2, s2, t0
_secondary_wait:
	REG_L	t1, 0(s2)
	bnez	t1, _secondary_go
	/*
	 * wfi returns when an interrupt is pending, also if it was pending
	 * before. The slot is checked again before and after every wait.
	 */
	wfi
	j	_secondary_wait

_secondary_go:
	/* Acquire: the stack memory is valid, the boot hart released it */
	fence	r, rw
	mv	sp, t1
	mv	a0, s1
	call	secondary_main

_secondary_hang:
	wfi
	j	_secondary_hang

	.section .data
	.align 3
_hart_lottery:
	.word	0
	.globl secondary_count
secondary_count:
	.word	0
