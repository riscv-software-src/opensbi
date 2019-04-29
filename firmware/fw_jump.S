/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include "fw_base.S"

	.align 3
	.section .entry, "ax", %progbits
	.global fw_save_info
	/*
	 * We can only use a0, a1, a2, a3, and a4 registers here.
	 * The a0, a1, and a2 registers will be same as passed by
	 * previous booting stage.
	 * Nothing to be returned here.
	 */
fw_save_info:
	ret

	.align 3
	.section .entry, "ax", %progbits
	.global fw_prev_arg1
	/*
	 * We can only use a0, a1, and a2 registers here.
	 * The previous arg1 should be returned in 'a0'.
	 */
fw_prev_arg1:
	add	a0, zero, zero
	ret

	.align 3
	.section .entry, "ax", %progbits
	.global fw_next_arg1
	/*
	 * We can only use a0, a1, and a2 registers here.
	 * The next arg1 should be returned in 'a0'.
	 */
fw_next_arg1:
#ifdef FW_JUMP_FDT_ADDR
	li	a0, FW_JUMP_FDT_ADDR
#else
	add	a0, zero, zero
#endif
	ret

	.align 3
	.section .entry, "ax", %progbits
	.global fw_next_addr
	/*
	 * We can only use a0, a1, and a2 registers here.
	 * The next address should be returned in 'a0'.
	 */
fw_next_addr:
	la	a0, _jump_addr
	REG_L	a0, (a0)
	ret

	.align 3
	.section .entry, "ax", %progbits
	.global fw_next_mode
	/*
	 * We can only use a0, a1, and a2 registers here.
	 * The next address should be returned in 'a0'
	 */
fw_next_mode:
	li	a0, PRV_S
	ret

	.align 3
	.section .entry, "ax", %progbits
	.global fw_options
	/*
	 * We can only use a0, a1, and a2 registers here.
	 * The 'a4' register will have default options.
	 * The next address should be returned in 'a0'.
	 */
fw_options:
	add	a0, zero, zero
	ret

#ifndef FW_JUMP_ADDR
#error "Must define FW_JUMP_ADDR"
#endif

	.align 3
	.section .entry, "ax", %progbits
_jump_addr:
	RISCV_PTR FW_JUMP_ADDR
