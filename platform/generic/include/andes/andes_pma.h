/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2023 Renesas Electronics Corp.
 */

#ifndef _ANDES45_PMA_H_
#define _ANDES45_PMA_H_

#include <sbi/sbi_types.h>

#define ANDES45_MAX_PMA_REGIONS			16

/* Naturally aligned power of 2 region */
#define ANDES45_PMACFG_ETYP_NAPOT		3

/* Memory, Non-cacheable, Bufferable */
#define ANDES45_PMACFG_MTYP_MEM_NON_CACHE_BUF	(3 << 2)

/**
 * struct andes45_pma_region - Describes PMA regions
 *
 * @pa: Address to be configured in the PMA
 * @size: Size of the region
 * @flags: Flags to be set for the PMA region
 * @dt_populate: Boolean flag indicating if the DT entry should be
 *               populated for the given PMA region
 * @shared_dma: Boolean flag if set "shared-dma-pool" property will
 *              be set in the DT node
 * @no_map: Boolean flag if set "no-map" property will be set in the
 *          DT node
 * @dma_default: Boolean flag if set "linux,dma-default" property will
 *              be set in the DT node. Note Linux expects single node
 *              with this property set.
 */
struct andes45_pma_region {
	unsigned long pa;
	unsigned long size;
	u8 flags:7;
	bool dt_populate;
	bool shared_dma;
	bool no_map;
	bool dma_default;
};

int andes45_pma_setup_regions(const struct andes45_pma_region *pma_regions,
			      unsigned int pma_regions_count);

#endif /* _ANDES45_PMA_H_ */
