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

struct uart8250_device {
	volatile char *base;
	u32 in_freq;
	u32 baudrate;
	u32 reg_width;
	u32 reg_shift;
};

int uart8250_device_getc(struct uart8250_device *dev);

void uart8250_device_putc(struct uart8250_device *dev, char ch);

void uart8250_device_init(struct uart8250_device *dev, unsigned long base,
			  u32 in_freq, u32 baudrate, u32 reg_shift,
			  u32 reg_width, u32 reg_offset, u32 caps);

int uart8250_init(unsigned long base, u32 in_freq, u32 baudrate, u32 reg_shift,
		  u32 reg_width, u32 reg_offset, u32 caps);

#endif
