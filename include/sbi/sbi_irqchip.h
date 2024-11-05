/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Anup Patel <apatel@ventanamicro.com>
 */

#ifndef __SBI_IRQCHIP_H__
#define __SBI_IRQCHIP_H__

#include <sbi/sbi_list.h>
#include <sbi/sbi_types.h>

struct sbi_scratch;

/** irqchip hardware device */
struct sbi_irqchip_device {
	/** Node in the list of irqchip devices */
	struct sbi_dlist node;

	/** Initialize per-hart state for the current hart */
	int (*warm_init)(struct sbi_irqchip_device *dev);

	/** Handle an IRQ from this irqchip */
	int (*irq_handle)(void);
};

/**
 * Process external interrupts
 *
 * This function is called by sbi_trap_handler() to handle external
 * interrupts.
 *
 * @param regs pointer for trap registers
 */
int sbi_irqchip_process(void);

/** Register an irqchip device to receive callbacks */
void sbi_irqchip_add_device(struct sbi_irqchip_device *dev);

/** Initialize interrupt controllers */
int sbi_irqchip_init(struct sbi_scratch *scratch, bool cold_boot);

/** Exit interrupt controllers */
void sbi_irqchip_exit(struct sbi_scratch *scratch);

#endif
