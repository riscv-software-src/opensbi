/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Simple heap of the S-mode test payload.
 */

#ifndef __SBITEST_HEAP_H__
#define __SBITEST_HEAP_H__

#include <sbi/sbi_types.h>

/* Alignment of all allocations */
#define HEAP_ALIGN 16

/* Memory in front of each allocation for its list node */
#define HEAP_HEADER_SIZE 32

/*
 * Use the memory area [start, start + size) as heap. Call it once, on one
 * hart, before the first malloc(). Memory that was allocated before is
 * forgotten.
 */
void heap_init(void *start, unsigned long size);

/*
 * Allocate size bytes, aligned to HEAP_ALIGN. Return NULL if size is 0 or if
 * there is no gap that is large enough. The memory is not cleared.
 *
 * malloc() and free() can be called from all harts at the same time. Do not
 * call them from the trap handler, it would deadlock if the trap interrupted
 * code that holds the lock of the heap.
 */
void *malloc(size_t size);

/*
 * Release memory that was returned by malloc(). free(NULL) does nothing.
 * Anything else that is not an allocation, for example a pointer that was
 * freed before, ends the test run with an error.
 */
void free(void *ptr);

/* Number of allocations */
unsigned long heap_count(void);

/* The largest size that malloc() can serve on an empty heap */
unsigned long heap_max_alloc(void);

#endif /* __SBITEST_HEAP_H__ */
