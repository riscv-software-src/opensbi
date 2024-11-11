/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <sbi_utils/reset/fdt_reset.h>

/* List of FDT reset drivers generated at compile time */
extern const struct fdt_driver *const fdt_reset_drivers[];

void fdt_reset_init(const void *fdt)
{
	fdt_driver_init_all(fdt, fdt_reset_drivers);
}
