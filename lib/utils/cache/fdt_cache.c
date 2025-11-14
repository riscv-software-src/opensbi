/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 SiFive Inc.
 */

#include <libfdt.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_heap.h>
#include <sbi_utils/cache/fdt_cache.h>
#include <sbi_utils/fdt/fdt_driver.h>

/* List of FDT cache drivers generated at compile time */
extern const struct fdt_driver *const fdt_cache_drivers[];

int fdt_cache_add(const void *fdt, int noff, struct cache_device *dev)
{
	int rc;

	dev->id = noff;
	sbi_strncpy(dev->name, fdt_get_name(fdt, noff, NULL), sizeof(dev->name) - 1);
	sbi_dprintf("%s: %s\n", __func__, dev->name);

	rc = fdt_next_cache_get(fdt, noff, &dev->next);
	if (rc == SBI_ENOENT)
		dev->next = NULL;
	else if (rc)
		return rc;

	return cache_add(dev);
}

static int fdt_cache_add_generic(const void *fdt, int noff)
{
	struct cache_device *dev;
	int rc;

	dev = sbi_zalloc(sizeof(*dev));
	if (!dev)
		return SBI_ENOMEM;

	rc = fdt_cache_add(fdt, noff, dev);
	if (rc) {
		sbi_free(dev);
		return rc;
	}

	return 0;
}

static int fdt_cache_find(const void *fdt, int noff, struct cache_device **out_dev)
{
	struct cache_device *dev = cache_find(noff);
	int rc;

	if (!dev) {
		rc = fdt_driver_init_by_offset(fdt, noff, fdt_cache_drivers);
		if (rc == SBI_ENODEV)
			rc = fdt_cache_add_generic(fdt, noff);
		if (rc)
			return rc;

		dev = cache_find(noff);
		if (!dev)
			return SBI_EFAIL;
	}

	if (out_dev)
		*out_dev = dev;

	return SBI_OK;
}

int fdt_next_cache_get(const void *fdt, int noff, struct cache_device **out_dev)
{
	const fdt32_t *val;
	int len;

	val = fdt_getprop(fdt, noff, "next-level-cache", &len);
	if (!val || len < sizeof(*val))
		return SBI_ENOENT;

	noff = fdt_node_offset_by_phandle(fdt, fdt32_to_cpu(val[0]));
	if (noff < 0)
		return noff;

	return fdt_cache_find(fdt, noff, out_dev);
}
