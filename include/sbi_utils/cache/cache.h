/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 SiFive Inc.
 */

#ifndef __CACHE_H__
#define __CACHE_H__

#include <sbi/sbi_list.h>
#include <sbi/sbi_types.h>

#define CACHE_NAME_LEN	32

struct cache_device;

struct cache_ops {
	/** Warm init **/
	int (*warm_init)(struct cache_device *dev);
	/** Flush entire cache **/
	int (*cache_flush_all)(struct cache_device *dev);
};

struct cache_device {
	/** Name of the device **/
	char name[CACHE_NAME_LEN];
	/** List node for search **/
	struct sbi_dlist node;
	/** Point to the next level cache **/
	struct cache_device *next;
	/** Cache Management Operations **/
	struct cache_ops *ops;
	/** CPU private cache **/
	bool cpu_private;
	/** The unique id of this cache device **/
	u32 id;
};

/**
 * Find a registered cache device
 *
 * @param id unique ID of the cache device
 *
 * @return the cache device or NULL
 */
struct cache_device *cache_find(u32 id);

/**
 * Register a cache device
 *
 * cache_device->id must be initialized already and must not change during the life
 * of the cache_device object.
 *
 * @param dev the cache device to register
 *
 * @return 0 on success, or a negative error code on failure
 */
int cache_add(struct cache_device *dev);

/**
 * Flush the entire cache
 *
 * @param dev the cache to flush
 *
 * @return 0 on success, or a negative error code on failure
 */
int cache_flush_all(struct cache_device *dev);

#endif
