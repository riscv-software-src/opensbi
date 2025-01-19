/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * fdt_driver.h - Generic support for initializing drivers from DT nodes.
 *
 * Copyright (c) 2024 SiFive
 */

#ifndef __FDT_DRIVER_H__
#define __FDT_DRIVER_H__

#include <sbi_utils/fdt/fdt_helper.h>

struct fdt_driver {
	const struct fdt_match *match_table;
	int (*init)(const void *fdt, int nodeoff,
		    const struct fdt_match *match);
	bool experimental;
};

/* List of early FDT drivers generated at compile time */
extern const struct fdt_driver *const fdt_early_drivers[];

/**
 * Initialize a driver instance for a specific DT node
 *
 * @param fdt devicetree blob
 * @param nodeoff offset of a node in the devicetree blob
 * @param drivers NULL-terminated array of drivers to match against this node
 *
 * @return 0 if a driver was matched and successfully initialized or a negative
 * error code on failure
 */
int fdt_driver_init_by_offset(const void *fdt, int nodeoff,
			      const struct fdt_driver *const *drivers);

/**
 * Initialize a driver instance for each DT node that matches any of the
 * provided drivers
 *
 * @param fdt devicetree blob
 * @param drivers NULL-terminated array of drivers to match against each node
 *
 * @return 0 if drivers for all matches (if any) were successfully initialized
 * or a negative error code on failure
 */
int fdt_driver_init_all(const void *fdt,
			const struct fdt_driver *const *drivers);

/**
 * Initialize a driver instance for the first DT node that matches any of the
 * provided drivers
 *
 * @param fdt devicetree blob
 * @param drivers NULL-terminated array of drivers to match against each node
 *
 * @return 0 if a driver was matched and successfully initialized or a negative
 * error code on failure
 */
int fdt_driver_init_one(const void *fdt,
			const struct fdt_driver *const *drivers);

#endif /* __FDT_DRIVER_H__ */
