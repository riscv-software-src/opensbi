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
#include <sbi_utils/timer/fdt_timer.h>

/* List of FDT timer drivers generated at compile time */
extern struct fdt_timer *fdt_timer_drivers[];
extern unsigned long fdt_timer_drivers_size;

static struct fdt_timer *current_driver = NULL;

void fdt_timer_exit(void)
{
	if (current_driver && current_driver->exit)
		current_driver->exit();
}

static int fdt_timer_warm_init(void)
{
	if (current_driver && current_driver->warm_init)
		return current_driver->warm_init();
	return 0;
}

static int fdt_timer_cold_init(void)
{
	int pos, noff, rc;
	struct fdt_timer *drv;
	const struct fdt_match *match;
	void *fdt = fdt_get_address();

	for (pos = 0; pos < fdt_timer_drivers_size; pos++) {
		drv = fdt_timer_drivers[pos];

		noff = -1;
		while ((noff = fdt_find_match(fdt, noff,
					drv->match_table, &match)) >= 0) {
			/* drv->cold_init must not be NULL */
			if (drv->cold_init == NULL)
				return SBI_EFAIL;

			rc = drv->cold_init(fdt, noff, match);
			if (rc == SBI_ENODEV)
				continue;
			if (rc)
				return rc;
			current_driver = drv;

			/*
			 * We will have multiple timer devices on multi-die or
			 * multi-socket systems so we cannot break here.
			 */
		}
	}

	/*
	 * We can't fail here since systems with Sstc might not provide
	 * mtimer/clint DT node in the device tree.
	 */
	return 0;
}

int fdt_timer_init(bool cold_boot)
{
	int rc;

	if (cold_boot) {
		rc = fdt_timer_cold_init();
		if (rc)
			return rc;
	}

	return fdt_timer_warm_init();
}
