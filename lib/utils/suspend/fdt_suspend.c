/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Anup Patel <apatel@ventanamicro.com>
 */

#include <sbi_utils/suspend/fdt_suspend.h>

/* List of FDT suspend drivers generated at compile time */
extern const struct fdt_driver *const fdt_suspend_drivers[];

void fdt_suspend_init(const void *fdt)
{
	/*
	 * Platforms might have multiple system suspend devices or
	 * might not have any so probe all and don't fail.
	 */
	fdt_driver_init_all(fdt, fdt_suspend_drivers);
}
