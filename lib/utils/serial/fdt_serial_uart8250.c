/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/serial/fdt_serial.h>
#include <sbi_utils/serial/uart8250.h>

static int serial_uart8250_init(const void *fdt, int nodeoff,
				const struct fdt_match *match)
{
	struct platform_uart_data uart = { 0 };
	ulong caps = (ulong)match->data;
	int rc;

	rc = fdt_parse_uart_node(fdt, nodeoff, &uart);
	if (rc)
		return rc;

	return uart8250_init(uart.addr, uart.freq, uart.baud,
			     uart.reg_shift, uart.reg_io_width,
			     uart.reg_offset, caps);
}

static const struct fdt_match serial_uart8250_match[] = {
	{ .compatible = "ns16550" },
	{ .compatible = "ns16550a" },
	{ .compatible = "snps,dw-apb-uart" },
	{ .compatible = "intel,xscale-uart",
	  .data = (void *)UART_CAP_UUE },
	{ },
};

const struct fdt_driver fdt_serial_uart8250 = {
	.match_table = serial_uart8250_match,
	.init = serial_uart8250_init,
};
