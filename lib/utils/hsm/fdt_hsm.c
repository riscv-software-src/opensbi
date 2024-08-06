/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Anup Patel <apatel@ventanamicro.com>
 */

#include <sbi_utils/hsm/fdt_hsm.h>

/* List of FDT HSM drivers generated at compile time */
extern const struct fdt_driver *const fdt_hsm_drivers[];

void fdt_hsm_init(const void *fdt)
{
	/*
	 * Platforms might have multiple HSM devices or might
	 * not have any so probe all and don't fail.
	 */
	fdt_driver_init_all(fdt, fdt_hsm_drivers);
}
