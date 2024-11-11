/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <sbi_utils/ipi/fdt_ipi.h>

/* List of FDT ipi drivers generated at compile time */
extern const struct fdt_driver *const fdt_ipi_drivers[];

int fdt_ipi_init(void)
{
	/*
	 * On some single-hart system there is no need for IPIs,
	 * so do not return a failure if no device is found.
	 */
	return fdt_driver_init_all(fdt_get_address(), fdt_ipi_drivers);
}
