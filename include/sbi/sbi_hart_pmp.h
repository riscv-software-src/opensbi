/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 Ventana Micro Systems Inc.
 */

#ifndef __SBI_HART_PMP_H__
#define __SBI_HART_PMP_H__

#include <sbi/sbi_types.h>

struct sbi_scratch;

unsigned int sbi_hart_pmp_count(struct sbi_scratch *scratch);
unsigned int sbi_hart_pmp_log2gran(struct sbi_scratch *scratch);
unsigned int sbi_hart_pmp_addrbits(struct sbi_scratch *scratch);
bool sbi_hart_smepmp_is_fw_region(unsigned int pmp_idx);
void sbi_hart_pmp_fence(void);
int sbi_hart_pmp_init(struct sbi_scratch *scratch);

#endif
