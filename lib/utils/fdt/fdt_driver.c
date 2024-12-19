/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024 SiFive
 */

#include <libfdt.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_error.h>
#include <sbi_utils/fdt/fdt_driver.h>
#include <sbi_utils/fdt/fdt_helper.h>

int fdt_driver_init_by_offset(const void *fdt, int nodeoff,
			      const struct fdt_driver *const *drivers)
{
	const struct fdt_driver *driver;
	const struct fdt_match *match;
	const void *prop;
	int len, rc;

	if (!fdt_node_is_enabled(fdt, nodeoff))
		return SBI_ENODEV;

	prop = fdt_getprop(fdt, nodeoff, "compatible", &len);
	if (!prop)
		return SBI_ENODEV;

	while ((driver = *drivers++)) {
		for (match = driver->match_table; match->compatible; match++) {
			if (!fdt_stringlist_contains(prop, len, match->compatible))
				continue;

			if (driver->experimental)
				sbi_printf("WARNING: %s driver is experimental and may change\n",
					   match->compatible);

			rc = driver->init(fdt, nodeoff, match);
			if (rc < 0) {
				const char *name;

				name = fdt_get_name(fdt, nodeoff, NULL);
				sbi_printf("%s: %s (%s) init failed: %d\n",
					   __func__, name, match->compatible, rc);
			}

			return rc;
		}
	}

	return SBI_ENODEV;
}

static int fdt_driver_init_scan(const void *fdt,
				const struct fdt_driver *const *drivers,
				bool one)
{
	int nodeoff, rc;

	for (nodeoff = fdt_next_node(fdt, -1, NULL);
	     nodeoff >= 0;
	     nodeoff = fdt_next_node(fdt, nodeoff, NULL)) {
		rc = fdt_driver_init_by_offset(fdt, nodeoff, drivers);
		if (rc == SBI_ENODEV)
			continue;
		if (rc < 0)
			return rc;
		if (one)
			return 0;
	}

	return one ? SBI_ENODEV : 0;
}

int fdt_driver_init_all(const void *fdt,
			const struct fdt_driver *const *drivers)
{
	return fdt_driver_init_scan(fdt, drivers, false);
}

int fdt_driver_init_one(const void *fdt,
			const struct fdt_driver *const *drivers)
{
	return fdt_driver_init_scan(fdt, drivers, true);
}
