/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024 Ventana Micro Systems Inc.
 */

#include <sbi/sbi_bitmap.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_heap.h>

static SBI_LIST_HEAD(data_list);
static DECLARE_BITMAP(data_idx_bmap, SBI_DOMAIN_MAX_DATA_PTRS);

void *sbi_domain_data_ptr(struct sbi_domain *dom, struct sbi_domain_data *data)
{
	if (dom && data && data->data_idx < SBI_DOMAIN_MAX_DATA_PTRS)
		return dom->data_priv.idx_to_data_ptr[data->data_idx];

	return NULL;
}

static int domain_setup_data_one(struct sbi_domain *dom,
				 struct sbi_domain_data *data)
{
	struct sbi_domain_data_priv *priv = &dom->data_priv;
	void *data_ptr;
	int rc;

	if (priv->idx_to_data_ptr[data->data_idx])
		return SBI_EALREADY;

	data_ptr = sbi_zalloc(data->data_size);
	if (!data_ptr) {
		sbi_domain_cleanup_data(dom);
		return SBI_ENOMEM;
	}

	if (data->data_setup) {
		rc = data->data_setup(dom, data, data_ptr);
		if (rc) {
			sbi_free(data_ptr);
			return rc;
		}
	}

	priv->idx_to_data_ptr[data->data_idx] = data_ptr;
	return 0;
}

static void domain_cleanup_data_one(struct sbi_domain *dom,
				    struct sbi_domain_data *data)
{
	struct sbi_domain_data_priv *priv = &dom->data_priv;
	void *data_ptr;

	data_ptr = priv->idx_to_data_ptr[data->data_idx];
	if (!data_ptr)
		return;

	if (data->data_cleanup)
		data->data_cleanup(dom, data, data_ptr);

	sbi_free(data_ptr);
	priv->idx_to_data_ptr[data->data_idx] = NULL;
}

int sbi_domain_setup_data(struct sbi_domain *dom)
{
	struct sbi_domain_data *data;
	int rc;

	if (!dom)
		return SBI_EINVAL;

	sbi_list_for_each_entry(data, &data_list, head) {
		rc = domain_setup_data_one(dom, data);
		if (rc) {
			sbi_domain_cleanup_data(dom);
			return rc;
		}
	}

	return 0;
}

void sbi_domain_cleanup_data(struct sbi_domain *dom)
{
	struct sbi_domain_data *data;

	if (!dom)
		return;

	sbi_list_for_each_entry(data, &data_list, head)
		domain_cleanup_data_one(dom, data);
}

int sbi_domain_register_data(struct sbi_domain_data *data)
{
	struct sbi_domain *dom;
	u32 data_idx;
	int rc;

	if (!data || !data->data_size)
		return SBI_EINVAL;

	for (data_idx = 0; data_idx < SBI_DOMAIN_MAX_DATA_PTRS; data_idx++) {
		if (!bitmap_test(data_idx_bmap, data_idx))
			break;
	}
	if (SBI_DOMAIN_MAX_DATA_PTRS <= data_idx)
		return SBI_ENOSPC;
	bitmap_set(data_idx_bmap, data_idx, 1);

	data->data_idx = data_idx;
	sbi_list_add_tail(&data->head, &data_list);

	sbi_domain_for_each(dom) {
		rc = domain_setup_data_one(dom, data);
		if (rc) {
			sbi_domain_unregister_data(data);
			return rc;
		}
	}

	return 0;
}

void sbi_domain_unregister_data(struct sbi_domain_data *data)
{
	struct sbi_domain *dom;

	sbi_domain_for_each(dom)
		domain_cleanup_data_one(dom, data);

	sbi_list_del(&data->head);
	bitmap_clear(data_idx_bmap, data->data_idx, 1);
}
