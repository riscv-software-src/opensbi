/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 Ventana Micro Systems Inc.
 */

#ifndef __SBI_HART_PMP_H__
#define __SBI_HART_PMP_H__

#include <sbi/sbi_types.h>

/* Disable pmp entry at a given index */
int sbi_hart_pmp_disable(unsigned int n);

/* Check if the matching field is set */
bool sbi_hart_is_pmp_enabled(unsigned int n);

int sbi_hart_pmp_set(unsigned int n, unsigned long prot,
		     unsigned long addr, unsigned long log2len);

int sbi_hart_pmp_get(unsigned int n, unsigned long *prot_out,
		     unsigned long *addr_out, unsigned long *log2len);

struct sbi_scratch;

unsigned int sbi_hart_pmp_count(struct sbi_scratch *scratch);
unsigned int sbi_hart_pmp_log2gran(struct sbi_scratch *scratch);
unsigned int sbi_hart_pmp_addrbits(struct sbi_scratch *scratch);
bool sbi_hart_smepmp_is_fw_region(unsigned int pmp_idx);
void sbi_hart_pmp_fence(void);
int sbi_hart_pmp_init(struct sbi_scratch *scratch);

#endif
