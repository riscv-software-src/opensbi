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
extern struct fdt_regmap *const fdt_regmap_drivers[];

static int fdt_regmap_init(const void *fdt, int nodeoff)
{
	int pos, rc;
	struct fdt_regmap *drv;
	const struct fdt_match *match;

	/* Try all I2C drivers one-by-one */
	for (pos = 0; fdt_regmap_drivers[pos]; pos++) {
		drv = fdt_regmap_drivers[pos];
		match = fdt_match_node(fdt, nodeoff, drv->match_table);
		if (match && drv->init) {
			rc = drv->init(fdt, nodeoff, match);
			if (rc == SBI_ENODEV)
				continue;
			if (rc)
				return rc;
			return 0;
		}
	}

	return SBI_ENOSYS;
}

static int fdt_regmap_find(const void *fdt, int nodeoff,
			   struct regmap **out_rmap)
{
	int rc;
	struct regmap *rmap = regmap_find(nodeoff);

	if (!rmap) {
		/* Regmap not found so initialize matching driver */
		rc = fdt_regmap_init(fdt, nodeoff);
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
