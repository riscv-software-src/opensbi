/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Simple heap of the S-mode test payload.
 *
 * The allocations are kept in a singly linked list that is sorted by address.
 * A node records the start and the end of its allocation and sits in the heap
 * directly in front of the memory it describes. A request is served from the
 * first gap that is large enough: before the first allocation, between two
 * allocations or behind the last one. free() unlinks the node, the gap then
 * merges with its neighbours by itself.
 *
 * One spin lock protects the list.
 */

#include <sbi/riscv_locks.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_types.h>
#include "heap.h"
#include "sbitest.h"

struct alloc {
	struct alloc *next;
	/* First byte of the memory that was handed out */
	unsigned long start;
	/* First byte behind the memory that was handed out */
	unsigned long end;
};

_Static_assert(sizeof(struct alloc) <= HEAP_HEADER_SIZE,
	       "struct alloc does not fit into HEAP_HEADER_SIZE");
_Static_assert((HEAP_HEADER_SIZE % HEAP_ALIGN) == 0,
	       "HEAP_HEADER_SIZE is not aligned");

static spinlock_t heap_lock = SPIN_LOCK_INITIALIZER;
/* Sorted by address, the node is at the start of its block */
static struct alloc *heap_list;
static unsigned long heap_begin;
static unsigned long heap_limit;

void heap_init(void *start, unsigned long size)
{
	unsigned long begin = ROUNDUP((unsigned long)start, HEAP_ALIGN);
	unsigned long limit =
		ROUNDDOWN((unsigned long)start + size, HEAP_ALIGN);

	if (limit < begin)
		limit = begin;

	spin_lock(&heap_lock);
	heap_begin = begin;
	heap_limit = limit;
	heap_list  = NULL;
	spin_unlock(&heap_lock);
}

void *malloc(size_t size)
{
	unsigned long gap_start, gap_end, need;
	struct alloc **link, *node, *new;
	void *ret = NULL;

	spin_lock(&heap_lock);

	/* This also rejects sizes where the rounding below could overflow */
	if (!size || size > heap_limit - heap_begin) {
		spin_unlock(&heap_lock);
		return NULL;
	}
	need = HEAP_HEADER_SIZE + ROUNDUP(size, HEAP_ALIGN);

	gap_start = heap_begin;
	for (link = &heap_list;; link = &node->next) {
		node	= *link;
		gap_end = node ? (unsigned long)node : heap_limit;

		if (gap_end - gap_start >= need) {
			new	   = (struct alloc *)gap_start;
			new->start = gap_start + HEAP_HEADER_SIZE;
			new->end   = gap_start + need;
			new->next  = node;
			*link	   = new;
			ret	   = (void *)new->start;
			break;
		}

		if (!node)
			break;
		gap_start = node->end;
	}

	spin_unlock(&heap_lock);

	return ret;
}

void free(void *ptr)
{
	unsigned long start = (unsigned long)ptr;
	struct alloc **link, *node;

	if (!ptr)
		return;

	spin_lock(&heap_lock);
	for (link = &heap_list; (node = *link); link = &node->next) {
		if (node->start == start) {
			*link = node->next;
			spin_unlock(&heap_lock);
			return;
		}
		/* The list is sorted, the pointer cannot be further down */
		if (node->start > start)
			break;
	}
	spin_unlock(&heap_lock);

	sbi_printf("%s(0x%lx): not an allocation\n", __func__, start);
	test_abort("invalid pointer released");
}

unsigned long heap_count(void)
{
	unsigned long count = 0;
	struct alloc *node;

	spin_lock(&heap_lock);
	for (node = heap_list; node; node = node->next)
		count++;
	spin_unlock(&heap_lock);

	return count;
}

unsigned long heap_max_alloc(void)
{
	unsigned long size = heap_limit - heap_begin;

	if (size < HEAP_HEADER_SIZE)
		return 0;

	return ROUNDDOWN(size - HEAP_HEADER_SIZE, HEAP_ALIGN);
}
