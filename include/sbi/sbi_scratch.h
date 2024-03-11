/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#ifndef __SBI_SCRATCH_H__
#define __SBI_SCRATCH_H__

#include <sbi/riscv_asm.h>

/* clang-format off */

/** Offset of fw_start member in sbi_scratch */
#define SBI_SCRATCH_FW_START_OFFSET		(0 * __SIZEOF_POINTER__)
/** Offset of fw_size member in sbi_scratch */
#define SBI_SCRATCH_FW_SIZE_OFFSET		(1 * __SIZEOF_POINTER__)
/** Offset (in sbi_scratch) of the R/W Offset */
#define SBI_SCRATCH_FW_RW_OFFSET		(2 * __SIZEOF_POINTER__)
/** Offset of fw_heap_offset member in sbi_scratch */
#define SBI_SCRATCH_FW_HEAP_OFFSET		(3 * __SIZEOF_POINTER__)
/** Offset of fw_heap_size_offset member in sbi_scratch */
#define SBI_SCRATCH_FW_HEAP_SIZE_OFFSET		(4 * __SIZEOF_POINTER__)
/** Offset of next_arg1 member in sbi_scratch */
#define SBI_SCRATCH_NEXT_ARG1_OFFSET		(5 * __SIZEOF_POINTER__)
/** Offset of next_addr member in sbi_scratch */
#define SBI_SCRATCH_NEXT_ADDR_OFFSET		(6 * __SIZEOF_POINTER__)
/** Offset of next_mode member in sbi_scratch */
#define SBI_SCRATCH_NEXT_MODE_OFFSET		(7 * __SIZEOF_POINTER__)
/** Offset of warmboot_addr member in sbi_scratch */
#define SBI_SCRATCH_WARMBOOT_ADDR_OFFSET	(8 * __SIZEOF_POINTER__)
/** Offset of platform_addr member in sbi_scratch */
#define SBI_SCRATCH_PLATFORM_ADDR_OFFSET	(9 * __SIZEOF_POINTER__)
/** Offset of hartid_to_scratch member in sbi_scratch */
#define SBI_SCRATCH_HARTID_TO_SCRATCH_OFFSET	(10 * __SIZEOF_POINTER__)
/** Offset of trap_context member in sbi_scratch */
#define SBI_SCRATCH_TRAP_CONTEXT_OFFSET		(11 * __SIZEOF_POINTER__)
/** Offset of tmp0 member in sbi_scratch */
#define SBI_SCRATCH_TMP0_OFFSET			(12 * __SIZEOF_POINTER__)
/** Offset of options member in sbi_scratch */
#define SBI_SCRATCH_OPTIONS_OFFSET		(13 * __SIZEOF_POINTER__)
/** Offset of extra space in sbi_scratch */
#define SBI_SCRATCH_EXTRA_SPACE_OFFSET		(14 * __SIZEOF_POINTER__)
/** Maximum size of sbi_scratch (4KB) */
#define SBI_SCRATCH_SIZE			(0x1000)

/* clang-format on */

#ifndef __ASSEMBLER__

#include <sbi/sbi_types.h>

/** Representation of per-HART scratch space */
struct sbi_scratch {
	/** Start (or base) address of firmware linked to OpenSBI library */
	unsigned long fw_start;
	/** Size (in bytes) of firmware linked to OpenSBI library */
	unsigned long fw_size;
	/** Offset (in bytes) of the R/W section */
	unsigned long fw_rw_offset;
	/** Offset (in bytes) of the heap area */
	unsigned long fw_heap_offset;
	/** Size (in bytes) of the heap area */
	unsigned long fw_heap_size;
	/** Arg1 (or 'a1' register) of next booting stage for this HART */
	unsigned long next_arg1;
	/** Address of next booting stage for this HART */
	unsigned long next_addr;
	/** Privilege mode of next booting stage for this HART */
	unsigned long next_mode;
	/** Warm boot entry point address for this HART */
	unsigned long warmboot_addr;
	/** Address of sbi_platform */
	unsigned long platform_addr;
	/** Address of HART ID to sbi_scratch conversion function */
	unsigned long hartid_to_scratch;
	/** Address of current trap context */
	unsigned long trap_context;
	/** Temporary storage */
	unsigned long tmp0;
	/** Options for OpenSBI library */
	unsigned long options;
};

