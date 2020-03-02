/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#ifndef __SBI_HART_H__
#define __SBI_HART_H__

#include <sbi/sbi_types.h>

struct sbi_scratch;

int sbi_hart_init(struct sbi_scratch *scratch, u32 hartid, bool cold_boot);

void *sbi_hart_get_trap_info(struct sbi_scratch *scratch);

void sbi_hart_set_trap_info(struct sbi_scratch *scratch, void *data);

void sbi_hart_delegation_dump(struct sbi_scratch *scratch);
void sbi_hart_pmp_dump(struct sbi_scratch *scratch);
int  sbi_hart_pmp_check_addr(struct sbi_scratch *scratch, unsigned long daddr,
			     unsigned long attr);

void __attribute__((noreturn)) sbi_hart_hang(void);

void __attribute__((noreturn))
sbi_hart_switch_mode(unsigned long arg0, unsigned long arg1,
		     unsigned long next_addr, unsigned long next_mode,
		     bool next_virt);

struct sbi_scratch *sbi_hart_id_to_scratch(struct sbi_scratch *scratch,
					   u32 hartid);

u32 sbi_current_hartid(void);

#endif
