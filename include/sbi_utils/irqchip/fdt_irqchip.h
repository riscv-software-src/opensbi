/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#ifndef __FDT_IRQCHIP_H__
#define __FDT_IRQCHIP_H__

#include <sbi/sbi_types.h>
#include <sbi_utils/fdt/fdt_driver.h>

#ifdef CONFIG_FDT_IRQCHIP

int fdt_irqchip_init(void);

#else

static inline int fdt_irqchip_init(void) { return 0; }

#endif

#endif
