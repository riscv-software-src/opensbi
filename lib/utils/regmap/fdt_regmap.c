/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Anup Patel <apatel@ventanamicro.com>
 */

#include <libfdt.h>
#include <sbi/sbi_error.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/regmap/fdt_regmap.h>

/* List of FDT regmap drivers generated at compile time */
extern const struct fdt_driver *const fdt_regmap_drivers[];

static int fdt_regmap_find(const void *fdt, int nodeoff,
			   struct regmap **out_rmap)
{
	int rc;
	struct regmap *rmap = regmap_find(nodeoff);

	if (!rmap) {
		/* Regmap not found so initialize matching driver */
		rc = fdt_driver_init_by_offset(fdt, nodeoff,
					       fdt_regmap_drivers);
		if (rc)
			return rc;

		/* Try to find regmap again */
		rmap = regmap_find(nodeoff);
		if (!rmap)
			return SBI_ENOSYS;
	}

	if (out_rmap)
		*out_rmap = rmap;

	return 0;
}

int fdt_regmap_get_by_phandle(const void *fdt, u32 phandle,
			      struct regmap **out_rmap)
{
	int pnodeoff;

	if (!fdt || !out_rmap)
		return SBI_EINVAL;

	pnodeoff = fdt_node_offset_by_phandle(fdt, phandle);
	if (pnodeoff < 0)
		return pnodeoff;

	return fdt_regmap_find(fdt, pnodeoff, out_rmap);
}

int fdt_regmap_get(const void *fdt, int nodeoff, struct regmap **out_rmap)
{
	int len;
	const fdt32_t *val;

	if (!fdt || (nodeoff < 0) || !out_rmap)
		return SBI_EINVAL;

	val = fdt_getprop(fdt, nodeoff, "regmap", &len);
	if (!val)
		return SBI_ENOENT;

	return fdt_regmap_get_by_phandle(fdt, fdt32_to_cpu(*val), out_rmap);
}
