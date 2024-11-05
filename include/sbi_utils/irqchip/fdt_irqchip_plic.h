/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022 Samuel Holland <samuel@sholland.org>
 */

#ifndef __IRQCHIP_FDT_IRQCHIP_PLIC_H__
#define __IRQCHIP_FDT_IRQCHIP_PLIC_H__

#include <sbi/sbi_types.h>
#include <sbi_utils/irqchip/plic.h>

struct plic_data *fdt_plic_get(void);

void fdt_plic_suspend(void);

void fdt_plic_resume(void);

#endif
