/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2021 Guokai Chen <chenguokai17@mails.ucas.ac.cn>
 *
 * This code is based on lib/utils/serial/xilinx-uart.c
 */

#include <sbi/riscv_io.h>
#include <sbi/sbi_console.h>
#include <sbi_utils/serial/xilinx-uart.h>

#define REG_TX		0x04
#define REG_RX		0x00
#define REG_STATUS	0x08
#define REG_CONTROL	0x0C

#define UART_TX_FULL    (1<<0x3)
#define UART_RX_VALID   (1<<0x0)

static volatile void *uart_base;

void xilinx_uart_putc(char ch)
{
	while((readb(uart_base + REG_STATUS) & UART_TX_FULL))
		;
	writeb(ch, uart_base + REG_TX);
}

int xilinx_uart_getc(void)
{
	u16 status = readb(uart_base + REG_STATUS);
	if (status & UART_RX_VALID)
		return readb(uart_base + REG_RX);
	return -1;
}

int xilinx_uart_init(unsigned long base, u32 in_freq, u32 baudrate)
{
	uart_base = (volatile void *)base;
	return 0;
}
