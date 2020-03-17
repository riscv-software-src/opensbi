/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#ifndef __IRQCHIP_PLIC_H__
#define __IRQCHIP_PLIC_H__

#include <sbi/sbi_types.h>

int plic_warm_irqchip_init(u32 target_hart, int m_cntx_id, int s_cntx_id);

int plic_cold_irqchip_init(unsigned long base, u32 num_sources, u32 hart_count);

void plic_set_thresh(u32 cntxid, u32 val);

void plic_set_ie(u32 cntxid, u32 word_index, u32 val);

#endif
