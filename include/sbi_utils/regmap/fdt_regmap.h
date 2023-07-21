/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Anup Patel <apatel@ventanamicro.com>
 */

#ifndef __FDT_REGMAP_H__
#define __FDT_REGMAP_H__

#include <sbi_utils/regmap/regmap.h>

struct fdt_phandle_args;

/** FDT based regmap driver */
struct fdt_regmap {
	const struct fdt_match *match_table;
	int (*init)(void *fdt, int nodeoff, u32 phandle,
		    const struct fdt_match *match);
};

/** Get regmap instance based on phandle */
int fdt_regmap_get_by_phandle(void *fdt, u32 phandle,
			      struct regmap **out_rmap);

/** Get regmap instance based on "regmap' property of the specified DT node */
int fdt_regmap_get(void *fdt, int nodeoff, struct regmap **out_rmap);

#endif