/**
 * Prevent modification of struct sbi_scratch from affecting
 * SBI_SCRATCH_xxx_OFFSET
 */
_Static_assert(
	offsetof(struct sbi_scratch, fw_start)
		== SBI_SCRATCH_FW_START_OFFSET,
	"struct sbi_scratch definition has changed, please redefine "
	"SBI_SCRATCH_FW_START_OFFSET");
_Static_assert(
	offsetof(struct sbi_scratch, fw_size)
		== SBI_SCRATCH_FW_SIZE_OFFSET,
	"struct sbi_scratch definition has changed, please redefine "
	"SBI_SCRATCH_FW_SIZE_OFFSET");
_Static_assert(
	offsetof(struct sbi_scratch, next_arg1)
		== SBI_SCRATCH_NEXT_ARG1_OFFSET,
	"struct sbi_scratch definition has changed, please redefine "
	"SBI_SCRATCH_NEXT_ARG1_OFFSET");
_Static_assert(
	offsetof(struct sbi_scratch, next_addr)
		== SBI_SCRATCH_NEXT_ADDR_OFFSET,
	"struct sbi_scratch definition has changed, please redefine "
	"SBI_SCRATCH_NEXT_ADDR_OFFSET");
_Static_assert(
	offsetof(struct sbi_scratch, next_mode)
		== SBI_SCRATCH_NEXT_MODE_OFFSET,
	"struct sbi_scratch definition has changed, please redefine "
	"SBI_SCRATCH_NEXT_MODE_OFFSET");
_Static_assert(
	offsetof(struct sbi_scratch, warmboot_addr)
		== SBI_SCRATCH_WARMBOOT_ADDR_OFFSET,
	"struct sbi_scratch definition has changed, please redefine "
	"SBI_SCRATCH_WARMBOOT_ADDR_OFFSET");
_Static_assert(
	offsetof(struct sbi_scratch, platform_addr)
		== SBI_SCRATCH_PLATFORM_ADDR_OFFSET,
	"struct sbi_scratch definition has changed, please redefine "
	"SBI_SCRATCH_PLATFORM_ADDR_OFFSET");
_Static_assert(
	offsetof(struct sbi_scratch, hartid_to_scratch)
		== SBI_SCRATCH_HARTID_TO_SCRATCH_OFFSET,
	"struct sbi_scratch definition has changed, please redefine "
	"SBI_SCRATCH_HARTID_TO_SCRATCH_OFFSET");
_Static_assert(
	offsetof(struct sbi_scratch, trap_context)
		== SBI_SCRATCH_TRAP_CONTEXT_OFFSET,
	"struct sbi_scratch definition has changed, please redefine "
	"SBI_SCRATCH_TRAP_CONTEXT_OFFSET");
_Static_assert(
	offsetof(struct sbi_scratch, tmp0)
		== SBI_SCRATCH_TMP0_OFFSET,
	"struct sbi_scratch definition has changed, please redefine "
	"SBI_SCRATCH_TMP0_OFFSET");
_Static_assert(
	offsetof(struct sbi_scratch, options)
		== SBI_SCRATCH_OPTIONS_OFFSET,
	"struct sbi_scratch definition has changed, please redefine "
	"SBI_SCRATCH_OPTIONS_OFFSET");

/** Possible options for OpenSBI library */
enum sbi_scratch_options {
	/** Disable prints during boot */
	SBI_SCRATCH_NO_BOOT_PRINTS = (1 << 0),
	/** Enable runtime debug prints */
	SBI_SCRATCH_DEBUG_PRINTS = (1 << 1),
};

