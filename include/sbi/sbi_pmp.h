/*
 * SPDX-FileCopyrightText: (c) 2025-2026 Tenstorrent USA, Inc.
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef __SBI_PMP_H__
#define __SBI_PMP_H__

#include <sbi/sbi_types.h>

struct pmp {
	unsigned long addr;
	u8 cfg;
};
typedef struct pmp pmp_t;

int sbi_pmp_encode(pmp_t *pmp, unsigned long prot, unsigned long addr,
		    unsigned long log2len);
int sbi_pmp_decode(pmp_t *pmp, unsigned long *prot, unsigned long *addr,
		    unsigned long *log2len);

#endif
