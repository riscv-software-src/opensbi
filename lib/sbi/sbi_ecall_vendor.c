/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 *   Atish Patra <atish.patra@wdc.com>
 */

#include <sbi/sbi_ecall.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_trap.h>
#include <sbi/riscv_asm.h>

static inline unsigned long sbi_ecall_vendor_id(void)
{
	return SBI_EXT_VENDOR_START +
		(csr_read(CSR_MVENDORID) &
		 (SBI_EXT_VENDOR_END - SBI_EXT_VENDOR_START));
}

static int sbi_ecall_vendor_probe(unsigned long extid,
				  unsigned long *out_val)
{
	if (!sbi_platform_vendor_ext_check(sbi_platform_thishart_ptr()) ||
	    extid != sbi_ecall_vendor_id())
		*out_val = 0;
	else
		*out_val = 1;
	return 0;
}

static int sbi_ecall_vendor_handler(unsigned long extid, unsigned long funcid,
				    const struct sbi_trap_regs *regs,
				    unsigned long *out_val,
				    struct sbi_trap_info *out_trap)
{
	if (!sbi_platform_vendor_ext_check(sbi_platform_thishart_ptr()) ||
	    extid != sbi_ecall_vendor_id())
		return SBI_ERR_NOT_SUPPORTED;

	return sbi_platform_vendor_ext_provider(sbi_platform_thishart_ptr(),
						funcid, regs,
						out_val, out_trap);
}

struct sbi_ecall_extension ecall_vendor = {
	.extid_start = SBI_EXT_VENDOR_START,
	.extid_end = SBI_EXT_VENDOR_END,
	.probe = sbi_ecall_vendor_probe,
	.handle = sbi_ecall_vendor_handler,
};
