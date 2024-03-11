/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#ifndef __SBI_TRAP_LDST_H__
#define __SBI_TRAP_LDST_H__

#include <sbi/sbi_types.h>
#include <sbi/sbi_trap.h>

union sbi_ldst_data {
	u64 data_u64;
	u32 data_u32;
	u8 data_bytes[8];
	ulong data_ulong;
};

int sbi_misaligned_load_handler(struct sbi_trap_context *tcntx);

int sbi_misaligned_store_handler(struct sbi_trap_context *tcntx);

int sbi_load_access_handler(struct sbi_trap_context *tcntx);

int sbi_store_access_handler(struct sbi_trap_context *tcntx);

#endif
