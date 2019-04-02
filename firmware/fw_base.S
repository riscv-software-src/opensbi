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
#include <sbi/sbi_platform.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_trap.h>

	.align 3
	.section .entry, "ax", %progbits
	.globl _start
	.globl _start_warm
_start:
	/*
	 * Jump to warm-boot if this is not the first core booting,
	 * that is, for mhartid != 0
	 */
	csrr	a6, CSR_MHARTID
	blt	zero, a6, _wait_for_boot_hart

	li	ra, 0
	call	_reset_regs

	/* Preload HART details
	 * s7 -> HART Count
	 * s8 -> HART Stack Size
	 */
	la	a4, platform
#if __riscv_xlen == 64
	lwu	s7, SBI_PLATFORM_HART_COUNT_OFFSET(a4)
	lwu	s8, SBI_PLATFORM_HART_STACK_SIZE_OFFSET(a4)
#else
	lw	s7, SBI_PLATFORM_HART_COUNT_OFFSET(a4)
	lw	s8, SBI_PLATFORM_HART_STACK_SIZE_OFFSET(a4)
#endif

	/* Setup scratch space for all the HARTs*/
	la	tp, _fw_end
	mul	a5, s7, s8
	add	tp, tp, a5
	/* Keep a copy of tp */
	add	t3, tp, zero
	/* Counter */
	li	t2, 1
	/* hartid 0 is mandated by ISA */
	li	t1, 0
_scratch_init:
	add	tp, t3, zero
	mul	a5, s8, t1
	sub	tp, tp, a5
	li	a5, SBI_SCRATCH_SIZE
	sub	tp, tp, a5

	/* Initialize scratch space */
	la	a4, _fw_start
	la	a5, _fw_end
	mul	t0, s7, s8
	add	a5, a5, t0
	sub	a5, a5, a4
	REG_S	a4, SBI_SCRATCH_FW_START_OFFSET(tp)
	REG_S	a5, SBI_SCRATCH_FW_SIZE_OFFSET(tp)
	/* Note: fw_next_arg1() uses a0, a1, and ra */
	call	fw_next_arg1
	REG_S	a0, SBI_SCRATCH_NEXT_ARG1_OFFSET(tp)
	/* Note: fw_next_addr() uses a0, a1, and ra */
	call	fw_next_addr
	REG_S	a0, SBI_SCRATCH_NEXT_ADDR_OFFSET(tp)
	li	a4, PRV_S
	REG_S	a4, SBI_SCRATCH_NEXT_MODE_OFFSET(tp)
	la	a4, _start_warm
	REG_S	a4, SBI_SCRATCH_WARMBOOT_ADDR_OFFSET(tp)
	la	a4, platform
	REG_S	a4, SBI_SCRATCH_PLATFORM_ADDR_OFFSET(tp)
	la	a4, _hartid_to_scratch
	REG_S	a4, SBI_SCRATCH_HARTID_TO_SCRATCH_OFFSET(tp)
	REG_S	zero, SBI_SCRATCH_TMP0_OFFSET(tp)
#ifdef FW_OPTIONS
	li	a4, FW_OPTIONS
	REG_S	a4, SBI_SCRATCH_OPTIONS_OFFSET(tp)
#else
	REG_S	zero, SBI_SCRATCH_OPTIONS_OFFSET(tp)
#endif
	add	t1, t1, t2
	blt	t1, s7, _scratch_init

	/* Zero-out BSS */
	la	a4, _bss_start
	la	a5, _bss_end
_bss_zero:
	REG_S	zero, (a4)
	add	a4, a4, __SIZEOF_POINTER__
	blt	a4, a5, _bss_zero

	/* Override pervious arg1 */
	add	s0, a0, zero
	add	s1, a1, zero
	call	fw_prev_arg1
	add	t1, a0, zero
	add	a0, s0, zero
	add	a1, s1, zero
	beqz	t1, _prev_arg1_override_done
	add	a1, t1, zero
_prev_arg1_override_done:

	/*
	 * Relocate Flatened Device Tree (FDT)
	 * source FDT address = previous arg1
	 * destination FDT address = next arg1
	 *
	 * Note: We will preserve a0 and a1 passed by
	 * previous booting stage.
	 */
	beqz	a1, _fdt_reloc_done
	/* Mask values in a3 and a4 */
	li	a3, ~(__SIZEOF_POINTER__ - 1)
	li	a4, 0xff
	/* t1 = destination FDT start address */
	add	s0, a0, zero
	add	s1, a1, zero
	call	fw_next_arg1
	add	t1, a0, zero
	add	a0, s0, zero
	add	a1, s1, zero
	beqz	t1, _fdt_reloc_done
	and	t1, t1, a3
	/* t0 = source FDT start address */
	add	t0, a1, zero
	and	t0, t0, a3
	/* t2 = source FDT size in big-endian */
