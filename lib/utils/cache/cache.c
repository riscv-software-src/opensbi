/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 SiFive Inc.
 */

#include <sbi/sbi_error.h>
#include <sbi_utils/cache/cache.h>

static SBI_LIST_HEAD(cache_list);

struct cache_device *cache_find(u32 id)
{
	struct cache_device *dev;

	sbi_list_for_each_entry(dev, &cache_list, node) {
		if (dev->id == id)
			return dev;
	}

	return NULL;
}

int cache_add(struct cache_device *dev)
{
	if (!dev)
		return SBI_ENODEV;

	if (cache_find(dev->id))
		return SBI_EALREADY;

	sbi_list_add(&dev->node, &cache_list);

	return SBI_OK;
}

int cache_flush_all(struct cache_device *dev)
{
	if (!dev)
		return SBI_ENODEV;

	if (!dev->ops || !dev->ops->cache_flush_all)
		return SBI_ENOTSUPP;

	return dev->ops->cache_flush_all(dev);
}
