/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2021 Guokai Chen <chenguokai17@mails.ucas.ac.cn>
 * 
 * This code is based on lib/utils/serial/fdt_serial_shakti.c
 */

#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/serial/fdt_serial.h>
#include <sbi_utils/serial/xilinx-uart.h>

static int serial_xilinx_init(void *fdt, int nodeoff,
				const struct fdt_match *match)
{
		
	int rc;
	struct platform_uart_data uart;
	// no need to reinvent a wheel
	rc = fdt_parse_shakti_uart_node(fdt, nodeoff, &uart);
	if (rc)
		return rc;
	return xilinx_uart_init(uart.addr, uart.freq, uart.baud);
}

static const struct fdt_match serial_xilinx_match[] = {
	{ .compatible = "xilinx,uartlite" },
	{ },
};

struct fdt_serial fdt_serial_xilinx = {
	.match_table = serial_xilinx_match,
	.init = serial_xilinx_init,
	.getc = xilinx_uart_getc,
	.putc = xilinx_uart_putc
};
