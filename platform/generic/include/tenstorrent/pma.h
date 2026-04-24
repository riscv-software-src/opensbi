/*
 * SPDX-FileCopyrightText: (c) 2025-2026 Tenstorrent USA, Inc.
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef __TENSTORRENT_PMA_H__
#define __TENSTORRENT_PMA_H__

/* Max number of PMAs for devices (CPU, IOMMU) for Tenstorrent platforms. */
#define TT_MAX_PMAS	32

u64 tt_pma_get(unsigned int n);
void tt_pma_set(unsigned int n, u64 pma);
bool tt_pma_validate(unsigned int i, u64 pma);
void tt_pma_print(unsigned int i, u64 pma);

#endif
