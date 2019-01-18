/*
 * Copyright (c) 2018 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sbi/sbi_ecall_interface.h>

#define wfi()						\
do {							\
	__asm__ __volatile__ ("wfi" ::: "memory");	\
} while (0)

static void sbi_puts(const char *str)
{
	while (*str) {
		SBI_ECALL_1(SBI_ECALL_CONSOLE_PUTCHAR, *str);
		str++;
	}
}

void test_main(unsigned long a0, unsigned long a1)
{
	sbi_puts("\nTest payload running\n");

	while (1)
		wfi();
}
