/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <sbi/riscv_encoding.h>
#define __ASM_STR(x)	x

#if __riscv_xlen == 64
#define __REG_SEL(a, b)		__ASM_STR(a)
#define RISCV_PTR		.dword
#elif __riscv_xlen == 32
#define __REG_SEL(a, b)		__ASM_STR(b)
#define RISCV_PTR		.word
#else
#error "Unexpected __riscv_xlen"
#endif

#define REG_L		__REG_SEL(ld, lw)
#define REG_S		__REG_SEL(sd, sw)

	.section .entry, "ax", %progbits
	.align 3
	.globl _start
_start:
	/* Pick one hart to run the main boot sequence */
	lla	a3, _hart_lottery
	li	a2, 1
#ifdef __riscv_atomic
	amoadd.w a3, a2, (a3)
#elif __riscv_zalrsc
_sc_fail:
	lr.w	t0, (a3)
	addw	t1, t0, a2
	sc.w	t1, t1, (a3)
	bnez	t1, _sc_fail
	move	a3, t0
#else
#error "need a or zalrsc"
#endif
	bnez	a3, _start_hang

	/* Save a0 and a1 */
	lla	a3, _boot_a0
	REG_S	a0, 0(a3)
	lla	a3, _boot_a1
	REG_S	a1, 0(a3)

	/* Zero-out BSS */
	lla	a4, _bss_start
	lla	a5, _bss_end
_bss_zero:
	REG_S	zero, (a4)
	add	a4, a4, __SIZEOF_POINTER__
	blt	a4, a5, _bss_zero

_start_warm:
	/* Disable and clear all interrupts */
	csrw	CSR_SIE, zero
	csrw	CSR_SIP, zero

	/* Setup exception vectors */
	lla	a3, _start_hang
	csrw	CSR_STVEC, a3

	/* Setup stack */
	lla	a3, _payload_end
	li	a4, 0x2000
	add	sp, a3, a4

	/* Jump to C main */
	lla	a3, _boot_a0
	REG_L	a0, 0(a3)
	lla	a3, _boot_a1
	REG_L	a1, 0(a3)
	call	test_main

	/* We don't expect to reach here hence just hang */
	j	_start_hang

	.section .entry, "ax", %progbits
	.align 3
	.globl _start_hang
_start_hang:
	wfi
	j	_start_hang

	.section .data
	.align	3
_hart_lottery:
	RISCV_PTR	0
_boot_a0:
	RISCV_PTR	0
_boot_a1:
	RISCV_PTR	0

	/* This will be called when the stack corruption is detected */
	.section .text
	.align 3
	.globl __stack_chk_fail
	.type __stack_chk_fail, %function
	.equ  __stack_chk_fail, _start_hang

	/* Initial value of the stack guard variable */
	.section .data
	.align 3
	.globl __stack_chk_guard
	.type __stack_chk_guard, %object
__stack_chk_guard:
	RISCV_PTR	0x95B5FF5A
