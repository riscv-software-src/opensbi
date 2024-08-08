/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Anup Patel <apatel@ventanamicro.com>
 */

#include <sbi_utils/mpxy/fdt_mpxy.h>

/* List of FDT MPXY drivers generated at compile time */
extern const struct fdt_driver *const fdt_mpxy_drivers[];

void fdt_mpxy_init(const void *fdt)
{
	/*
	 * Platforms might have multiple MPXY devices or might not
	 * have any MPXY devices so don't fail.
	 */
	fdt_driver_init_all(fdt, fdt_mpxy_drivers);
}
