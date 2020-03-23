// SPDX-License-Identifier: BSD-2-Clause
/*
 * fdt_helper.h - Flat Device Tree parsing helper routines
 * Implement helper routines to parse FDT nodes on top of
 * libfdt for OpenSBI usage
 *
 * Copyright (C) 2020 Bin Meng <bmeng.cn@gmail.com>
 */

#ifndef __FDT_HELPER_H__
#define __FDT_HELPER_H__

struct platform_uart_data {
	unsigned long addr;
	unsigned long freq;
	unsigned long baud;
};

struct platform_plic_data {
	unsigned long addr;
	unsigned long num_src;
};

int fdt_parse_uart8250(void *fdt, struct platform_uart_data *uart,
		       const char *compatible);

int fdt_parse_plic(void *fdt, struct platform_plic_data *plic,
		   const char *compatible);

int fdt_parse_clint(void *fdt, unsigned long *clint_addr,
		    const char *compatible);

#endif /* __FDT_HELPER_H__ */