#if __riscv_xlen == 64
	lwu	t2, 4(t0)
#else
	lw	t2, 4(t0)
#endif
	/* t3 = bit[15:8] of FDT size */
	add	t3, t2, zero
	srli	t3, t3, 16
	and	t3, t3, a4
	slli	t3, t3, 8
	/* t4 = bit[23:16] of FDT size */
	add	t4, t2, zero
	srli	t4, t4, 8
	and	t4, t4, a4
	slli	t4, t4, 16
	/* t5 = bit[31:24] of FDT size */
	add	t5, t2, zero
	and	t5, t5, a4
	slli	t5, t5, 24
	/* t2 = bit[7:0] of FDT size */
	srli	t2, t2, 24
	and	t2, t2, a4
	/* t2 = FDT size in little-endian */
	or	t2, t2, t3
	or	t2, t2, t4
	or	t2, t2, t5
	/* t2 = destination FDT end address */
	add	t2, t1, t2
	/* FDT copy loop */
	ble	t2, t1, _fdt_reloc_done
_fdt_reloc_again:
	REG_L	t3, 0(t0)
	REG_S	t3, 0(t1)
	add	t0, t0, __SIZEOF_POINTER__
	add	t1, t1, __SIZEOF_POINTER__
	blt	t1, t2, _fdt_reloc_again
_fdt_reloc_done:

	/* Update boot hart flag */
	la	a4, _boot_hart_done
	li	a5, 1
	REG_S	a5, (a4)

	/* Wait for boot hart */
_wait_for_boot_hart:
	la	a4, _boot_hart_done
	REG_L	a5, (a4)
	/* Reduce the bus traffic so that boot hart may proceed faster */
	nop
	nop
	nop
	beqz	a5, _wait_for_boot_hart

_start_warm:
	li	ra, 0
	call	_reset_regs

	/* Disable and clear all interrupts */
	csrw	CSR_MIE, zero
	csrw	CSR_MIP, zero

	la	a4, platform
#if __riscv_xlen == 64
	lwu	s7, SBI_PLATFORM_HART_COUNT_OFFSET(a4)
	lwu	s8, SBI_PLATFORM_HART_STACK_SIZE_OFFSET(a4)
#else
	lw	s7, SBI_PLATFORM_HART_COUNT_OFFSET(a4)
	lw	s8, SBI_PLATFORM_HART_STACK_SIZE_OFFSET(a4)
#endif

	/* HART ID should be within expected limit */
	csrr	s6, CSR_MHARTID
	bge	s6, s7, _start_hang

	/* find the scratch space for this hart */
	la	tp, _fw_end
	mul	a5, s7, s8
	add	tp, tp, a5
	mul	a5, s8, s6
	sub	tp, tp, a5
	li	a5, SBI_SCRATCH_SIZE
	sub	tp, tp, a5

	/* update the mscratch */
	csrw	CSR_MSCRATCH, tp

	/* Setup stack */
	add	sp, tp, zero

	/* Setup trap handler */
	la	a4, _trap_handler
	csrw	CSR_MTVEC, a4
	/* Make sure that mtvec is updated */
	1:
	csrr	a5, CSR_MTVEC
	bne	a4, a5, 1b


	/* Initialize SBI runtime */
	csrr	a0, CSR_MSCRATCH
	call	sbi_init

	/* We don't expect to reach here hence just hang */
	j	_start_hang

	.align 3
	.section .data, "aw"
_boot_hart_done:
	RISCV_PTR	0

	.align 3
	.section .entry, "ax", %progbits
	.globl _hartid_to_scratch
_hartid_to_scratch:
	add	sp, sp, -(3 * __SIZEOF_POINTER__)
	REG_S	s0, (sp)
	REG_S	s1, (__SIZEOF_POINTER__)(sp)
	REG_S	s2, (__SIZEOF_POINTER__ * 2)(sp)
	/*
	 * a0 -> HART ID (passed by caller)
	 * s0 -> HART Stack Size
	 * s1 -> HART Stack End
	 * s2 -> Temporary
	 */
	la	s2, platform
#if __riscv_xlen == 64
	lwu	s0, SBI_PLATFORM_HART_STACK_SIZE_OFFSET(s2)
	lwu	s2, SBI_PLATFORM_HART_COUNT_OFFSET(s2)
#else
	lw	s0, SBI_PLATFORM_HART_STACK_SIZE_OFFSET(s2)
	lw	s2, SBI_PLATFORM_HART_COUNT_OFFSET(s2)
