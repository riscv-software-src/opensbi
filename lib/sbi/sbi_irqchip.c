/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Anup Patel <apatel@ventanamicro.com>
 */

#include <sbi/sbi_heap.h>
#include <sbi/sbi_irqchip.h>
#include <sbi/sbi_list.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_scratch.h>

/** Internal irqchip hardware interrupt data */
struct sbi_irqchip_hwirq_data {
	/** raw hardware interrupt handler */
	int (*raw_handler)(struct sbi_irqchip_device *chip, u32 hwirq);
};

/** Internal irqchip interrupt handler */
struct sbi_irqchip_handler {
	/** Node in the list of irqchip handlers (private) */
	struct sbi_dlist node;

	/** First hardware IRQ handled by this handler */
	u32 first_hwirq;

	/** Number of consecutive hardware IRQs handled by this handler */
	u32 num_hwirq;

	/** Callback function of this handler */
	int (*callback)(u32 hwirq, void *priv);

	/** Callback private data */
	void *priv;
};

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

int sbi_irqchip_process_hwirq(struct sbi_irqchip_device *chip, u32 hwirq)
{
	struct sbi_irqchip_hwirq_data *data;

	if (!chip || chip->num_hwirq <= hwirq)
		return SBI_EINVAL;

	data = &chip->hwirqs[hwirq];
	if (!data->raw_handler)
		return SBI_ENOENT;

	return data->raw_handler(chip, hwirq);
}

int sbi_irqchip_unmask_hwirq(struct sbi_irqchip_device *chip, u32 hwirq)
{
	if (!chip || chip->num_hwirq <= hwirq)
		return SBI_EINVAL;

	if (chip->hwirq_unmask)
		chip->hwirq_unmask(chip, hwirq);
	return 0;
}

int sbi_irqchip_mask_hwirq(struct sbi_irqchip_device *chip, u32 hwirq)
{
	if (!chip || chip->num_hwirq <= hwirq)
		return SBI_EINVAL;

	if (chip->hwirq_mask)
		chip->hwirq_mask(chip, hwirq);
	return 0;
}

static struct sbi_irqchip_handler *sbi_irqchip_find_handler(struct sbi_irqchip_device *chip,
							    u32 hwirq)
{
	struct sbi_irqchip_handler *h;

	if (!chip || chip->num_hwirq <= hwirq)
		return NULL;

	sbi_list_for_each_entry(h, &chip->handler_list, node) {
		if (h->first_hwirq <= hwirq && hwirq < (h->first_hwirq + h->num_hwirq))
			return h;
	}

	return NULL;
}

int sbi_irqchip_raw_handler_default(struct sbi_irqchip_device *chip, u32 hwirq)
{
	struct sbi_irqchip_handler *h;
	int rc;

	if (!chip || chip->num_hwirq <= hwirq)
		return SBI_EINVAL;

	h = sbi_irqchip_find_handler(chip, hwirq);
	rc = h->callback(hwirq, h->priv);

	if (chip->hwirq_eoi)
		chip->hwirq_eoi(chip, hwirq);

	return rc;
}

int sbi_irqchip_set_raw_handler(struct sbi_irqchip_device *chip, u32 hwirq,
				int (*raw_hndl)(struct sbi_irqchip_device *, u32))
{
	struct sbi_irqchip_hwirq_data *data;

	if (!chip || chip->num_hwirq <= hwirq)
		return SBI_EINVAL;

	data = &chip->hwirqs[hwirq];
	data->raw_handler = raw_hndl;
	return 0;
}

int sbi_irqchip_register_handler(struct sbi_irqchip_device *chip,
				 u32 first_hwirq, u32 num_hwirq,
				 int (*callback)(u32 hwirq, void *opaque), void *priv)
{
	struct sbi_irqchip_handler *h;
	u32 i, j;
	int rc;

	if (!chip || !num_hwirq || !callback)
		return SBI_EINVAL;
	if (chip->num_hwirq <= first_hwirq ||
	    chip->num_hwirq <= (first_hwirq + num_hwirq - 1))
		return SBI_EBAD_RANGE;

	h = sbi_irqchip_find_handler(chip, first_hwirq);
	if (h)
		return SBI_EALREADY;
	h = sbi_irqchip_find_handler(chip, first_hwirq + num_hwirq - 1);
	if (h)
		return SBI_EALREADY;

	h = sbi_zalloc(sizeof(*h));
	if (!h)
		return SBI_ENOMEM;
	h->first_hwirq = first_hwirq;
	h->num_hwirq = num_hwirq;
	h->callback = callback;
	h->priv = priv;
	sbi_list_add_tail(&h->node, &chip->handler_list);

	if (chip->hwirq_setup) {
		for (i = 0; i < h->num_hwirq; i++) {
			rc = chip->hwirq_setup(chip, h->first_hwirq + i);
			if (rc) {
				if (chip->hwirq_cleanup) {
					for (j = 0; j < i; j++)
						chip->hwirq_cleanup(chip, h->first_hwirq + j);
				}
				sbi_list_del(&h->node);
				sbi_free(h);
				return rc;
			}
		}
	}

	if (chip->hwirq_unmask) {
		for (i = 0; i < h->num_hwirq; i++)
			chip->hwirq_unmask(chip, h->first_hwirq + i);
	}

	return 0;
}

int sbi_irqchip_unregister_handler(struct sbi_irqchip_device *chip,
				   u32 first_hwirq, u32 num_hwirq)
{
	struct sbi_irqchip_handler *fh, *lh;
	u32 i;

	if (!chip || !num_hwirq)
		return SBI_EINVAL;
	if (chip->num_hwirq <= first_hwirq ||
	    chip->num_hwirq <= (first_hwirq + num_hwirq - 1))
		return SBI_EBAD_RANGE;

	fh = sbi_irqchip_find_handler(chip, first_hwirq);
	if (!fh || fh->first_hwirq != first_hwirq || fh->num_hwirq != num_hwirq)
		return SBI_ENODEV;
	lh = sbi_irqchip_find_handler(chip, first_hwirq + num_hwirq - 1);
	if (!lh || lh != fh)
		return SBI_ENODEV;

	if (chip->hwirq_mask) {
		for (i = 0; i < fh->num_hwirq; i++)
			chip->hwirq_mask(chip, fh->first_hwirq + i);
	}

	if (chip->hwirq_cleanup) {
		for (i = 0; i < fh->num_hwirq; i++)
			chip->hwirq_cleanup(chip, fh->first_hwirq + i);
	}

	sbi_list_del(&fh->node);
	return 0;
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
	u32 i, h;

	if (!chip || !chip->num_hwirq || !sbi_hartmask_weight(&chip->target_harts))
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

	chip->hwirqs = sbi_zalloc(sizeof(*chip->hwirqs) * chip->num_hwirq);
	if (!chip->hwirqs)
		return SBI_ENOMEM;
	for (i = 0; i < chip->num_hwirq; i++)
		sbi_irqchip_set_raw_handler(chip, i, sbi_irqchip_raw_handler_default);

	SBI_INIT_LIST_HEAD(&chip->handler_list);

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
