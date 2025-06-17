/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Anup Patel<apatel@ventanamicro.com>
 */

#include <sbi/riscv_locks.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_heap.h>
#include <sbi/sbi_list.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_string.h>

/* Minimum size and alignment of heap allocations */
#define HEAP_ALLOC_ALIGN		64

/* Number of heap nodes to allocate at once */
#define HEAP_NODE_BATCH_SIZE		8

struct heap_node {
	struct sbi_dlist head;
	unsigned long addr;
	unsigned long size;
};

struct sbi_heap_control {
	spinlock_t lock;
	unsigned long base;
	unsigned long size;
	unsigned long resv;
	struct sbi_dlist free_node_list;
	struct sbi_dlist free_space_list;
	struct sbi_dlist used_space_list;
	struct heap_node init_free_space_node;
};

struct sbi_heap_control global_hpctrl;

static bool alloc_nodes(struct sbi_heap_control *hpctrl)
{
	size_t size = HEAP_NODE_BATCH_SIZE * sizeof(struct heap_node);
	struct heap_node *n, *new = NULL;

	/* alloc_with_align() requires at most two free nodes */
	if (hpctrl->free_node_list.next != hpctrl->free_node_list.prev)
		return true;

	sbi_list_for_each_entry_reverse(n, &hpctrl->free_space_list, head) {
		if (n->size >= size) {
			n->size -= size;
			if (!n->size) {
				sbi_list_del(&n->head);
				sbi_list_add_tail(&n->head, &hpctrl->free_node_list);
			}
			new = (void *)(n->addr + n->size);
			break;
		}
	}
	if (!new)
		return false;

	for (size_t i = 0; i < HEAP_NODE_BATCH_SIZE; i++)
		sbi_list_add_tail(&new[i].head, &hpctrl->free_node_list);
	hpctrl->resv += size;

	return true;
}

static void *alloc_with_align(struct sbi_heap_control *hpctrl,
			      size_t align, size_t size)
{
	void *ret = NULL;
	struct heap_node *n, *np;
	unsigned long lowest_aligned;
	size_t pad;

	if (!size)
		return NULL;

	size += align - 1;
	size &= ~((unsigned long)align - 1);

	spin_lock(&hpctrl->lock);

	/* Ensure at least two free nodes are available for use below */
	if (!alloc_nodes(hpctrl))
		goto out;

	np = NULL;
	sbi_list_for_each_entry(n, &hpctrl->free_space_list, head) {
		lowest_aligned = ROUNDUP(n->addr, align);
		pad = lowest_aligned - n->addr;

		if (size + pad <= n->size) {
			np = n;
			break;
		}
	}
	if (!np)
		goto out;

	if (pad) {
		n = sbi_list_first_entry(&hpctrl->free_node_list,
					 struct heap_node, head);
		sbi_list_del(&n->head);

		n->addr = np->addr;
		n->size = pad;
		sbi_list_add_tail(&n->head, &np->head);

		np->addr += pad;
		np->size -= pad;
	}

	if (size < np->size) {
		n = sbi_list_first_entry(&hpctrl->free_node_list,
					 struct heap_node, head);
		sbi_list_del(&n->head);

		n->addr = np->addr + size;
		n->size = np->size - size;
		sbi_list_add(&n->head, &np->head);

		np->size = size;
	}

	sbi_list_del(&np->head);
	sbi_list_add_tail(&np->head, &hpctrl->used_space_list);
	ret = (void *)np->addr;

out:
	spin_unlock(&hpctrl->lock);

	return ret;
}

void *sbi_malloc_from(struct sbi_heap_control *hpctrl, size_t size)
{
	return alloc_with_align(hpctrl, HEAP_ALLOC_ALIGN, size);
}

void *sbi_aligned_alloc_from(struct sbi_heap_control *hpctrl,
			     size_t alignment, size_t size)
{
	if (alignment < HEAP_ALLOC_ALIGN)
		alignment = HEAP_ALLOC_ALIGN;

	/* Make sure alignment is power of two */
	if ((alignment & (alignment - 1)) != 0)
		return NULL;

	/* Make sure size is multiple of alignment */
	if (size % alignment != 0)
		return NULL;

	return alloc_with_align(hpctrl, alignment, size);
}

