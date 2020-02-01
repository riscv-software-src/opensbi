/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 */

#ifndef __SYS_HTIF_H__
#define __SYS_HTIF_H__

#include <sbi/sbi_types.h>

void htif_putc(char ch);

int htif_getc(void);

int htif_system_down(u32 type);

#endif
