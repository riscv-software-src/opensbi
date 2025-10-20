/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 SiFive Inc.
 */

#ifndef __FDT_CMO_HELPER_H__
#define __FDT_CMO_HELPER_H__

#ifdef CONFIG_FDT_CACHE
/**
 * Flush the private first level cache of the current hart
 *
 * @return 0 on success, or a negative error code on failure
 */
int fdt_cmo_private_flc_flush_all(void);

/**
 * Flush the last level cache of the current hart
 *
 * @return 0 on success, or a negative error code on failure
 */
int fdt_cmo_llc_flush_all(void);

/**
 * Initialize the cache devices for each hart
 *
 * @param fdt devicetree blob
 * @param cold_boot cold init or warm init
 *
 * @return 0 on success, or a negative error code on failure
 */
int fdt_cmo_init(bool cold_boot);

#else

static inline int fdt_cmo_init(bool cold_boot) { return 0; }

#endif /* CONFIG_FDT_CACHE */
#endif /* __FDT_CMO_HELPER_H__ */
