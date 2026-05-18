/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2026 RISCstar Solutions.
 *
 * Authors:
 *   Dave Patel <dave.patel@riscstar.com>
 */
#ifndef __SBI_FP_H__
#define __SBI_FP_H__

#include <sbi/sbi_types.h>

struct sbi_fp_context {
#if __riscv_d
	uint64_t f[32];
#else
	uint32_t f[32];
#endif
	unsigned long fcsr;
};

#if defined(__riscv_f) || defined(__riscv_d)
void sbi_fp_save(struct sbi_fp_context *dst);
void sbi_fp_restore(const struct sbi_fp_context *src);
#else
static inline void sbi_fp_save(struct sbi_fp_context *dst)
{
}
static inline void sbi_fp_restore(const struct sbi_fp_context *src)
{
}
#endif /* __riscv_f || __riscv_d */

#endif /*__SBI_FP_H__ */
