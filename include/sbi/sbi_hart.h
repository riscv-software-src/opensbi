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

/** Possible feature flags of a hart */
enum sbi_hart_features {
	/** Hart has PMP support */
	SBI_HART_HAS_PMP = (1 << 0),
	/** Hart has S-mode counter enable */
	SBI_HART_HAS_SCOUNTEREN = (1 << 1),
	/** Hart has M-mode counter enable */
	SBI_HART_HAS_MCOUNTEREN = (1 << 2),
	/** HART has timer csr implementation in hardware */
	SBI_HART_HAS_TIME = (1 << 3),

	/** Last index of Hart features*/
	SBI_HART_HAS_LAST_FEATURE = SBI_HART_HAS_TIME,
};

struct sbi_scratch;

int sbi_hart_init(struct sbi_scratch *scratch, u32 hartid, bool cold_boot);

extern void (*sbi_hart_expected_trap)(void);
static inline ulong sbi_hart_expected_trap_addr(void)
{
	return (ulong)sbi_hart_expected_trap;
}

void sbi_hart_delegation_dump(struct sbi_scratch *scratch);
unsigned int sbi_hart_pmp_count(struct sbi_scratch *scratch);
int sbi_hart_pmp_get(struct sbi_scratch *scratch, unsigned int n,
		     unsigned long *prot_out, unsigned long *addr_out,
		     unsigned long *size);
void sbi_hart_pmp_dump(struct sbi_scratch *scratch);
int  sbi_hart_pmp_check_addr(struct sbi_scratch *scratch, unsigned long daddr,
			     unsigned long attr);
bool sbi_hart_has_feature(struct sbi_scratch *scratch, unsigned long feature);
void sbi_hart_get_features_str(struct sbi_scratch *scratch,
			       char *features_str, int nfstr);

void __attribute__((noreturn)) sbi_hart_hang(void);

void __attribute__((noreturn))
sbi_hart_switch_mode(unsigned long arg0, unsigned long arg1,
		     unsigned long next_addr, unsigned long next_mode,
		     bool next_virt);

#endif
