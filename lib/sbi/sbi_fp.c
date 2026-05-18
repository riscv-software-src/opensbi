/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2026 RISCstar Solutions.
 *
 * Authors:
 *   Dave Patel <dave.patel@riscstar.com>
 */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_encoding.h>
#include <sbi/sbi_fp.h>

#if defined(__riscv_f) || defined(__riscv_d)

void sbi_fp_save(struct sbi_fp_context *dst)
{
	unsigned long mstatus_orig;

	if (!dst)
		return;

	mstatus_orig = csr_read_set(CSR_MSTATUS, MSTATUS_VS);

	asm volatile(
#if defined(__riscv_d)
		"fsd f0,  0(%0)\n"
		"fsd f1,  8(%0)\n"
		"fsd f2,  16(%0)\n"
		"fsd f3,  24(%0)\n"
		"fsd f4,  32(%0)\n"
		"fsd f5,  40(%0)\n"
		"fsd f6,  48(%0)\n"
		"fsd f7,  56(%0)\n"
		"fsd f8,  64(%0)\n"
		"fsd f9,  72(%0)\n"
		"fsd f10, 80(%0)\n"
		"fsd f11, 88(%0)\n"
		"fsd f12, 96(%0)\n"
		"fsd f13, 104(%0)\n"
		"fsd f14, 112(%0)\n"
		"fsd f15, 120(%0)\n"
		"fsd f16, 128(%0)\n"
		"fsd f17, 136(%0)\n"
		"fsd f18, 144(%0)\n"
		"fsd f19, 152(%0)\n"
		"fsd f20, 160(%0)\n"
		"fsd f21, 168(%0)\n"
		"fsd f22, 176(%0)\n"
		"fsd f23, 184(%0)\n"
		"fsd f24, 192(%0)\n"
		"fsd f25, 200(%0)\n"
		"fsd f26, 208(%0)\n"
		"fsd f27, 216(%0)\n"
		"fsd f28, 224(%0)\n"
		"fsd f29, 232(%0)\n"
		"fsd f30, 240(%0)\n"
		"fsd f31, 248(%0)\n"
#else
		"fsw f0,  0(%0)\n"
		"fsw f1,  4(%0)\n"
		"fsw f2,  8(%0)\n"
		"fsw f3,  12(%0)\n"
		"fsw f4,  16(%0)\n"
		"fsw f5,  20(%0)\n"
		"fsw f6,  24(%0)\n"
		"fsw f7,  28(%0)\n"
		"fsw f8,  32(%0)\n"
		"fsw f9,  36(%0)\n"
		"fsw f10, 40(%0)\n"
		"fsw f11, 44(%0)\n"
		"fsw f12, 48(%0)\n"
		"fsw f13, 52(%0)\n"
		"fsw f14, 56(%0)\n"
		"fsw f15, 60(%0)\n"
		"fsw f16, 64(%0)\n"
		"fsw f17, 68(%0)\n"
		"fsw f18, 72(%0)\n"
		"fsw f19, 76(%0)\n"
		"fsw f20, 80(%0)\n"
		"fsw f21, 84(%0)\n"
		"fsw f22, 88(%0)\n"
		"fsw f23, 92(%0)\n"
		"fsw f24, 96(%0)\n"
		"fsw f25, 100(%0)\n"
		"fsw f26, 104(%0)\n"
		"fsw f27, 108(%0)\n"
		"fsw f28, 112(%0)\n"
		"fsw f29, 116(%0)\n"
		"fsw f30, 120(%0)\n"
		"fsw f31, 124(%0)\n"
#endif /* __riscv_d */
		:
		: "r"(dst->f)
		: "memory"
	);

	dst->fcsr = csr_read(CSR_FCSR);

	/* Restore original mstatus LAST */
	csr_write(CSR_MSTATUS, mstatus_orig);
}

void sbi_fp_restore(const struct sbi_fp_context *src)
{
	unsigned long mstatus_orig;

	if (!src)
		return;

	/* Save original mstatus */
	mstatus_orig = csr_read_set(CSR_MSTATUS, MSTATUS_FS);

	asm volatile(
#if defined(__riscv_d)
		"fld f0,  0(%0)\n"
		"fld f1,  8(%0)\n"
		"fld f2,  16(%0)\n"
		"fld f3,  24(%0)\n"
		"fld f4,  32(%0)\n"
		"fld f5,  40(%0)\n"
		"fld f6,  48(%0)\n"
		"fld f7,  56(%0)\n"
		"fld f8,  64(%0)\n"
		"fld f9,  72(%0)\n"
		"fld f10, 80(%0)\n"
		"fld f11, 88(%0)\n"
		"fld f12, 96(%0)\n"
		"fld f13, 104(%0)\n"
		"fld f14, 112(%0)\n"
		"fld f15, 120(%0)\n"
		"fld f16, 128(%0)\n"
		"fld f17, 136(%0)\n"
		"fld f18, 144(%0)\n"
		"fld f19, 152(%0)\n"
		"fld f20, 160(%0)\n"
		"fld f21, 168(%0)\n"
		"fld f22, 176(%0)\n"
		"fld f23, 184(%0)\n"
		"fld f24, 192(%0)\n"
		"fld f25, 200(%0)\n"
		"fld f26, 208(%0)\n"
		"fld f27, 216(%0)\n"
		"fld f28, 224(%0)\n"
		"fld f29, 232(%0)\n"
		"fld f30, 240(%0)\n"
		"fld f31, 248(%0)\n"
#else
		"flw f0,   0(%0)\n"
		"flw f1,   4(%0)\n"
		"flw f2,   8(%0)\n"
		"flw f3,  12(%0)\n"
		"flw f4,  16(%0)\n"
		"flw f5,  20(%0)\n"
		"flw f6,  24(%0)\n"
		"flw f7,  28(%0)\n"
		"flw f8,  32(%0)\n"
		"flw f9,  36(%0)\n"
		"flw f10, 40(%0)\n"
		"flw f11, 44(%0)\n"
		"flw f12, 48(%0)\n"
		"flw f13, 52(%0)\n"
		"flw f14, 56(%0)\n"
		"flw f15, 60(%0)\n"
		"flw f16, 64(%0)\n"
		"flw f17, 68(%0)\n"
		"flw f18, 72(%0)\n"
		"flw f19, 76(%0)\n"
		"flw f20, 80(%0)\n"
		"flw f21, 84(%0)\n"
		"flw f22, 88(%0)\n"
		"flw f23, 92(%0)\n"
		"flw f24, 96(%0)\n"
		"flw f25, 100(%0)\n"
		"flw f26, 104(%0)\n"
		"flw f27, 108(%0)\n"
		"flw f28, 112(%0)\n"
		"flw f29, 116(%0)\n"
		"flw f30, 120(%0)\n"
		"flw f31, 124(%0)\n"
#endif /* __riscv_d */
		:
		: "r"(src->f)
		: "memory"
	);

	csr_write(CSR_FCSR, src->fcsr);

	/* Restore original mstatus LAST */
	csr_write(CSR_MSTATUS, mstatus_orig);
}
#endif /* __riscv_f || __riscv_d */
