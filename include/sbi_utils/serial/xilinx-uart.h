/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2021 Guokai Chen <chenguokai17@mails.ucas.ac.cn>
 *
 * This code is based on include/sbi_utils/serial/shakti-uart.h
 */

#ifndef __SERIAL_XILINX_UART_H__
#define __SERIAL_XILINX_UART_H__

#include <sbi/sbi_types.h>

void xilinx_uart_putc(char ch);

int xilinx_uart_getc(void);

int xilinx_uart_init(unsigned long base, u32 in_freq, u32 baudrate);

#endif
