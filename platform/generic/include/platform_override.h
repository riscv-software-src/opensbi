/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#ifndef __PLATFORM_OVERRIDE_H__
#define __PLATFORM_OVERRIDE_H__

#include <sbi/sbi_hart.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_types.h>
#include <sbi_utils/fdt/fdt_driver.h>

bool generic_cold_boot_allowed(u32 hartid);
int generic_nascent_init(void);
int generic_early_init(bool cold_boot);
int generic_final_init(bool cold_boot);
int generic_extensions_init(struct sbi_hart_features *hfeatures);
int generic_domains_init(void);
int generic_pmu_init(void);
uint64_t generic_pmu_xlate_to_mhpmevent(uint32_t event_idx, uint64_t data);
u64 generic_tlbr_flush_limit(void);
u32 generic_tlb_num_entries(void);
int generic_mpxy_init(void);

extern struct sbi_platform_operations generic_platform_ops;

#endif
