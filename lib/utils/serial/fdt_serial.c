/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <libfdt.h>
#include <sbi/sbi_error.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/serial/fdt_serial.h>

/* List of FDT serial drivers generated at compile time */
extern const struct fdt_driver *const fdt_serial_drivers[];

int fdt_serial_init(const void *fdt)
{
	const void *prop;
	int noff = -1, len, coff, rc;

	/* Find offset of node pointed to by stdout-path */
	coff = fdt_path_offset(fdt, "/chosen");
	if (-1 < coff) {
		prop = fdt_getprop(fdt, coff, "stdout-path", &len);
		if (prop && len) {
			const char *sep, *start = prop;

			/* The device path may be followed by ':' */
			sep = strchr(start, ':');
			if (sep)
				noff = fdt_path_offset_namelen(fdt, prop,
							       sep - start);
			else
				noff = fdt_path_offset(fdt, prop);
		}
	}

	/* First check DT node pointed by stdout-path */
	if (-1 < noff) {
		rc = fdt_driver_init_by_offset(fdt, noff, fdt_serial_drivers);
		if (rc != SBI_ENODEV)
			return rc;
	}

	/* Lastly check all DT nodes */
	return fdt_driver_init_one(fdt, fdt_serial_drivers);
}