/** Get pointer to sbi_scratch for current HART */
#define sbi_scratch_thishart_ptr() \
	((struct sbi_scratch *)csr_read(CSR_MSCRATCH))

/** Get Arg1 of next booting stage for current HART */
#define sbi_scratch_thishart_arg1_ptr() \
	((void *)(sbi_scratch_thishart_ptr()->next_arg1))

/** Initialize scratch table and allocator */
int sbi_scratch_init(struct sbi_scratch *scratch);

/**
 * Allocate from extra space in sbi_scratch
 *
 * @return zero on failure and non-zero (>= SBI_SCRATCH_EXTRA_SPACE_OFFSET)
 * on success
 */
unsigned long sbi_scratch_alloc_offset(unsigned long size);

/** Free-up extra space in sbi_scratch */
void sbi_scratch_free_offset(unsigned long offset);

/** Amount (in bytes) of used space in in sbi_scratch */
unsigned long sbi_scratch_used_space(void);

/** Get pointer from offset in sbi_scratch */
#define sbi_scratch_offset_ptr(scratch, offset)	(void *)((char *)(scratch) + (offset))

/** Get pointer from offset in sbi_scratch for current HART */
#define sbi_scratch_thishart_offset_ptr(offset)	\
	(void *)((char *)sbi_scratch_thishart_ptr() + (offset))

/** Allocate offset for a data type in sbi_scratch */
#define sbi_scratch_alloc_type_offset(__type)				\
	sbi_scratch_alloc_offset(sizeof(__type))

/** Read a data type from sbi_scratch at given offset */
#define sbi_scratch_read_type(__scratch, __type, __offset)		\
({									\
	*((__type *)sbi_scratch_offset_ptr((__scratch), (__offset)));	\
})

/** Write a data type to sbi_scratch at given offset */
#define sbi_scratch_write_type(__scratch, __type, __offset, __ptr)	\
do {									\
	*((__type *)sbi_scratch_offset_ptr((__scratch), (__offset)))	\
					= (__type)(__ptr);		\
} while (0)

/** Last HART index having a sbi_scratch pointer */
extern u32 last_hartindex_having_scratch;

/** Get last HART index having a sbi_scratch pointer */
#define sbi_scratch_last_hartindex()	last_hartindex_having_scratch

/** Check whether a particular HART index is valid or not */
#define sbi_hartindex_valid(__hartindex) \
(((__hartindex) <= sbi_scratch_last_hartindex()) ? true : false)

/** HART index to HART id table */
extern u32 hartindex_to_hartid_table[];

/** Get sbi_scratch from HART index */
#define sbi_hartindex_to_hartid(__hartindex)		\
({							\
	((__hartindex) <= sbi_scratch_last_hartindex()) ?\
	hartindex_to_hartid_table[__hartindex] : -1U;	\
})

/** HART index to scratch table */
extern struct sbi_scratch *hartindex_to_scratch_table[];

/** Get sbi_scratch from HART index */
#define sbi_hartindex_to_scratch(__hartindex)		\
({							\
	((__hartindex) <= sbi_scratch_last_hartindex()) ?\
	hartindex_to_scratch_table[__hartindex] : NULL;\
})

/**
 * Get logical index for given HART id
 * @param hartid physical HART id
 * @returns value between 0 to SBI_HARTMASK_MAX_BITS upon success and
 *	    SBI_HARTMASK_MAX_BITS upon failure.
 */
u32 sbi_hartid_to_hartindex(u32 hartid);

/** Get sbi_scratch from HART id */
#define sbi_hartid_to_scratch(__hartid) \
	sbi_hartindex_to_scratch(sbi_hartid_to_hartindex(__hartid))

/** Check whether particular HART id is valid or not */
#define sbi_hartid_valid(__hartid)	\
	sbi_hartindex_valid(sbi_hartid_to_hartindex(__hartid))

#endif

#endif
