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
#include <sbi/sbi_scratch.h>

struct sbi_irqchip_hart_data {
	struct sbi_irqchip_device *chip;
};

static unsigned long irqchip_hart_data_off;
static SBI_LIST_HEAD(irqchip_list);

int sbi_irqchip_process(void)
{
	struct sbi_irqchip_hart_data *hd;

	hd = sbi_scratch_thishart_offset_ptr(irqchip_hart_data_off);
	if (!hd || !hd->chip || !hd->chip->process_hwirqs)
		return SBI_ENODEV;

	return hd->chip->process_hwirqs(hd->chip);
}

struct sbi_irqchip_device *sbi_irqchip_find_device(u32 id)
{
	struct sbi_irqchip_device *chip;

	sbi_list_for_each_entry(chip, &irqchip_list, node) {
		if (chip->id == id)
			return chip;
	}

	return NULL;
}

int sbi_irqchip_add_device(struct sbi_irqchip_device *chip)
{
	struct sbi_irqchip_hart_data *hd;
	struct sbi_scratch *scratch;
	u32 h;

	if (!chip || !sbi_hartmask_weight(&chip->target_harts))
		return SBI_EINVAL;

	if (sbi_irqchip_find_device(chip->id))
		return SBI_EALREADY;

	if (chip->process_hwirqs) {
		sbi_hartmask_for_each_hartindex(h, &chip->target_harts) {
			scratch = sbi_hartindex_to_scratch(h);
			if (!scratch)
				continue;

			hd = sbi_scratch_offset_ptr(scratch, irqchip_hart_data_off);
			if (hd->chip && hd->chip != chip)
				return SBI_EINVAL;

			hd->chip = chip;
		}
	}

	sbi_list_add_tail(&chip->node, &irqchip_list);
	return 0;
}

int sbi_irqchip_init(struct sbi_scratch *scratch, bool cold_boot)
{
	const struct sbi_platform *plat = sbi_platform_ptr(scratch);
	struct sbi_irqchip_hart_data *hd;
	struct sbi_irqchip_device *chip;
	int rc;

	if (cold_boot) {
		irqchip_hart_data_off =
			sbi_scratch_alloc_offset(sizeof(struct sbi_irqchip_hart_data));
		if (!irqchip_hart_data_off)
			return SBI_ENOMEM;
		rc = sbi_platform_irqchip_init(plat);
		if (rc)
			return rc;
	}

	sbi_list_for_each_entry(chip, &irqchip_list, node) {
		if (!chip->warm_init)
			continue;
		if (!sbi_hartmask_test_hartindex(current_hartindex(), &chip->target_harts))
			continue;
		rc = chip->warm_init(chip);
		if (rc)
			return rc;
	}

	hd = sbi_scratch_thishart_offset_ptr(irqchip_hart_data_off);
	if (hd && hd->chip && hd->chip->process_hwirqs)
		csr_set(CSR_MIE, MIP_MEIP);

	return 0;
}

void sbi_irqchip_exit(struct sbi_scratch *scratch)
{
	struct sbi_irqchip_hart_data *hd;

	hd = sbi_scratch_thishart_offset_ptr(irqchip_hart_data_off);
	if (hd && hd->chip && hd->chip->process_hwirqs)
		csr_clear(CSR_MIE, MIP_MEIP);
}
