/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Anup Patel <apatel@ventanamicro.com>
 */

#include <sbi/sbi_irqchip.h>
#include <sbi/sbi_list.h>
#include <sbi/sbi_platform.h>

static SBI_LIST_HEAD(irqchip_list);

static int default_irqfn(void)
{
	return SBI_ENODEV;
}

static int (*ext_irqfn)(void) = default_irqfn;

int sbi_irqchip_process(void)
{
	return ext_irqfn();
}

void sbi_irqchip_add_device(struct sbi_irqchip_device *chip)
{
	sbi_list_add_tail(&chip->node, &irqchip_list);

	if (chip->process_hwirqs)
		ext_irqfn = chip->process_hwirqs;
}

int sbi_irqchip_init(struct sbi_scratch *scratch, bool cold_boot)
{
	int rc;
	const struct sbi_platform *plat = sbi_platform_ptr(scratch);
	struct sbi_irqchip_device *chip;

	if (cold_boot) {
		rc = sbi_platform_irqchip_init(plat);
		if (rc)
			return rc;
	}

	sbi_list_for_each_entry(chip, &irqchip_list, node) {
		if (!chip->warm_init)
			continue;
		rc = chip->warm_init(chip);
		if (rc)
			return rc;
	}

	if (ext_irqfn != default_irqfn)
		csr_set(CSR_MIE, MIP_MEIP);

	return 0;
}

void sbi_irqchip_exit(struct sbi_scratch *scratch)
{
	if (ext_irqfn != default_irqfn)
		csr_clear(CSR_MIE, MIP_MEIP);
}
