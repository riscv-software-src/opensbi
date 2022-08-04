/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Bohdan Fedorov <misterjdrg@gmail.com>
 *
 * Based on virtio-uart
 */

#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/serial/fdt_serial.h>
#include <sbi_utils/serial/virtio-uart.h>

static int serial_virtio_init(void *fdt, int nodeoff,
				const struct fdt_match *match)
{
	int rc;
	struct platform_uart_data uart;

	rc = fdt_parse_virtio_uart_node(fdt, nodeoff, &uart);
	if (rc)
		return rc;

	return virtio_uart_init(uart.addr);
}

static const struct fdt_match serial_virtio_match[] = {
	{ .compatible = "virtio,mmio" },
	{ },
};

struct fdt_serial fdt_serial_virtio = {
	.match_table = serial_virtio_match,
	.init = serial_virtio_init
};
