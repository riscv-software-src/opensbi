/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Anup Patel <apatel@ventanamicro.com>
 */

#include <sbi/sbi_console.h>
#include <sbi/sbi_heap.h>
#include <sbi/sbi_hsm.h>
#include <sbi/sbi_irqchip.h>
#include <sbi/sbi_list.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_scratch.h>

/** Internal irqchip hardware interrupt data */
struct sbi_irqchip_hwirq_data {
	/** raw hardware interrupt handler */
	int (*raw_handler)(struct sbi_irqchip_device *chip, u32 hwirq);

	/** target hart index */
	u32 hart_index;
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
	int rc = SBI_OK;

	if (!chip || chip->num_hwirq <= hwirq)
		return SBI_EINVAL;

	h = sbi_irqchip_find_handler(chip, hwirq);
	if (h->callback)
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

int sbi_irqchip_get_affinity(struct sbi_irqchip_device *chip, u32 hwirq,
			     u32 *out_hart_index)
{
	if (!chip || chip->num_hwirq <= hwirq)
		return SBI_EINVAL;

	/*
	 * If no handler registered for hwirq then hwirq
	 * is not being used so return failure
	 */
	if (!sbi_irqchip_find_handler(chip, hwirq))
		return SBI_ENOTSUPP;

	*out_hart_index = chip->hwirqs[hwirq].hart_index;
	return 0;
}

int sbi_irqchip_set_affinity(struct sbi_irqchip_device *chip, u32 hwirq,
			     u32 hart_index)
{
	struct sbi_irqchip_hwirq_data *data;
	int rc;

	if (!chip || chip->num_hwirq <= hwirq || sbi_hart_count() <= hart_index)
		return SBI_EINVAL;

	/*
	 * If no handler registered for hwirq then hwirq
	 * is not being used so return failure
	 */
	if (!sbi_irqchip_find_handler(chip, hwirq))
		return SBI_ENOTSUPP;

	data = &chip->hwirqs[hwirq];
	if (data->hart_index != hart_index) {
		if (chip->hwirq_set_affinity) {
			rc = chip->hwirq_set_affinity(chip, hwirq, hart_index);
			if (rc)
				return rc;
		}
		data->hart_index = hart_index;
	}

	return 0;
}

static int __sbi_irqchip_handler_set_affinity(struct sbi_irqchip_device *chip,
					      struct sbi_irqchip_handler *h,
					      u32 compare_hart_index,
					      u32 hart_index)
{
	u32 i, current_hart_index;
	int rc;

	for (i = 0; i < h->num_hwirq; i++) {
		rc = sbi_irqchip_get_affinity(chip, h->first_hwirq + i,
					      &current_hart_index);
		if (rc)
			return rc;

		if (compare_hart_index != -1U &&
		    current_hart_index != compare_hart_index)
			continue;

		rc = sbi_irqchip_set_affinity(chip, h->first_hwirq + i, hart_index);
		if (rc)
			return rc;
	}

	return 0;
}

static int __sbi_irqchip_register_handler(struct sbi_irqchip_device *chip,
					  u32 first_hwirq, u32 num_hwirq, u32 hwirq_flags,
					  int (*callback)(u32 hwirq, void *priv), void *priv)
{
	struct sbi_irqchip_handler *h, *th, *nh;
	u32 i, j;
	int rc;

	for (i = first_hwirq; i < (first_hwirq + num_hwirq); i++) {
		h = sbi_irqchip_find_handler(chip, i);
		if (h)
			return SBI_EALREADY;
	}

	h = sbi_zalloc(sizeof(*h));
	if (!h)
		return SBI_ENOMEM;
	h->first_hwirq = first_hwirq;
	h->num_hwirq = num_hwirq;
	h->callback = callback;
	h->priv = priv;

	nh = NULL;
	sbi_list_for_each_entry(th, &chip->handler_list, node) {
		if (h->first_hwirq < th->first_hwirq) {
			nh = th;
			break;
		}
	}
	if (nh)
		sbi_list_add(&h->node, &nh->node);
	else
		sbi_list_add_tail(&h->node, &chip->handler_list);

	if (chip->hwirq_setup) {
		for (i = 0; i < h->num_hwirq; i++) {
			rc = chip->hwirq_setup(chip, h->first_hwirq + i, hwirq_flags);
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

	rc = __sbi_irqchip_handler_set_affinity(chip, h, -1U, current_hartindex());
	if (rc) {
		if (chip->hwirq_cleanup) {
			for (i = 0; i < h->num_hwirq; i++)
				chip->hwirq_cleanup(chip, h->first_hwirq + i);
		}
		sbi_list_del(&h->node);
		sbi_free(h);
		return rc;
	}

	if (chip->hwirq_unmask) {
		for (i = 0; i < h->num_hwirq; i++)
			chip->hwirq_unmask(chip, h->first_hwirq + i);
	}

	return 0;
}

int sbi_irqchip_register_handler(struct sbi_irqchip_device *chip,
				 u32 first_hwirq, u32 num_hwirq, u32 hwirq_flags,
				 int (*callback)(u32 hwirq, void *priv), void *priv)
{
	if (!chip || !num_hwirq || !callback)
		return SBI_EINVAL;
	if (chip->num_hwirq <= first_hwirq ||
	    chip->num_hwirq <= (first_hwirq + num_hwirq - 1))
		return SBI_EBAD_RANGE;

	return __sbi_irqchip_register_handler(chip, first_hwirq, num_hwirq, hwirq_flags,
					      callback, priv);
}

int sbi_irqchip_register_reserved(struct sbi_irqchip_device *chip,
				  u32 first_hwirq, u32 num_hwirq)
{
	if (!chip || !num_hwirq)
		return SBI_EINVAL;
	if (chip->num_hwirq <= first_hwirq ||
	    chip->num_hwirq <= (first_hwirq + num_hwirq - 1))
		return SBI_EBAD_RANGE;

	return __sbi_irqchip_register_handler(chip, first_hwirq, num_hwirq,
					      SBI_HWIRQ_FLAGS_NONE, NULL, NULL);
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
	for (i = 0; i < chip->num_hwirq; i++) {
		sbi_irqchip_set_raw_handler(chip, i, sbi_irqchip_raw_handler_default);
		chip->hwirqs[i].hart_index = -1U;
	}

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
	struct sbi_irqchip_device *chip;
	struct sbi_irqchip_handler *h;
	u32 migrate_hidx = -1U;
	bool migrate = false;
	int rc;

	sbi_for_each_hartindex(i) {
		if (i == current_hartindex())
			continue;
		if (__sbi_hsm_hart_get_state(i) == SBI_HSM_STATE_STOPPED ||
		    __sbi_hsm_hart_get_state(i) == SBI_HSM_STATE_STOP_PENDING)
			continue;
		migrate_hidx = i;
		migrate = true;
		break;
	}

	if (!migrate)
		goto skip_migrate;
	sbi_list_for_each_entry(chip, &irqchip_list, node) {
		sbi_list_for_each_entry(h, &chip->handler_list, node) {
			rc = __sbi_irqchip_handler_set_affinity(chip, h,
								current_hartindex(),
								migrate_hidx);
			if (rc) {
				sbi_printf("%s: chip 0x%x handler 0x%x set affinity (err %d)\n",
					   __func__, chip->id, h->first_hwirq, rc);
			}
		}
	}
skip_migrate:

	hd = sbi_scratch_thishart_offset_ptr(irqchip_hart_data_off);
	if (hd && hd->chip && hd->chip->process_hwirqs)
		csr_clear(CSR_MIE, MIP_MEIP);
}
