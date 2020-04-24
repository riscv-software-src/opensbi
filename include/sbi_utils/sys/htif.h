/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2010-2020, The Regents of the University of California
 * (Regents).  All Rights Reserved.
 */

#ifndef __SYS_HTIF_H__
#define __SYS_HTIF_H__

#include <sbi/sbi_types.h>

void htif_putc(char ch);

int htif_getc(void);

int htif_system_reset(u32 type);

#endif
