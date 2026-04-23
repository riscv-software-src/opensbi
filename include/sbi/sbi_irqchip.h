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

/** irqchip message signalled interrupt (MSI) */
struct sbi_irqchip_msi_msg {
	u32 address_lo;
	u32 address_hi;
	u32 data;
};

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
	int (*hwirq_setup)(struct sbi_irqchip_device *chip, u32 hwirq,
			   u32 hwirq_flags);
#define SBI_HWIRQ_FLAGS_NONE			0x00000000UL
#define SBI_HWIRQ_FLAGS_EDGE_RISING		0x00000001UL
#define SBI_HWIRQ_FLAGS_EDGE_FALLING		0x00000002UL
#define SBI_HWIRQ_FLAGS_EDGE_BOTH		(SBI_HWIRQ_FLAGS_EDGE_RISING | \
						 SBI_HWIRQ_FLAGS_EDGE_FALLING)
#define SBI_HWIRQ_FLAGS_LEVEL_HIGH		0x00000004UL
#define SBI_HWIRQ_FLAGS_LEVEL_LOW		0x00000008UL
#define SBI_HWIRQ_FLAGS_LEVEL_SENSE_MASK	0x0000000fUL

	/** Cleanup a hardware interrupt of this irqchip */
	void (*hwirq_cleanup)(struct sbi_irqchip_device *chip, u32 hwirq);

	/** End of hardware interrupt of this irqchip */
	void (*hwirq_eoi)(struct sbi_irqchip_device *chip, u32 hwirq);

	/** Set hardware interrupt affinity */
	int (*hwirq_set_affinity)(struct sbi_irqchip_device *chip, u32 hwirq,
				  u32 hart_index);

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

/** Get hardware interrupt affinity */
int sbi_irqchip_get_affinity(struct sbi_irqchip_device *chip, u32 hwirq,
			     u32 *out_hart_index);

/** Set hardware interrupt affinity */
int sbi_irqchip_set_affinity(struct sbi_irqchip_device *chip, u32 hwirq, u32 hart_index);

/** Write MSI message to the hardware interrupt handler */
int sbi_irqchip_write_msi(struct sbi_irqchip_device *chip, u32 hwirq,
			  const struct sbi_irqchip_msi_msg *msg);

/** Register a hardware MSI handler */
int sbi_irqchip_register_msi(struct sbi_irqchip_device *chip, u32 num_hwirq,
			     void (*write_msi)(u32 hwirq,
					       const struct sbi_irqchip_msi_msg *msg,
					       void *priv),
			     int (*callback)(u32 hwirq, void *priv), void *priv,
			     u32 *out_first_hwirq);

/** Register a hardware interrupt handler */
int sbi_irqchip_register_handler(struct sbi_irqchip_device *chip,
				 u32 first_hwirq, u32 num_hwirq, u32 hwirq_flags,
				 int (*callback)(u32 hwirq, void *priv), void *priv);

/** Register a hardware interrupts as reserved */
int sbi_irqchip_register_reserved(struct sbi_irqchip_device *chip,
				  u32 first_hwirq, u32 num_hwirq);

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
