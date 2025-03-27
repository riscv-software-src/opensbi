/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#ifndef __SERIAL_UART8250_H__
#define __SERIAL_UART8250_H__

#include <sbi/sbi_types.h>

#define UART_CAP_UUE	BIT(0)	/* Check UUE capability for XScale PXA UARTs */

int uart8250_init(unsigned long base, u32 in_freq, u32 baudrate, u32 reg_shift,
		  u32 reg_width, u32 reg_offset, u32 caps);

#endif
