/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024 Rivos Inc.
 *
 * Authors:
 *   Clément Léger <cleger@rivosinc.com>
 */

#ifndef __SBI_FW_FEATURE_H__
#define __SBI_FW_FEATURE_H__

#include <sbi/sbi_ecall_interface.h>

struct sbi_scratch;

int sbi_fwft_set(enum sbi_fwft_feature_t feature, unsigned long value,
		 unsigned long flags);
int sbi_fwft_get(enum sbi_fwft_feature_t feature, unsigned long *out_val);

int sbi_fwft_init(struct sbi_scratch *scratch, bool cold_boot);

#endif
