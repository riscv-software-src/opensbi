/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2026 RISCstar Solutions.
 *
 * Authors:
 *   Dave Patel <dave.patel@riscstar.com>
 */

#ifndef __SBI_VECTOR_H__
#define __SBI_VECTOR_H__

#include <sbi/sbi_types.h>

struct sbi_vector_context {
	unsigned long vcsr;
	unsigned long vstart;

	/* size depends on VLEN */
	uint8_t vregs[];
};

#ifdef OPENSBI_CC_SUPPORT_VECTOR
void sbi_vector_save(struct sbi_vector_context *dst);
void sbi_vector_restore(const struct sbi_vector_context *src);
size_t sbi_vector_context_size(void);
#else
static inline void sbi_vector_save(struct sbi_vector_context *dst)
{
}
static inline void sbi_vector_restore(const struct sbi_vector_context *src)
{
}
static inline size_t sbi_vector_context_size(void)
{
	return 0;
}
#endif /* OPENSBI_CC_SUPPORT_VECTOR */

#endif /* __SBI_VECTOR_H__ */
