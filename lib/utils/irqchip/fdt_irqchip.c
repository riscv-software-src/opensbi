/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <sbi_utils/irqchip/fdt_irqchip.h>

/* List of FDT irqchip drivers generated at compile time */
extern const struct fdt_driver *const fdt_irqchip_drivers[];

int fdt_irqchip_init(void)
{
	return fdt_driver_init_all(fdt_get_address(), fdt_irqchip_drivers);
}
