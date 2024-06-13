/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <sbi/sbi_console.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_scratch.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/reset/fdt_reset.h>

/* List of FDT reset drivers generated at compile time */
extern struct fdt_reset *fdt_reset_drivers[];
extern unsigned long fdt_reset_drivers_size;

int fdt_reset_driver_init(void *fdt, struct fdt_reset *drv)
{
	int noff, rc, cnt = 0;
	const struct fdt_match *match;

	noff = -1;
	while ((noff = fdt_find_match(fdt, noff,
				drv->match_table, &match)) >= 0) {
		if (!fdt_node_is_enabled(fdt, noff))
			continue;

		if (drv->init) {
			rc = drv->init(fdt, noff, match);
			if (!rc)
				cnt++;
			else if (rc != SBI_ENODEV) {
				sbi_printf("%s: %s init failed, %d\n",
					__func__, match->compatible, rc);
			}
		}
	}

	return cnt > 0 ? 0 : SBI_ENODEV;
}

void fdt_reset_init(void)
{
	int pos;
	void *fdt = fdt_get_address();

	for (pos = 0; pos < fdt_reset_drivers_size; pos++)
		fdt_reset_driver_init(fdt, fdt_reset_drivers[pos]);
}
