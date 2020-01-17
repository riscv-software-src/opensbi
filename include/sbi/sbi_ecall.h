/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#ifndef __SBI_ECALL_H__
#define __SBI_ECALL_H__

#include <sbi/sbi_types.h>
#include <sbi/sbi_list.h>

#define SBI_ECALL_VERSION_MAJOR		0
#define SBI_ECALL_VERSION_MINOR		2
#define SBI_OPENSBI_IMPID		1

struct sbi_trap_regs;
struct sbi_trap_info;
struct sbi_scratch;

struct sbi_ecall_extension {
	struct sbi_dlist head;
	unsigned long extid_start;
	unsigned long extid_end;
	int (* probe)(struct sbi_scratch *scratch,
		      unsigned long extid, unsigned long *out_val);
	int (* handle)(struct sbi_scratch *scratch,
		       unsigned long extid, unsigned long funcid,
		       unsigned long *args, unsigned long *out_val,
		       struct sbi_trap_info *out_trap);
};

extern struct sbi_ecall_extension ecall_base;
extern struct sbi_ecall_extension ecall_legacy;
extern struct sbi_ecall_extension ecall_time;
extern struct sbi_ecall_extension ecall_rfence;
extern struct sbi_ecall_extension ecall_ipi;
extern struct sbi_ecall_extension ecall_vendor;

u16 sbi_ecall_version_major(void);

u16 sbi_ecall_version_minor(void);

struct sbi_ecall_extension *sbi_ecall_find_extension(unsigned long extid);

int sbi_ecall_register_extension(struct sbi_ecall_extension *ext);

void sbi_ecall_unregister_extension(struct sbi_ecall_extension *ext);

int sbi_ecall_handler(u32 hartid, ulong mcause, struct sbi_trap_regs *regs,
		      struct sbi_scratch *scratch);

int sbi_ecall_init(void);

#endif
