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
#include <sbi/sbi_irqchip.h>

struct plic_data {
	/* Private members */
	struct sbi_irqchip_device irqchip;
	/* Public members */
	unsigned long addr;
	unsigned long size;
	unsigned long num_src;
	unsigned long flags;
	void *pm_data;
	s16 context_map[][2];
};

/** Work around a bug on Ariane that requires enabling interrupts at boot */
#define PLIC_FLAG_ARIANE_BUG		BIT(0)
/** PLIC must be delegated to S-mode like T-HEAD C906 and C910 */
#define PLIC_FLAG_THEAD_DELEGATION	BIT(1)
/** Allocate space for power management save/restore operations */
#define PLIC_FLAG_ENABLE_PM		BIT(2)

#define PLIC_M_CONTEXT			0
#define PLIC_S_CONTEXT			1

#define PLIC_DATA_SIZE(__hart_count)	(sizeof(struct plic_data) + \
					 (__hart_count) * 2 * sizeof(s16))

#define PLIC_IE_WORDS(__p)		((__p)->num_src / 32 + 1)

struct plic_data *plic_get(void);

void plic_suspend(void);

void plic_resume(void);

int plic_cold_irqchip_init(struct plic_data *plic);

#endif