#endif
	mul	s2, s2, s0
	la	s1, _fw_end
	add	s1, s1, s2
	mul	s2, s0, a0
	sub	s1, s1, s2
	li	s2, SBI_SCRATCH_SIZE
	sub	a0, s1, s2
	REG_L	s0, (sp)
	REG_L	s1, (__SIZEOF_POINTER__)(sp)
	REG_L	s2, (__SIZEOF_POINTER__ * 2)(sp)
	add	sp, sp, (3 * __SIZEOF_POINTER__)
	ret

	.align 3
	.section .entry, "ax", %progbits
	.globl _start_hang
_start_hang:
	wfi
	j	_start_hang

	.align 3
	.section .entry, "ax", %progbits
	.globl _trap_handler
_trap_handler:
	/* Swap TP and MSCRATCH */
	csrrw	tp, CSR_MSCRATCH, tp

	/* Save T0 in scratch space */
	REG_S	t0, SBI_SCRATCH_TMP0_OFFSET(tp)

	/* Check which mode we came from */
	csrr	t0, CSR_MSTATUS
	srl	t0, t0, MSTATUS_MPP_SHIFT
	and	t0, t0, PRV_M
	xori	t0, t0, PRV_M
	beq	t0, zero, _trap_handler_m_mode

	/* We came from S-mode or U-mode */
_trap_handler_s_mode:
	/* Set T0 to original SP */
	add	t0, sp, zero

	/* Setup exception stack */
	add	sp, tp, -(SBI_TRAP_REGS_SIZE)

	/* Jump to code common for all modes */
	j	_trap_handler_all_mode

	/* We came from M-mode */
_trap_handler_m_mode:
	/* Set T0 to original SP */
	add	t0, sp, zero

	/* Re-use current SP as exception stack */
	add	sp, sp, -(SBI_TRAP_REGS_SIZE)

