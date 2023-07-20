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

/** Possible privileged specification versions of a hart */
enum sbi_hart_priv_versions {
	/** Unknown privileged specification */
	SBI_HART_PRIV_VER_UNKNOWN = 0,
	/** Privileged specification v1.10 */
	SBI_HART_PRIV_VER_1_10 = 1,
	/** Privileged specification v1.11 */
	SBI_HART_PRIV_VER_1_11 = 2,
	/** Privileged specification v1.12 */
	SBI_HART_PRIV_VER_1_12 = 3,
};

/** Possible ISA extensions of a hart */
enum sbi_hart_extensions {
	/** HART has AIA M-mode CSRs */
	SBI_HART_EXT_SMAIA = 0,
	/** HART has Smepmp */
	SBI_HART_EXT_SMEPMP,
	/** HART has Smstateen CSR **/
	SBI_HART_EXT_SMSTATEEN,
	/** Hart has Sscofpmt extension */
	SBI_HART_EXT_SSCOFPMF,
	/** HART has Sstc extension */
	SBI_HART_EXT_SSTC,
	/** HART has Zicntr extension (i.e. HW cycle, time & instret CSRs) */
	SBI_HART_EXT_ZICNTR,
	/** HART has Zihpm extension */
	SBI_HART_EXT_ZIHPM,
	/** Hart has Smcntrpmf extension */
	SBI_HART_EXT_SMCNTRPMF,

	/** Maximum index of Hart extension */
	SBI_HART_EXT_MAX,
};

/*
 * Smepmp enforces access boundaries between M-mode and
 * S/U-mode. When it is enabled, the PMPs are programmed
 * such that M-mode doesn't have access to S/U-mode memory.
 *
 * To give M-mode R/W access to the shared memory between M and
 * S/U-mode, first entry is reserved. It is disabled at boot.
 * When shared memory access is required, the physical address
 * should be programmed into the first PMP entry with R/W
 * permissions to the M-mode. Once the work is done, it should be
 * unmapped. sbi_hart_map_saddr/sbi_hart_unmap_saddr function
 * pair should be used to map/unmap the shared memory.
 */
#define SBI_SMEPMP_RESV_ENTRY		0

struct sbi_hart_features {
	bool detected;
	int priv_version;
	unsigned long extensions;
	unsigned int pmp_count;
	unsigned int pmp_addr_bits;
	unsigned long pmp_gran;
	unsigned int mhpm_count;
	unsigned int mhpm_mask;
	unsigned int mhpm_bits;
};

struct sbi_scratch;

int sbi_hart_reinit(struct sbi_scratch *scratch);
int sbi_hart_init(struct sbi_scratch *scratch, bool cold_boot);

extern void (*sbi_hart_expected_trap)(void);
static inline ulong sbi_hart_expected_trap_addr(void)
{
	return (ulong)sbi_hart_expected_trap;
}

unsigned int sbi_hart_mhpm_count(struct sbi_scratch *scratch);
unsigned int sbi_hart_mhpm_mask(struct sbi_scratch *scratch);
void sbi_hart_delegation_dump(struct sbi_scratch *scratch,
			      const char *prefix, const char *suffix);
unsigned int sbi_hart_pmp_count(struct sbi_scratch *scratch);
unsigned long sbi_hart_pmp_granularity(struct sbi_scratch *scratch);
unsigned int sbi_hart_pmp_addrbits(struct sbi_scratch *scratch);
unsigned int sbi_hart_mhpm_bits(struct sbi_scratch *scratch);
int sbi_hart_pmp_configure(struct sbi_scratch *scratch);
int sbi_hart_map_saddr(unsigned long base, unsigned long size);
int sbi_hart_unmap_saddr(void);
int sbi_hart_priv_version(struct sbi_scratch *scratch);
void sbi_hart_get_priv_version_str(struct sbi_scratch *scratch,
				   char *version_str, int nvstr);
void sbi_hart_update_extension(struct sbi_scratch *scratch,
			       enum sbi_hart_extensions ext,
			       bool enable);
bool sbi_hart_has_extension(struct sbi_scratch *scratch,
			    enum sbi_hart_extensions ext);
void sbi_hart_get_extensions_str(struct sbi_scratch *scratch,
				 char *extension_str, int nestr);

void __attribute__((noreturn)) sbi_hart_hang(void);

void __attribute__((noreturn))
sbi_hart_switch_mode(unsigned long arg0, unsigned long arg1,
		     unsigned long next_addr, unsigned long next_mode,
		     bool next_virt);

#endif
