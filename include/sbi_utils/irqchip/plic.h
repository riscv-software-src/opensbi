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

struct plic_data {
	unsigned long addr;
	unsigned long size;
	unsigned long num_src;
	unsigned long flags;
};

/** Work around a bug on Ariane that requires enabling interrupts at boot */
#define PLIC_FLAG_ARIANE_BUG		BIT(0)
/** PLIC must be delegated to S-mode like T-HEAD C906 and C910 */
#define PLIC_FLAG_THEAD_DELEGATION	BIT(1)

/* So far, priorities on all consumers of these functions fit in 8 bits. */
void plic_priority_save(const struct plic_data *plic, u8 *priority, u32 num);

void plic_priority_restore(const struct plic_data *plic, const u8 *priority,
			   u32 num);

void plic_delegate(const struct plic_data *plic);

void plic_context_save(const struct plic_data *plic, int context_id,
		       u32 *enable, u32 *threshold, u32 num);

void plic_context_restore(const struct plic_data *plic, int context_id,
			  const u32 *enable, u32 threshold, u32 num);

int plic_warm_irqchip_init(const struct plic_data *plic,
			   int m_cntx_id, int s_cntx_id);

int plic_cold_irqchip_init(const struct plic_data *plic);

#endif
