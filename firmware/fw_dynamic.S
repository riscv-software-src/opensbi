/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <sbi/fw_dynamic.h>

#include "fw_base.S"

	.align 3
	.section .entry, "ax", %progbits
_bad_dynamic_info:
	wfi
	j	_bad_dynamic_info

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
	la	a4, _dynamic_next_arg1
	REG_S	a1, (a4)
	li	a4, FW_DYNAMIC_INFO_MAGIC_VALUE
	REG_L	a3, FW_DYNAMIC_INFO_MAGIC_OFFSET(a2)
	bne	a3, a4, _bad_dynamic_info
	li	a4, FW_DYNAMIC_INFO_VERSION_MAX
	REG_L	a3, FW_DYNAMIC_INFO_VERSION_OFFSET(a2)
	bgt	a3, a4, _bad_dynamic_info
	la	a4, _dynamic_next_addr
	REG_L	a3, FW_DYNAMIC_INFO_NEXT_ADDR_OFFSET(a2)
	REG_S	a3, (a4)
	la	a4, _dynamic_next_mode
	REG_L	a3, FW_DYNAMIC_INFO_NEXT_MODE_OFFSET(a2)
	REG_S	a3, (a4)
	la	a4, _dynamic_options
	REG_L	a3, FW_DYNAMIC_INFO_OPTIONS_OFFSET(a2)
	REG_S	a3, (a4)
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
	la	a0, _dynamic_next_arg1
	REG_L	a0, (a0)
	ret

	.align 3
	.section .entry, "ax", %progbits
	.global fw_next_addr
	/*
	 * We can only use a0, a1, and a2 registers here.
	 * The next address should be returned in 'a0'.
	 */
fw_next_addr:
	la	a0, _dynamic_next_addr
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
	la	a0, _dynamic_next_mode
	REG_L	a0, (a0)
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
	la	a0, _dynamic_options
	REG_L	a0, (a0)
	ret

	.align 3
	.section .entry, "ax", %progbits
_dynamic_next_arg1:
	RISCV_PTR 0x0
_dynamic_next_addr:
	RISCV_PTR 0x0
_dynamic_next_mode:
	RISCV_PTR PRV_S
_dynamic_options:
	RISCV_PTR 0x0
