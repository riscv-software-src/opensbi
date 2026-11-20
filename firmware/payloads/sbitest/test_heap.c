/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Tests of the heap of the S-mode test payload.
 */

#include <sbi/sbi_console.h>
#include <sbi/sbi_string.h>
#include "heap.h"
#include "sbitest.h"

#define CHECK(cond)                                                          \
	do {                                                                 \
		if (!(cond)) {                                               \
			sbi_printf("FAILED %s:%d: %s\n", __func__, __LINE__, \
				   #cond);                                   \
			failed++;                                            \
		}                                                            \
	} while (0)

static int is_aligned(const void *p)
{
	return ((unsigned long)p & (HEAP_ALIGN - 1)) == 0;
}

/* Is every byte of the block equal to val? */
static int block_is(const unsigned char *p, unsigned long size,
		    unsigned char val)
{
	unsigned long i;

	for (i = 0; i < size; i++)
		if (p[i] != val)
			return 0;

	return 1;
}

unsigned int test_heap_basic(void)
{
	unsigned long max = heap_max_alloc();
	unsigned char *a, *b, *c, *d;
	unsigned int failed = 0;

	/* The heap is empty and nothing fits into nothing */
	CHECK(heap_count() == 0);
	CHECK(max > 0);

	a = malloc(100);
	b = malloc(200);
	c = malloc(300);
	CHECK(a && b && c);
	if (!a || !b || !c)
		return failed;

	CHECK(is_aligned(a) && is_aligned(b) && is_aligned(c));
	CHECK(heap_count() == 3);
	/* An empty heap is used from the bottom, without overlap */
	CHECK(a + 100 <= b && b + 200 <= c);

	sbi_memset(a, 0xa1, 100);
	sbi_memset(b, 0xb2, 200);
	sbi_memset(c, 0xc3, 300);
	CHECK(block_is(a, 100, 0xa1));
	CHECK(block_is(b, 200, 0xb2));
	CHECK(block_is(c, 300, 0xc3));

	/* The first gap that is large enough is reused */
	free(b);
	CHECK(heap_count() == 2);
	d = malloc(150);
	CHECK(d == b);
	sbi_memset(d, 0xd4, 150);
	CHECK(block_is(a, 100, 0xa1));
	CHECK(block_is(c, 300, 0xc3));

	free(a);
	free(c);
	free(d);
	CHECK(heap_count() == 0);

	/* Everything is merged again: one block takes the whole heap */
	a = malloc(max);
	CHECK(a);
	CHECK(!malloc(1));
	free(a);

	/* Sizes that cannot be served */
	CHECK(!malloc(0));
	CHECK(!malloc(max + 1));
	CHECK(!malloc((size_t)-1));
	CHECK(heap_count() == 0);

	/* Free two neighbours: their gap, with both headers, takes one block */
	a = malloc(64);
	b = malloc(64);
	c = malloc(64);
	CHECK(a && b && c);
	free(a);
	free(b);
	d = malloc(64 + HEAP_HEADER_SIZE + 64);
	CHECK(d == a);
	free(d);
	free(c);
	CHECK(heap_count() == 0);

	return failed;
}

/*
 * Every hart allocates and frees blocks of varying size, fills them with a
 * pattern of its own and checks the pattern before it frees the block. Blocks
 * of different harts that overlap, or a damaged list, show up as errors.
 */
unsigned int test_heap_stress(unsigned long hartid)
{
	enum { SLOTS = 4, ROUNDS = 400 };
	unsigned char pattern[SLOTS] = { 0 };
	unsigned char *p[SLOTS]	     = { NULL };
	unsigned long size[SLOTS]    = { 0 };
	unsigned int failed	     = 0;
	unsigned long i, slot;

	/* The last SLOTS rounds only free what is left */
	for (i = 0; i < ROUNDS + SLOTS; i++) {
		slot = i % SLOTS;

		if (p[slot]) {
			if (!block_is(p[slot], size[slot], pattern[slot]))
				failed++;
			free(p[slot]);
			p[slot] = NULL;
		}
		if (i >= ROUNDS)
			continue;

		size[slot] = 16 + (i * 37 + hartid * 11) % 240;
		p[slot]	   = malloc(size[slot]);
		if (!p[slot]) {
			failed++;
			continue;
		}
		pattern[slot] = (unsigned char)(hartid * 17 + i + 1);
		sbi_memset(p[slot], pattern[slot], size[slot]);
	}

	return failed;
}
