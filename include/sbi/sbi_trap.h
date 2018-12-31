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

#define SBI_TRAP_REGS_zero			0
#define SBI_TRAP_REGS_ra			1
#define SBI_TRAP_REGS_sp			2
#define SBI_TRAP_REGS_gp			3
#define SBI_TRAP_REGS_tp			4
#define SBI_TRAP_REGS_t0			5
#define SBI_TRAP_REGS_t1			6
#define SBI_TRAP_REGS_t2			7
#define SBI_TRAP_REGS_s0			8
#define SBI_TRAP_REGS_s1			9
#define SBI_TRAP_REGS_a0			10
#define SBI_TRAP_REGS_a1			11
#define SBI_TRAP_REGS_a2			12
#define SBI_TRAP_REGS_a3			13
#define SBI_TRAP_REGS_a4			14
#define SBI_TRAP_REGS_a5			15
#define SBI_TRAP_REGS_a6			16
#define SBI_TRAP_REGS_a7			17
#define SBI_TRAP_REGS_s2			18
#define SBI_TRAP_REGS_s3			19
#define SBI_TRAP_REGS_s4			20
#define SBI_TRAP_REGS_s5			21
#define SBI_TRAP_REGS_s6			22
#define SBI_TRAP_REGS_s7			23
#define SBI_TRAP_REGS_s8			24
#define SBI_TRAP_REGS_s9			25
#define SBI_TRAP_REGS_s10			26
#define SBI_TRAP_REGS_s11			27
#define SBI_TRAP_REGS_t3			28
#define SBI_TRAP_REGS_t4			29
#define SBI_TRAP_REGS_t5			30
#define SBI_TRAP_REGS_t6			31
#define SBI_TRAP_REGS_mepc			32
#define SBI_TRAP_REGS_mstatus			33
#define SBI_TRAP_REGS_last			34

#define SBI_TRAP_REGS_OFFSET(x)			\
				((SBI_TRAP_REGS_##x) * __SIZEOF_POINTER__)
#define SBI_TRAP_REGS_SIZE			SBI_TRAP_REGS_OFFSET(last)

#ifndef __ASSEMBLY__

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

#endif
