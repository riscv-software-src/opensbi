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

/* Opaque declaration of heap control struct */
struct heap_control;

/* Alignment of heap base address and size */
#define HEAP_BASE_ALIGN			1024

struct sbi_scratch;

/** Allocate from heap area */
void *sbi_malloc(size_t size);
void *sbi_malloc_from(struct heap_control *hpctrl, size_t size);

/** Zero allocate from heap area */
void *sbi_zalloc(size_t size);
void *sbi_zalloc_from(struct heap_control *hpctrl, size_t size);

/** Allocate array from heap area */
static inline void *sbi_calloc(size_t nitems, size_t size)
{
	return sbi_zalloc(nitems * size);
}

static inline void *sbi_calloc_from(struct heap_control *hpctrl,
				    size_t nitems, size_t size)
{
	return sbi_zalloc_from(hpctrl, nitems * size);
}

/** Free-up to heap area */
void sbi_free(void *ptr);
void sbi_free_from(struct heap_control *hpctrl, void *ptr);

/** Amount (in bytes) of free space in the heap area */
unsigned long sbi_heap_free_space(void);
unsigned long sbi_heap_free_space_from(struct heap_control *hpctrl);

/** Amount (in bytes) of used space in the heap area */
unsigned long sbi_heap_used_space(void);
unsigned long sbi_heap_used_space_from(struct heap_control *hpctrl);

/** Amount (in bytes) of reserved space in the heap area */
unsigned long sbi_heap_reserved_space(void);
unsigned long sbi_heap_reserved_space_from(struct heap_control *hpctrl);

/** Initialize heap area */
int sbi_heap_init(struct sbi_scratch *scratch);
int sbi_heap_init_new(struct heap_control *hpctrl, unsigned long base,
		       unsigned long size);
int sbi_heap_alloc_new(struct heap_control **hpctrl);

#endif
