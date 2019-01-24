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
	.global fw_prev_arg1
fw_prev_arg1:
	/* We return previous arg1 in 'a0' */
	add	a0, zero, zero
	ret

	.align 3
	.section .entry, "ax", %progbits
	.global fw_next_arg1
fw_next_arg1:
	/* We return next arg1 in 'a0' */
#ifdef FW_JUMP_FDT_ADDR
	li	a0, FW_JUMP_FDT_ADDR
#else
	add	a0, zero, zero
#endif
	ret

	.align 3
	.section .entry, "ax", %progbits
	.global fw_next_addr
fw_next_addr:
	/* We return next address in 'a0' */
	la	a0, _jump_addr
	REG_L	a0, (a0)
	ret

#ifndef FW_JUMP_ADDR
#error "Must define FW_JUMP_ADDR"
#endif

	.align 3
	.section .entry, "ax", %progbits
_jump_addr:
	RISCV_PTR FW_JUMP_ADDR
