/*
 * Copyright (c) 2018 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef __SBI_SCRATCH_H__
#define __SBI_SCRATCH_H__

#include <sbi/riscv_asm.h>
#include <sbi/sbi_types.h>

struct sbi_scratch {
	unsigned long tmp0;
	unsigned long fw_start;
	unsigned long fw_size;
	unsigned long next_arg1;
	unsigned long next_addr;
	unsigned long next_mode;
	unsigned long warmboot_addr;
	unsigned long platform_addr;
	unsigned long hartid_to_scratch;
	unsigned long ipi_type;
} __attribute__((packed));

#define sbi_scratch_thishart_ptr()	\
((struct sbi_scratch *)csr_read(mscratch))

#define sbi_scratch_thishart_arg1_ptr()	\
((void *)(sbi_scratch_thishart_ptr()->next_arg1))

#endif
