/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#ifndef __SBI_CONSOLE_H__
#define __SBI_CONSOLE_H__

#include <sbi/sbi_types.h>

#define __printf(a, b) __attribute__((format(printf, a, b)))

bool sbi_isprintable(char ch);

int sbi_getc(void);

void sbi_putc(char ch);

void sbi_puts(const char *str);

void sbi_gets(char *s, int maxwidth, char endchar);

int __printf(2, 3) sbi_sprintf(char *out, const char *format, ...);

int __printf(3, 4) sbi_snprintf(char *out, u32 out_sz, const char *format, ...);

int __printf(1, 2) sbi_printf(const char *format, ...);

struct sbi_scratch;

int __printf(2, 3) sbi_dprintf(struct sbi_scratch *scratch,
			       const char *format, ...);

int sbi_console_init(struct sbi_scratch *scratch);

#endif
