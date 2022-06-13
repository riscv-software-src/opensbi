/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022 Samuel Holland <samuel@sholland.org>
 */

#ifndef __IRQCHIP_FDT_IRQCHIP_PLIC_H__
#define __IRQCHIP_FDT_IRQCHIP_PLIC_H__

#include <sbi/sbi_types.h>

void fdt_plic_priority_save(u8 *priority);

void fdt_plic_priority_restore(const u8 *priority);

void fdt_plic_context_save(bool smode, u32 *enable, u32 *threshold);

void fdt_plic_context_restore(bool smode, const u32 *enable, u32 threshold);

void thead_plic_restore(void);

#endif
