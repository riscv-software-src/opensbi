/*
 * Copyright (c) 2018 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef __SBI_TRAP_H__
#define __SBI_TRAP_H__

#include <sbi/sbi_types.h>

struct sbi_trap_regs {
	unsigned long zero;
	unsigned long ra;
	unsigned long sp;
	unsigned long gp;
	unsigned long tp;
	unsigned long t0;
	unsigned long t1;
	unsigned long t2;
	unsigned long s0;
	unsigned long s1;
	unsigned long a0;
	unsigned long a1;
	unsigned long a2;
	unsigned long a3;
	unsigned long a4;
	unsigned long a5;
	unsigned long a6;
	unsigned long a7;
	unsigned long s2;
	unsigned long s3;
	unsigned long s4;
	unsigned long s5;
	unsigned long s6;
	unsigned long s7;
	unsigned long s8;
	unsigned long s9;
	unsigned long s10;
	unsigned long s11;
	unsigned long t3;
	unsigned long t4;
	unsigned long t5;
	unsigned long t6;
	unsigned long mepc;
	unsigned long mstatus;
} __attribute__((packed));

struct sbi_scratch;

int sbi_trap_redirect(struct sbi_trap_regs *regs,
		      struct sbi_scratch *scratch,
		      ulong epc, ulong cause, ulong tval);

void sbi_trap_handler(struct sbi_trap_regs *regs,
		      struct sbi_scratch *scratch);

#endif