void *sbi_zalloc_from(struct sbi_heap_control *hpctrl, size_t size)
{
	void *ret = sbi_malloc_from(hpctrl, size);

	if (ret)
		sbi_memset(ret, 0, size);
	return ret;
}

void sbi_free_from(struct sbi_heap_control *hpctrl, void *ptr)
{
	struct heap_node *n, *np;

	if (!ptr)
		return;

	spin_lock(&hpctrl->lock);

	np = NULL;
	sbi_list_for_each_entry(n, &hpctrl->used_space_list, head) {
		if ((n->addr <= (unsigned long)ptr) &&
		    ((unsigned long)ptr < (n->addr + n->size))) {
			np = n;
			break;
		}
	}
	if (!np) {
		spin_unlock(&hpctrl->lock);
		return;
	}

	sbi_list_del(&np->head);

	sbi_list_for_each_entry(n, &hpctrl->free_space_list, head) {
		if ((np->addr + np->size) == n->addr) {
			n->addr = np->addr;
			n->size += np->size;
			sbi_list_add_tail(&np->head, &hpctrl->free_node_list);
			np = NULL;
			break;
		} else if (np->addr == (n->addr + n->size)) {
			n->size += np->size;
			sbi_list_add_tail(&np->head, &hpctrl->free_node_list);
			np = NULL;
			break;
		} else if ((n->addr + n->size) < np->addr) {
			sbi_list_add(&np->head, &n->head);
			np = NULL;
			break;
		}
	}
	if (np)
		sbi_list_add_tail(&np->head, &hpctrl->free_space_list);

	spin_unlock(&hpctrl->lock);
}

unsigned long sbi_heap_free_space_from(struct sbi_heap_control *hpctrl)
{
	struct heap_node *n;
	unsigned long ret = 0;

	spin_lock(&hpctrl->lock);
	sbi_list_for_each_entry(n, &hpctrl->free_space_list, head)
		ret += n->size;
	spin_unlock(&hpctrl->lock);

	return ret;
}

unsigned long sbi_heap_used_space_from(struct sbi_heap_control *hpctrl)
{
	return hpctrl->size - hpctrl->resv - sbi_heap_free_space();
}

unsigned long sbi_heap_reserved_space_from(struct sbi_heap_control *hpctrl)
{
	return hpctrl->resv;
}

int sbi_heap_init_new(struct sbi_heap_control *hpctrl, unsigned long base,
		       unsigned long size)
{
	struct heap_node *n;

	/* Initialize heap control */
	SPIN_LOCK_INIT(hpctrl->lock);
	hpctrl->base = base;
	hpctrl->size = size;
	hpctrl->resv = 0;
	SBI_INIT_LIST_HEAD(&hpctrl->free_node_list);
	SBI_INIT_LIST_HEAD(&hpctrl->free_space_list);
	SBI_INIT_LIST_HEAD(&hpctrl->used_space_list);

	/* Prepare free space list */
	n = &hpctrl->init_free_space_node;
	n->addr = base;
	n->size = size;
	sbi_list_add_tail(&n->head, &hpctrl->free_space_list);

	return 0;
}

int sbi_heap_init(struct sbi_scratch *scratch)
{
	/* Sanity checks on heap offset and size */
	if (!scratch->fw_heap_size ||
	    (scratch->fw_heap_size & (HEAP_BASE_ALIGN - 1)) ||
	    (scratch->fw_heap_offset < scratch->fw_rw_offset) ||
	    (scratch->fw_size < (scratch->fw_heap_offset + scratch->fw_heap_size)) ||
	    (scratch->fw_heap_offset & (HEAP_BASE_ALIGN - 1)))
		return SBI_EINVAL;

	return sbi_heap_init_new(&global_hpctrl,
				  scratch->fw_start + scratch->fw_heap_offset,
				  scratch->fw_heap_size);
}

int sbi_heap_alloc_new(struct sbi_heap_control **hpctrl)
{
	*hpctrl = sbi_calloc(1, sizeof(struct sbi_heap_control));
	return 0;
}
