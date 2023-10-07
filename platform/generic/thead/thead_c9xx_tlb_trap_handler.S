/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Authors:
 *   Inochi Amaoto <inochiama@outlook.com>
 *
 */
	.section .entry, "ax", %progbits
	.align 3
	.globl _thead_tlb_flush_fixup_trap_handler
_thead_tlb_flush_fixup_trap_handler:
	sfence.vma t0, zero
	j _trap_handler
