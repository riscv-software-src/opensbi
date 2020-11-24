/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#ifndef __SYS_SIFIVE_TEST_H__
#define __SYS_SIFIVE_TEST_H__

#include <sbi/sbi_types.h>

int sifive_test_system_reset_check(u32 type, u32 reason);

void sifive_test_system_reset(u32 type, u32 reason);

int sifive_test_init(unsigned long base);

#endif
