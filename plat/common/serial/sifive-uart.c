/*
 * Copyright (c) 2018 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sbi/riscv_io.h>
#include <plat/serial/sifive-uart.h>

#define UART_REG_TXFIFO		0
#define UART_REG_RXFIFO		1
#define UART_REG_TXCTRL		2
#define UART_REG_RXCTRL		3
#define UART_REG_IE		4
#define UART_REG_IP		5
#define UART_REG_DIV		6

#define UART_TXFIFO_FULL	0x80000000
#define UART_RXFIFO_EMPTY	0x80000000
#define UART_RXFIFO_DATA	0x000000ff
#define UART_TXCTRL_TXEN	0x1
#define UART_RXCTRL_RXEN	0x1

static volatile void *uart_base;
static u32 uart_in_freq;
static u32 uart_baudrate;

static u32 get_reg(u32 num)
{
	return readl(uart_base + (num * 0x4));
}

static void set_reg(u32 num, u32 val)
{
	writel(val, uart_base + (num * 0x4));
}

void sifive_uart_putc(char ch)
{
	while (get_reg(UART_REG_TXFIFO) & UART_TXFIFO_FULL);

	set_reg(UART_REG_TXFIFO, ch);
}

char sifive_uart_getc(void)
{
	u32 ret = get_reg(UART_REG_RXFIFO);
	if (!(ret & UART_RXFIFO_EMPTY))
		return ret & UART_RXFIFO_DATA;
	return 0;
}

int sifive_uart_init(unsigned long base,
		     u32 in_freq, u32 baudrate)
{
	uart_base = (volatile void *)base;
	uart_in_freq = in_freq;
	uart_baudrate = baudrate;

	/* Configure baudrate */
	set_reg(UART_REG_DIV, (in_freq / baudrate) - 1);
	/* Disable interrupts */
	set_reg(UART_REG_IE, 0);
	/* Enable TX */
	set_reg(UART_REG_TXCTRL, UART_TXCTRL_TXEN);
	/* Enable Rx */
	set_reg(UART_REG_RXCTRL, UART_RXCTRL_RXEN);

	return 0;
}
