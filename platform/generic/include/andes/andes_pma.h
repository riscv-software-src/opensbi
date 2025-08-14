/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2023 Renesas Electronics Corp.
 */

#ifndef _ANDES_PMA_H_
#define _ANDES_PMA_H_

#include <sbi/sbi_types.h>

#define ANDES_MAX_PMA_REGIONS			16

#define ANDES_PMA_GRANULARITY			(1 << 12)

#define ANDES_PMACFG_ETYP_OFFSET		0
#define ANDES_PMACFG_ETYP_MASK			(3 << ANDES_PMACFG_ETYP_OFFSET)
#define ANDES_PMACFG_ETYP_OFF			(0 << ANDES_PMACFG_ETYP_OFFSET)
/* Naturally aligned power of 2 region */
#define ANDES_PMACFG_ETYP_NAPOT			(3 << ANDES_PMACFG_ETYP_OFFSET)

#define ANDES_PMACFG_MTYP_OFFSET		2
#define ANDES_PMACFG_MTYP_DEV_NOBUF		(0 << ANDES_PMACFG_MTYP_OFFSET)
/* Memory, Non-cacheable, Bufferable */
#define ANDES_PMACFG_MTYP_MEM_NON_CACHE_BUF	(3 << ANDES_PMACFG_MTYP_OFFSET)

/**
 * struct andes_pma_region - Describes PMA regions
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
struct andes_pma_region {
	unsigned long pa;
	unsigned long size;
	u8 flags:7;
	bool dt_populate;
	bool shared_dma;
	bool no_map;
	bool dma_default;
};

int andes_pma_setup_regions(void *fdt,
			    const struct andes_pma_region *pma_regions,
			    unsigned int pma_regions_count);

/**
 * Programmable PMA(PPMA) is a feature for Andes. PPMA allows dynamic adjustment
 * of memory attributes in the runtime. It contains a configurable amount of PMA
 * entries implemented as CSRs to control the attributes of memory locations.
 *
 * Check if hardware supports PPMA
 *
 * @return true if PPMA is supported
 * @return false if PPMA is not supported
 */
bool andes_sbi_probe_pma(void);

/**
 * Set a NAPOT region with given memory attributes
 * @param pa: Start address of the NAPOT region
 * @param size: Size of the NAPOT region
 * @param flags: Memory attributes set to the NAPOT region
 *
 * @return SBI_SUCCESS on success
 * @return SBI_ERR_NOT_SUPPORTED if hardware does not support PPMA features
 * @return SBI_ERR_INVALID_PARAM if the given region is overlapped with the
 *	   region that has been set already
 * @return SBI_ERR_FAILED if available entries have run out or setup fails
 */
int andes_sbi_set_pma(unsigned long pa, unsigned long size, u8 flags);

/**
 * Reset the memory attribute of a NAPOT region
 * @param pa Start address of the NAPOT region
 *
 * @return SBI_SUCCESS on success
 * @return SBI_ERR_FAILED if the given region is not set before
 */
int andes_sbi_free_pma(unsigned long pa);

#endif /* _ANDES_PMA_H_ */
