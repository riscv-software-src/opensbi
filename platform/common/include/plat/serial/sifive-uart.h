/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#ifndef __SERIAL_SIFIVE_UART_H__
#define __SERIAL_SIFIVE_UART_H__

#include <sbi/sbi_types.h>

void sifive_uart_putc(char ch);

int sifive_uart_getc(void);

int sifive_uart_init(unsigned long base, u32 in_freq, u32 baudrate);

#endif
