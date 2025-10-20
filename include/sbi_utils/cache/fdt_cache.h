/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 SiFive Inc.
 */

#ifndef __FDT_CACHE_H__
#define __FDT_CACHE_H__

#include <sbi_utils/cache/cache.h>

/**
 * Register a cache device using information from the DT
 *
 * @param fdt devicetree blob
 * @param noff offset of a node in the devicetree blob
 * @param dev cache device to register for this devicetree node
 *
 * @return 0 on success, or a negative error code on failure
 */
int fdt_cache_add(const void *fdt, int noff, struct cache_device *dev);

/**
 * Get the cache device referencd by the "next-level-cache" property of a DT node
 *
 * @param fdt devicetree blob
 * @param noff offset of a node in the devicetree blob
 * @param out_dev location to return the cache device
 *
 * @return 0 on success, or a negative error code on failure
 */
int fdt_next_cache_get(const void *fdt, int noff, struct cache_device **out_dev);

#endif
