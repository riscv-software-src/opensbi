/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Bohdan Fedorov <misterjdrg@gmial.com>
 *
 * Based on:
 *   sifive-uart
 */

#include <sbi/riscv_io.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_console.h>
#include <sbi_utils/serial/virtio-uart.h>

/* clang-format off */

#define VIRTIO_MAGIC                  0x0
#define VIRTIO_MAGIC_VALUE            0x74726976
#define VIRTIO_DEVICE_ID	      0x8
#define VIRTIO_DEVICE_ID_CONSOLE      0x3
#define VIRTIO_CONSOLE_F_EMERG_WRITE  (1 << 2)
#define VIRTIO_DEVICE_FEATURES 	      0x10
#define VIRTIO_DEVICE_FEATURES_SEL    0x14
#define VIRTIO_CONSOLE_EMERG_WR       0x108

/* clang-format on */

static volatile void *uart_base;

static u32 get_reg(u32 off)
{
	return readl(uart_base + off);
}

static void set_reg(u32 off, u32 val)
{
	writel(val, uart_base + off);
}

static void virtio_uart_putc(char ch)
{
	set_reg(VIRTIO_CONSOLE_EMERG_WR, ch);
}

static struct sbi_console_device virtio_console = {
	.name = "virtio_uart",
	.console_putc = virtio_uart_putc,
	.console_getc = NULL,
};

int virtio_uart_init(unsigned long base)
{
	uart_base = (volatile void *)base;
	
	// Chech magic
	if(get_reg(VIRTIO_MAGIC) != VIRTIO_MAGIC_VALUE) {
		return SBI_ENODEV;
	}

	// Virtio device is console
	if(get_reg(VIRTIO_DEVICE_ID) != VIRTIO_DEVICE_ID_CONSOLE) {
		return SBI_ENODEV;
	}

	// Virtio console has emergency write feature
	set_reg(VIRTIO_DEVICE_FEATURES_SEL, 0);
	if((get_reg(VIRTIO_DEVICE_FEATURES) & VIRTIO_CONSOLE_F_EMERG_WRITE) == 0) {
		return SBI_ENODEV;
	}

	sbi_console_set_device(&virtio_console);
	return 0;
}
