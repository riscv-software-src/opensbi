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

static int sbi_ecall_vendor_probe(struct sbi_scratch *scratch,
				  unsigned long extid,
				  unsigned long *out_val)
{
	*out_val = sbi_platform_vendor_ext_check(sbi_platform_ptr(scratch),
						 extid);
	return 0;
}

static int sbi_ecall_vendor_handler(struct sbi_scratch *scratch,
				    unsigned long extid, unsigned long funcid,
				    unsigned long *args, unsigned long *out_val,
				    struct sbi_trap_info *out_trap)
{
	return sbi_platform_vendor_ext_provider(sbi_platform_ptr(scratch),
						extid, funcid, args,
						out_val, out_trap);
}

struct sbi_ecall_extension ecall_vendor = {
	.extid_start = SBI_EXT_VENDOR_START,
	.extid_end = SBI_EXT_VENDOR_END,
	.probe = sbi_ecall_vendor_probe,
	.handle = sbi_ecall_vendor_handler,
};
