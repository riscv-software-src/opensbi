/*
 * Copyright (c) 2018 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "fw_common.S"

	.align 3
	.section .entry, "ax", %progbits
	.global fw_next_arg1
fw_next_arg1:
	/* We return FDT destination address in 'a0' */
#ifdef FW_PAYLOAD_FDT_ADDR
	/* a0 = destination FDT start address */
	li	a0, FW_PAYLOAD_FDT_ADDR
#else
	add	a0, zero, zero
#endif
	ret

	.align 3
	.section .entry, "ax", %progbits
	.global fw_next_addr
fw_next_addr:
	/* We return next address in 'a0' */
	la	a0, payload_bin
	ret

#define	str(s)		#s
#define	stringify(s)	str(s)

	.section .payload, "ax", %progbits
	.globl	payload_bin
payload_bin:
#ifndef FW_PAYLOAD_PATH
	wfi
	j	payload_bin
#else
	.incbin	stringify(FW_PAYLOAD_PATH)
#endif
