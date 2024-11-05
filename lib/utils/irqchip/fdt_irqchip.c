/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <sbi/sbi_error.h>
#include <sbi/sbi_scratch.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/irqchip/fdt_irqchip.h>

/* List of FDT irqchip drivers generated at compile time */
extern struct fdt_irqchip *fdt_irqchip_drivers[];
extern unsigned long fdt_irqchip_drivers_size;

int fdt_irqchip_init(void)
{
	int pos, noff, rc;
	struct fdt_irqchip *drv;
	const struct fdt_match *match;
	const void *fdt = fdt_get_address();

	for (pos = 0; pos < fdt_irqchip_drivers_size; pos++) {
		drv = fdt_irqchip_drivers[pos];

		noff = -1;
		while ((noff = fdt_find_match(fdt, noff,
					drv->match_table, &match)) >= 0) {
			if (!fdt_node_is_enabled(fdt,noff))
				continue;

			if (drv->cold_init) {
				rc = drv->cold_init(fdt, noff, match);
				if (rc == SBI_ENODEV)
					continue;
				if (rc)
					return rc;
			}
		}
	}

	return 0;
}
