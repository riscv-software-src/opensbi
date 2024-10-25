/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#ifndef __FDT_IPI_H__
#define __FDT_IPI_H__

#include <sbi/sbi_types.h>

#ifdef CONFIG_FDT_IPI

struct fdt_ipi {
	const struct fdt_match *match_table;
	int (*cold_init)(const void *fdt, int nodeoff, const struct fdt_match *match);
};

int fdt_ipi_init(void);

#else

static inline int fdt_ipi_init(void) { return 0; }

#endif

#endif
