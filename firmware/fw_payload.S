/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include "fw_base.S"

	.align 4
	.section .entry, "ax", %progbits
	.global fw_prev_arg1
fw_prev_arg1:
	/* We return previous arg1 in 'a0' */
#ifdef FW_PAYLOAD_FDT_PATH
	la	a0, fdt_bin
#else
	add	a0, zero, zero
#endif
	ret

	.align 4
	.section .entry, "ax", %progbits
	.global fw_next_arg1
fw_next_arg1:
	/* We return next arg1 in 'a0' */
#ifdef FW_PAYLOAD_FDT_ADDR
	li	a0, FW_PAYLOAD_FDT_ADDR
#else
	add	a0, zero, zero
#endif
	ret

	.align 4
	.section .entry, "ax", %progbits
	.global fw_next_addr
fw_next_addr:
	/* We return next address in 'a0' */
	la	a0, payload_bin
	ret

#ifdef FW_PAYLOAD_FDT_PATH
	.align 4
	.section .text, "ax", %progbits
	.globl fdt_bin
fdt_bin:
	.incbin	FW_PAYLOAD_FDT_PATH
#endif

	.align 4
	.section .payload, "ax", %progbits
	.globl payload_bin
payload_bin:
#ifndef FW_PAYLOAD_PATH
	wfi
	j	payload_bin
#else
	.incbin	FW_PAYLOAD_PATH
#endif
