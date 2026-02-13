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

#include <sbi/sbi_hartmask.h>
#include <sbi/sbi_list.h>
#include <sbi/sbi_types.h>

struct sbi_scratch;

/** irqchip hardware device */
struct sbi_irqchip_device {
	/** Node in the list of irqchip devices (private) */
	struct sbi_dlist node;

	/** Internal data of all hardware interrupts of this irqchip (private) */
	struct sbi_irqchip_hwirq_data *hwirqs;

	/** List of interrupt handlers */
	struct sbi_dlist handler_list;

	/** Unique ID of this irqchip */
	u32 id;

	/** Number of hardware IRQs of this irqchip */
	u32 num_hwirq;

	/** Set of harts targetted by this irqchip */
	struct sbi_hartmask target_harts;

	/** Initialize per-hart state for the current hart */
	int (*warm_init)(struct sbi_irqchip_device *chip);

	/** Process hardware interrupts from this irqchip */
	int (*process_hwirqs)(struct sbi_irqchip_device *chip);

	/** Setup a hardware interrupt of this irqchip */
	int (*hwirq_setup)(struct sbi_irqchip_device *chip, u32 hwirq);

	/** Cleanup a hardware interrupt of this irqchip */
	void (*hwirq_cleanup)(struct sbi_irqchip_device *chip, u32 hwirq);

	/** End of hardware interrupt of this irqchip */
	void (*hwirq_eoi)(struct sbi_irqchip_device *chip, u32 hwirq);

	/** Mask a hardware interrupt of this irqchip */
	void (*hwirq_mask)(struct sbi_irqchip_device *chip, u32 hwirq);

	/** Unmask a hardware interrupt of this irqchip */
	void (*hwirq_unmask)(struct sbi_irqchip_device *chip, u32 hwirq);
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

/**
 * Process a hwirq of an irqchip device
 *
 * This function is called by irqchip drivers to handle hardware
 * interrupts of the irqchip.
 */
int sbi_irqchip_process_hwirq(struct sbi_irqchip_device *chip, u32 hwirq);

/** Unmask a hardware interrupt */
int sbi_irqchip_unmask_hwirq(struct sbi_irqchip_device *chip, u32 hwirq);

/** Mask a hardware interrupt */
int sbi_irqchip_mask_hwirq(struct sbi_irqchip_device *chip, u32 hwirq);

/** Default raw hardware interrupt handler */
int sbi_irqchip_raw_handler_default(struct sbi_irqchip_device *chip, u32 hwirq);

/** Set raw hardware interrupt handler */
int sbi_irqchip_set_raw_handler(struct sbi_irqchip_device *chip, u32 hwirq,
				int (*raw_hndl)(struct sbi_irqchip_device *, u32));

/** Register a hardware interrupt handler */
int sbi_irqchip_register_handler(struct sbi_irqchip_device *chip,
				 u32 first_hwirq, u32 num_hwirq,
				 int (*callback)(u32 hwirq, void *opaque), void *opaque);

/** Unregister a hardware interrupt handler */
int sbi_irqchip_unregister_handler(struct sbi_irqchip_device *chip,
				   u32 first_hwirq, u32 num_hwirq);

/** Find an irqchip device based on unique ID */
struct sbi_irqchip_device *sbi_irqchip_find_device(u32 id);

/** Register an irqchip device to receive callbacks */
int sbi_irqchip_add_device(struct sbi_irqchip_device *chip);

/** Initialize interrupt controllers */
int sbi_irqchip_init(struct sbi_scratch *scratch, bool cold_boot);

/** Exit interrupt controllers */
void sbi_irqchip_exit(struct sbi_scratch *scratch);

#endif
