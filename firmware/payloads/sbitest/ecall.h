/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * SBI call wrappers of the S-mode test payload.
 */

#ifndef __SBITEST_ECALL_H__
#define __SBITEST_ECALL_H__

#include <sbi/sbi_ecall_interface.h>

struct sbiret {
	long error;
	long value;
};

static inline struct sbiret sbi_ecall(unsigned long ext, unsigned long fid,
				      unsigned long arg0, unsigned long arg1,
				      unsigned long arg2, unsigned long arg3,
				      unsigned long arg4, unsigned long arg5)
{
	register unsigned long a0 asm("a0") = arg0;
	register unsigned long a1 asm("a1") = arg1;
	register unsigned long a2 asm("a2") = arg2;
	register unsigned long a3 asm("a3") = arg3;
	register unsigned long a4 asm("a4") = arg4;
	register unsigned long a5 asm("a5") = arg5;
	register unsigned long a6 asm("a6") = fid;
	register unsigned long a7 asm("a7") = ext;
	struct sbiret ret;

	asm volatile("ecall"
		     : "+r"(a0), "+r"(a1)
		     : "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6), "r"(a7)
		     : "memory");
	ret.error = a0;
	ret.value = a1;

	return ret;
}

/* Base extension */
static inline struct sbiret sbi_get_spec_version(void)
{
	return sbi_ecall(SBI_EXT_BASE, SBI_EXT_BASE_GET_SPEC_VERSION, 0, 0, 0,
			 0, 0, 0);
}

/* Hart state management extension */
static inline struct sbiret sbi_hart_start(unsigned long hartid,
					   unsigned long start_addr,
					   unsigned long opaque)
{
	return sbi_ecall(SBI_EXT_HSM, SBI_EXT_HSM_HART_START, hartid,
			 start_addr, opaque, 0, 0, 0);
}

/* IPI extension */
static inline struct sbiret sbi_send_ipi(unsigned long hart_mask,
					 unsigned long hart_mask_base)
{
	return sbi_ecall(SBI_EXT_IPI, SBI_EXT_IPI_SEND_IPI, hart_mask,
			 hart_mask_base, 0, 0, 0, 0);
}

/* Debug console extension: write bytes of a buffer in physical memory */
static inline struct sbiret sbi_debug_console_write(unsigned long num_bytes,
						    const char *base)
{
	return sbi_ecall(SBI_EXT_DBCN, SBI_EXT_DBCN_CONSOLE_WRITE, num_bytes,
			 (unsigned long)base, 0, 0, 0, 0);
}

/* System reset extension */
static inline struct sbiret sbi_system_reset(unsigned long type,
					     unsigned long reason)
{
	return sbi_ecall(SBI_EXT_SRST, SBI_EXT_SRST_RESET, type, reason, 0, 0,
			 0, 0);
}

#endif /* __SBITEST_ECALL_H__ */
