/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#ifndef __FDT_RESET_H__
#define __FDT_RESET_H__

#include <sbi/sbi_types.h>
#include <sbi_utils/fdt/fdt_driver.h>

#ifdef CONFIG_FDT_RESET

/**
 * fdt_reset_init() - initialize reset drivers based on the device-tree
 *
 * This function shall be invoked in final init.
 */
void fdt_reset_init(const void *fdt);

#else

static inline void fdt_reset_init(const void *fdt) { }

#endif

#endif
