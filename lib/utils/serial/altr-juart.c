/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 ISCAS
 *
 * Authors:
 *   Icenowy Zheng <zhengxingda@iscas.ac.cn>
 */

#include <sbi/riscv_io.h>
#include <sbi/sbi_console.h>
#include <sbi_utils/serial/altr-juart.h>

/* clang-format off */

#define JUART_DATA_OFFSET		0x00
# define JUART_DATA_DATA_MASK		0x000000ff
# define JUART_DATA_RVALID		(1 << 15)
# define JUART_DATA_RAVAIL_MASK		0xffff0000
# define JUART_DATA_RAVAIL_SHIFT	16
#define JUART_CTRL_OFFSET		0x04
# define JUART_CTRL_RE			(1 << 0)
# define JUART_CTRL_WE			(1 << 1)
# define JUART_CTRL_RI			(1 << 8)
# define JUART_CTRL_WI			(1 << 9)
# define JUART_CTRL_AC			(1 << 10)
# define JUART_CTRL_WSPACE_MASK		0xffff0000
# define JUART_CTRL_WSPACE_SHIFT	16

/* clang-format on */

static volatile char *altr_juart_base;

static u32 get_reg(u32 offset)
{
	return readl(altr_juart_base + offset);
}

static void set_reg(u32 offset, u32 val)
{
	writel(val, altr_juart_base + offset);
}

static void altr_juart_putc(char ch)
{
	while(!(get_reg(JUART_CTRL_OFFSET) & JUART_CTRL_WSPACE_MASK))
		;

	set_reg(JUART_DATA_OFFSET, (unsigned char)ch);
}

static int altr_juart_getc(void)
{
	u32 reg = get_reg(JUART_DATA_OFFSET);
	if (reg & JUART_DATA_RVALID)
		return reg & JUART_DATA_DATA_MASK;

	return -1;
}

static struct sbi_console_device altr_juart_console = {
	.name = "altr-juart",
	.console_putc = altr_juart_putc,
	.console_getc = altr_juart_getc
};

int altr_juart_init(unsigned long base)
{
	altr_juart_base = (volatile char *)base;

	sbi_console_set_device(&altr_juart_console);

	return 0;
}
