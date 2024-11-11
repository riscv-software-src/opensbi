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

#include <sbi_utils/fdt/fdt_driver.h>
#include <sbi_utils/regmap/regmap.h>

struct fdt_phandle_args;

/** Get regmap instance based on phandle */
int fdt_regmap_get_by_phandle(const void *fdt, u32 phandle,
			      struct regmap **out_rmap);

/** Get regmap instance based on "regmap" property of the specified DT node */
int fdt_regmap_get(const void *fdt, int nodeoff, struct regmap **out_rmap);

#endif
