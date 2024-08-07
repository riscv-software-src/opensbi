/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Anup Patel <apatel@ventanamicro.com>
 */

#include <sbi_utils/cppc/fdt_cppc.h>

/* List of FDT CPPC drivers generated at compile time */
extern const struct fdt_driver *const fdt_cppc_drivers[];

void fdt_cppc_init(const void *fdt)
{
	/*
	 * Platforms might have multiple CPPC devices or might
	 * not have any so probe all and don't fail.
	 */
	fdt_driver_init_all(fdt, fdt_cppc_drivers);
}
