/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#ifndef __SBI_ECALL_INTERFACE_H__
#define __SBI_ECALL_INTERFACE_H__

/* clang-format off */

#define SBI_ECALL_SET_TIMER			0
#define SBI_ECALL_CONSOLE_PUTCHAR		1
#define SBI_ECALL_CONSOLE_GETCHAR		2
#define SBI_ECALL_CLEAR_IPI			3
#define SBI_ECALL_SEND_IPI			4
#define SBI_ECALL_REMOTE_FENCE_I		5
#define SBI_ECALL_REMOTE_SFENCE_VMA		6
#define SBI_ECALL_REMOTE_SFENCE_VMA_ASID	7
#define SBI_ECALL_SHUTDOWN			8

/* clang-format on */

#endif
