/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Anup Patel<apatel@ventanamicro.com>
 */

#ifndef __SBI_HEAP_H__
#define __SBI_HEAP_H__

#include <sbi/sbi_types.h>

struct sbi_scratch;

/** Allocate from heap area */
void *sbi_malloc(size_t size);

/** Zero allocate from heap area */
void *sbi_zalloc(size_t size);

/** Allocate array from heap area */
static inline void *sbi_calloc(size_t nitems, size_t size)
{
	return sbi_zalloc(nitems * size);
}

/** Free-up to heap area */
void sbi_free(void *ptr);

/** Amount (in bytes) of free space in the heap area */
unsigned long sbi_heap_free_space(void);

/** Amount (in bytes) of used space in the heap area */
unsigned long sbi_heap_used_space(void);

/** Amount (in bytes) of reserved space in the heap area */
unsigned long sbi_heap_reserved_space(void);

/** Initialize heap area */
int sbi_heap_init(struct sbi_scratch *scratch);

#endif