_trap_handler_all_mode:
	/* Save original SP (from T0) on stack */
	REG_S	t0, SBI_TRAP_REGS_OFFSET(sp)(sp)

	/* Restore T0 from scratch space */
	REG_L	t0, SBI_SCRATCH_TMP0_OFFSET(tp)

	/* Save T0 on stack */
	REG_S	t0, SBI_TRAP_REGS_OFFSET(t0)(sp)

	/* Swap TP and MSCRATCH */
	csrrw	tp, CSR_MSCRATCH, tp

	/* Save MEPC and MSTATUS CSRs */
	csrr	t0, CSR_MEPC
	REG_S	t0, SBI_TRAP_REGS_OFFSET(mepc)(sp)
	csrr	t0, CSR_MSTATUS
	REG_S	t0, SBI_TRAP_REGS_OFFSET(mstatus)(sp)

	/* Save all general regisers except SP and T0 */
	REG_S	zero, SBI_TRAP_REGS_OFFSET(zero)(sp)
	REG_S	ra, SBI_TRAP_REGS_OFFSET(ra)(sp)
	REG_S	gp, SBI_TRAP_REGS_OFFSET(gp)(sp)
	REG_S	tp, SBI_TRAP_REGS_OFFSET(tp)(sp)
	REG_S	t1, SBI_TRAP_REGS_OFFSET(t1)(sp)
	REG_S	t2, SBI_TRAP_REGS_OFFSET(t2)(sp)
	REG_S	s0, SBI_TRAP_REGS_OFFSET(s0)(sp)
	REG_S	s1, SBI_TRAP_REGS_OFFSET(s1)(sp)
	REG_S	a0, SBI_TRAP_REGS_OFFSET(a0)(sp)
	REG_S	a1, SBI_TRAP_REGS_OFFSET(a1)(sp)
	REG_S	a2, SBI_TRAP_REGS_OFFSET(a2)(sp)
	REG_S	a3, SBI_TRAP_REGS_OFFSET(a3)(sp)
	REG_S	a4, SBI_TRAP_REGS_OFFSET(a4)(sp)
	REG_S	a5, SBI_TRAP_REGS_OFFSET(a5)(sp)
	REG_S	a6, SBI_TRAP_REGS_OFFSET(a6)(sp)
	REG_S	a7, SBI_TRAP_REGS_OFFSET(a7)(sp)
	REG_S	s2, SBI_TRAP_REGS_OFFSET(s2)(sp)
	REG_S	s3, SBI_TRAP_REGS_OFFSET(s3)(sp)
	REG_S	s4, SBI_TRAP_REGS_OFFSET(s4)(sp)
	REG_S	s5, SBI_TRAP_REGS_OFFSET(s5)(sp)
	REG_S	s6, SBI_TRAP_REGS_OFFSET(s6)(sp)
	REG_S	s7, SBI_TRAP_REGS_OFFSET(s7)(sp)
	REG_S	s8, SBI_TRAP_REGS_OFFSET(s8)(sp)
	REG_S	s9, SBI_TRAP_REGS_OFFSET(s9)(sp)
	REG_S	s10, SBI_TRAP_REGS_OFFSET(s10)(sp)
	REG_S	s11, SBI_TRAP_REGS_OFFSET(s11)(sp)
	REG_S	t3, SBI_TRAP_REGS_OFFSET(t3)(sp)
	REG_S	t4, SBI_TRAP_REGS_OFFSET(t4)(sp)
	REG_S	t5, SBI_TRAP_REGS_OFFSET(t5)(sp)
	REG_S	t6, SBI_TRAP_REGS_OFFSET(t6)(sp)

	/* Call C routine */
	add	a0, sp, zero
	csrr	a1, CSR_MSCRATCH
	call	sbi_trap_handler

	/* Restore all general regisers except SP and T0 */
	REG_L	ra, SBI_TRAP_REGS_OFFSET(ra)(sp)
	REG_L	gp, SBI_TRAP_REGS_OFFSET(gp)(sp)
	REG_L	tp, SBI_TRAP_REGS_OFFSET(tp)(sp)
	REG_L	t1, SBI_TRAP_REGS_OFFSET(t1)(sp)
	REG_L	t2, SBI_TRAP_REGS_OFFSET(t2)(sp)
	REG_L	s0, SBI_TRAP_REGS_OFFSET(s0)(sp)
	REG_L	s1, SBI_TRAP_REGS_OFFSET(s1)(sp)
	REG_L	a0, SBI_TRAP_REGS_OFFSET(a0)(sp)
	REG_L	a1, SBI_TRAP_REGS_OFFSET(a1)(sp)
	REG_L	a2, SBI_TRAP_REGS_OFFSET(a2)(sp)
	REG_L	a3, SBI_TRAP_REGS_OFFSET(a3)(sp)
	REG_L	a4, SBI_TRAP_REGS_OFFSET(a4)(sp)
	REG_L	a5, SBI_TRAP_REGS_OFFSET(a5)(sp)
	REG_L	a6, SBI_TRAP_REGS_OFFSET(a6)(sp)
	REG_L	a7, SBI_TRAP_REGS_OFFSET(a7)(sp)
	REG_L	s2, SBI_TRAP_REGS_OFFSET(s2)(sp)
	REG_L	s3, SBI_TRAP_REGS_OFFSET(s3)(sp)
	REG_L	s4, SBI_TRAP_REGS_OFFSET(s4)(sp)
	REG_L	s5, SBI_TRAP_REGS_OFFSET(s5)(sp)
	REG_L	s6, SBI_TRAP_REGS_OFFSET(s6)(sp)
	REG_L	s7, SBI_TRAP_REGS_OFFSET(s7)(sp)
	REG_L	s8, SBI_TRAP_REGS_OFFSET(s8)(sp)
	REG_L	s9, SBI_TRAP_REGS_OFFSET(s9)(sp)
	REG_L	s10, SBI_TRAP_REGS_OFFSET(s10)(sp)
	REG_L	s11, SBI_TRAP_REGS_OFFSET(s11)(sp)
	REG_L	t3, SBI_TRAP_REGS_OFFSET(t3)(sp)
	REG_L	t4, SBI_TRAP_REGS_OFFSET(t4)(sp)
	REG_L	t5, SBI_TRAP_REGS_OFFSET(t5)(sp)
	REG_L	t6, SBI_TRAP_REGS_OFFSET(t6)(sp)

	/* Restore MEPC and MSTATUS CSRs */
	REG_L	t0, SBI_TRAP_REGS_OFFSET(mepc)(sp)
	csrw	CSR_MEPC, t0
	REG_L	t0, SBI_TRAP_REGS_OFFSET(mstatus)(sp)
	csrw	CSR_MSTATUS, t0

	/* Restore T0 */
	REG_L	t0, SBI_TRAP_REGS_OFFSET(t0)(sp)

	/* Restore SP */
	REG_L	sp, SBI_TRAP_REGS_OFFSET(sp)(sp)

	mret

	.align 3
	.section .entry, "ax", %progbits
	.globl _reset_regs
_reset_regs:

	/* flush the instruction cache */
	fence.i
	/* Reset all registers except ra, a0,a1 */
	li sp, 0
	li gp, 0
	li tp, 0
	li t0, 0
	li t1, 0
	li t2, 0
	li s0, 0
	li s1, 0
	li a2, 0
	li a3, 0
	li a4, 0
	li a5, 0
	li a6, 0
	li a7, 0
	li s2, 0
	li s3, 0
	li s4, 0
	li s5, 0
	li s6, 0
	li s7, 0
	li s8, 0
	li s9, 0
	li s10, 0
	li s11, 0
	li t3, 0
	li t4, 0
	li t5, 0
	li t6, 0
	csrw CSR_MSCRATCH, 0

	ret
