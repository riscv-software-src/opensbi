/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#ifndef __SBI_DOMAIN_H__
#define __SBI_DOMAIN_H__

#include <sbi/riscv_locks.h>
#include <sbi/sbi_list.h>
#include <sbi/sbi_types.h>
#include <sbi/sbi_hartmask.h>
#include <sbi/sbi_domain_context.h>
#include <sbi/sbi_domain_data.h>

struct sbi_scratch;

/** Domain access types */
enum sbi_domain_access {
	SBI_DOMAIN_READ = (1UL << 0),
	SBI_DOMAIN_WRITE = (1UL << 1),
	SBI_DOMAIN_EXECUTE = (1UL << 2),
	SBI_DOMAIN_MMIO = (1UL << 3)
};

/** Representation of OpenSBI domain memory region */
struct sbi_domain_memregion {
	/**
	 * Size of memory region as power of 2
	 * It has to be minimum 3 and maximum __riscv_xlen
	 */
	unsigned long order;
	/**
	 * Base address of memory region
	 * It must be 2^order aligned address
	 */
	unsigned long base;
	/** Flags representing memory region attributes */
#define SBI_DOMAIN_MEMREGION_M_READABLE		(1UL << 0)
#define SBI_DOMAIN_MEMREGION_M_WRITABLE		(1UL << 1)
#define SBI_DOMAIN_MEMREGION_M_EXECUTABLE	(1UL << 2)
#define SBI_DOMAIN_MEMREGION_SU_READABLE	(1UL << 3)
#define SBI_DOMAIN_MEMREGION_SU_WRITABLE	(1UL << 4)
#define SBI_DOMAIN_MEMREGION_SU_EXECUTABLE	(1UL << 5)

#define SBI_DOMAIN_MEMREGION_ACCESS_MASK	(0x3fUL)
#define SBI_DOMAIN_MEMREGION_M_ACCESS_MASK	(0x7UL)
#define SBI_DOMAIN_MEMREGION_SU_ACCESS_MASK	(0x38UL)

#define SBI_DOMAIN_MEMREGION_SU_ACCESS_SHIFT	(3)

#define SBI_DOMAIN_MEMREGION_SHARED_RDONLY		\
		(SBI_DOMAIN_MEMREGION_M_READABLE |	\
		 SBI_DOMAIN_MEMREGION_SU_READABLE)

#define SBI_DOMAIN_MEMREGION_SHARED_SUX_MRX		\
		(SBI_DOMAIN_MEMREGION_M_READABLE   |	\
		 SBI_DOMAIN_MEMREGION_M_EXECUTABLE |	\
		 SBI_DOMAIN_MEMREGION_SU_EXECUTABLE)

#define SBI_DOMAIN_MEMREGION_SHARED_SUX_MX		\
		(SBI_DOMAIN_MEMREGION_M_EXECUTABLE |	\
		 SBI_DOMAIN_MEMREGION_SU_EXECUTABLE)

#define SBI_DOMAIN_MEMREGION_SHARED_SURW_MRW		\
		(SBI_DOMAIN_MEMREGION_M_READABLE |	\
		 SBI_DOMAIN_MEMREGION_M_WRITABLE |	\
		 SBI_DOMAIN_MEMREGION_SU_READABLE|	\
		 SBI_DOMAIN_MEMREGION_SU_WRITABLE)

#define SBI_DOMAIN_MEMREGION_SHARED_SUR_MRW		\
		(SBI_DOMAIN_MEMREGION_M_READABLE |	\
		 SBI_DOMAIN_MEMREGION_M_WRITABLE |	\
		 SBI_DOMAIN_MEMREGION_SU_READABLE)

	/* Shared read-only region between M and SU mode */
#define SBI_DOMAIN_MEMREGION_IS_SUR_MR(__flags)			 \
		((__flags & SBI_DOMAIN_MEMREGION_ACCESS_MASK) == \
		 SBI_DOMAIN_MEMREGION_SHARED_RDONLY)

	/* Shared region: SU execute-only and M read/execute */
#define SBI_DOMAIN_MEMREGION_IS_SUX_MRX(__flags)		 \
		((__flags & SBI_DOMAIN_MEMREGION_ACCESS_MASK) == \
		 SBI_DOMAIN_MEMREGION_SHARED_SUX_MRX)

	/* Shared region: SU and M execute-only */
#define SBI_DOMAIN_MEMREGION_IS_SUX_MX(__flags)			 \
		((__flags & SBI_DOMAIN_MEMREGION_ACCESS_MASK) == \
		 SBI_DOMAIN_MEMREGION_SHARED_SUX_MX)

	/* Shared region: SU and M read/write */
#define SBI_DOMAIN_MEMREGION_IS_SURW_MRW(__flags)		 \
		((__flags & SBI_DOMAIN_MEMREGION_ACCESS_MASK) == \
		 SBI_DOMAIN_MEMREGION_SHARED_SURW_MRW)

	/* Shared region: SU read-only and M read/write */
#define SBI_DOMAIN_MEMREGION_IS_SUR_MRW(__flags)		 \
		((__flags & SBI_DOMAIN_MEMREGION_ACCESS_MASK) == \
		 SBI_DOMAIN_MEMREGION_SHARED_SUR_MRW)

	/*
	 * Check if region flags match with any of the above
	 * mentioned shared region type
	 */
#define SBI_DOMAIN_MEMREGION_IS_SHARED(_flags)			\
		(SBI_DOMAIN_MEMREGION_IS_SUR_MR(_flags)  ||	\
		 SBI_DOMAIN_MEMREGION_IS_SUX_MRX(_flags) ||	\
		 SBI_DOMAIN_MEMREGION_IS_SUX_MX(_flags)  ||	\
		 SBI_DOMAIN_MEMREGION_IS_SURW_MRW(_flags)||	\
		 SBI_DOMAIN_MEMREGION_IS_SUR_MRW(_flags))

#define SBI_DOMAIN_MEMREGION_M_ONLY_ACCESS(__flags)			\
		((__flags & SBI_DOMAIN_MEMREGION_M_ACCESS_MASK) &&	\
		 !(__flags & SBI_DOMAIN_MEMREGION_SU_ACCESS_MASK))

#define SBI_DOMAIN_MEMREGION_SU_ONLY_ACCESS(__flags)			\
		((__flags & SBI_DOMAIN_MEMREGION_SU_ACCESS_MASK)  &&	\
		 !(__flags & SBI_DOMAIN_MEMREGION_M_ACCESS_MASK))

#define SBI_DOMAIN_MEMREGION_IS_FIRMWARE(__flags)			\
		((__flags & SBI_DOMAIN_MEMREGION_FW) ? true : false)	\

/** Bit to control if permissions are enforced on all modes */
#define SBI_DOMAIN_MEMREGION_ENF_PERMISSIONS	(1UL << 6)

#define SBI_DOMAIN_MEMREGION_M_RWX		\
				(SBI_DOMAIN_MEMREGION_M_READABLE | \
				 SBI_DOMAIN_MEMREGION_M_WRITABLE | \
				 SBI_DOMAIN_MEMREGION_M_EXECUTABLE)

#define SBI_DOMAIN_MEMREGION_SU_RWX		\
				(SBI_DOMAIN_MEMREGION_SU_READABLE | \
				 SBI_DOMAIN_MEMREGION_SU_WRITABLE | \
				 SBI_DOMAIN_MEMREGION_SU_EXECUTABLE)

/* Unrestricted M-mode accesses but enfoced on SU-mode */
#define SBI_DOMAIN_MEMREGION_READABLE		\
				(SBI_DOMAIN_MEMREGION_SU_READABLE | \
				 SBI_DOMAIN_MEMREGION_M_RWX)
#define SBI_DOMAIN_MEMREGION_WRITEABLE		\
				(SBI_DOMAIN_MEMREGION_SU_WRITABLE | \
				 SBI_DOMAIN_MEMREGION_M_RWX)
#define SBI_DOMAIN_MEMREGION_EXECUTABLE		\
				(SBI_DOMAIN_MEMREGION_SU_EXECUTABLE | \
				 SBI_DOMAIN_MEMREGION_M_RWX)

/* Enforced accesses across all modes */
#define SBI_DOMAIN_MEMREGION_ENF_READABLE	\
				(SBI_DOMAIN_MEMREGION_SU_READABLE | \
				 SBI_DOMAIN_MEMREGION_M_READABLE)
#define SBI_DOMAIN_MEMREGION_ENF_WRITABLE	\
				(SBI_DOMAIN_MEMREGION_SU_WRITABLE | \
				 SBI_DOMAIN_MEMREGION_M_WRITABLE)
#define SBI_DOMAIN_MEMREGION_ENF_EXECUTABLE	\
				(SBI_DOMAIN_MEMREGION_SU_EXECUTABLE | \
				 SBI_DOMAIN_MEMREGION_M_EXECUTABLE)

#define SBI_DOMAIN_MEMREGION_MMIO		(1UL << 31)
#define SBI_DOMAIN_MEMREGION_FW			(1UL << 30)
	unsigned long flags;
};

/** Representation of OpenSBI domain */
struct sbi_domain {
	/** Node in linked list of domains */
	struct sbi_dlist node;
	/** Internal state of per-domain data */
	struct sbi_domain_data_priv data_priv;
	/** Logical index of this domain */
	u32 index;
	/** HARTs assigned to this domain */
	struct sbi_hartmask assigned_harts;
	/** Spinlock for accessing assigned_harts */
	spinlock_t assigned_harts_lock;
	/** Name of this domain */
	char name[64];
	/** Possible HARTs in this domain */
	const struct sbi_hartmask *possible_harts;
	/** Array of memory regions terminated by a region with order zero */
	struct sbi_domain_memregion *regions;
	/** HART id of the HART booting this domain */
	u32 boot_hartid;
	/** Arg1 (or 'a1' register) of next booting stage for this domain */
	unsigned long next_arg1;
	/** Address of next booting stage for this domain */
	unsigned long next_addr;
	/** Privilege mode of next booting stage for this domain */
	unsigned long next_mode;
	/** Is domain allowed to reset the system */
	bool system_reset_allowed;
	/** Is domain allowed to suspend the system */
	bool system_suspend_allowed;
	/** Identifies whether to include the firmware region */
	bool fw_region_inited;
};

/** The root domain instance */
extern struct sbi_domain root;

/** Get pointer to sbi_domain from HART index */
struct sbi_domain *sbi_hartindex_to_domain(u32 hartindex);

/** Update HART local pointer to point to specified domain */
void sbi_update_hartindex_to_domain(u32 hartindex, struct sbi_domain *dom);

/** Get pointer to sbi_domain for current HART */
#define sbi_domain_thishart_ptr() \
	sbi_hartindex_to_domain(current_hartindex())

/** Head of linked list of domains */
extern struct sbi_dlist domain_list;

/** Iterate over each domain */
#define sbi_domain_for_each(__d) \
	sbi_list_for_each_entry(__d, &domain_list, node)

/** Iterate over each memory region of a domain */
#define sbi_domain_for_each_memregion(__d, __r) \
	for ((__r) = (__d)->regions; (__r)->order; (__r)++)

/**
 * Check whether given HART is assigned to specified domain
 * @param dom pointer to domain
 * @param hartindex the HART index
 * @return true if HART is assigned to domain otherwise false
 */
bool sbi_domain_is_assigned_hart(const struct sbi_domain *dom, u32 hartindex);

/**
 * Get the assigned HART mask for given domain
 * @param dom pointer to domain
 * @param mask the output hartmask to fill
 * @return 0 on success and SBI_Exxx (< 0) on failure
 */
int sbi_domain_get_assigned_hartmask(const struct sbi_domain *dom,
				     struct sbi_hartmask *mask);

/**
 * Initialize a domain memory region based on it's physical
 * address and size.
 *
 * @param addr start physical address of memory region
 * @param size physical size of memory region
 * @param flags memory region flags
 * @param reg pointer to memory region being initialized
 */
void sbi_domain_memregion_init(unsigned long addr,
				unsigned long size,
				unsigned long flags,
				struct sbi_domain_memregion *reg);

/**
 * Return the Smepmp pmpcfg LRWX encoding for the flags in @reg.
 *
 * @param reg pointer to memory region; its flags field encodes permissions.
 */
unsigned int sbi_domain_get_smepmp_flags(struct sbi_domain_memregion *reg);

/**
 * Check whether we can access specified address for given mode and
 * memory region flags under a domain
 * @param dom pointer to domain
 * @param addr the address to be checked
 * @param mode the privilege mode of access
 * @param access_flags bitmask of domain access types (enum sbi_domain_access)
 * @return true if access allowed otherwise false
 */
bool sbi_domain_check_addr(const struct sbi_domain *dom,
			   unsigned long addr, unsigned long mode,
			   unsigned long access_flags);

/**
 * Check whether we can access specified address range for given mode and
 * memory region flags under a domain
 * @param dom pointer to domain
 * @param addr the start of the address range to be checked
 * @param size the size of the address range to be checked
 * @param mode the privilege mode of access
 * @param access_flags bitmask of domain access types (enum sbi_domain_access)
 * @return TRUE if access allowed otherwise FALSE
 */
bool sbi_domain_check_addr_range(const struct sbi_domain *dom,
				 unsigned long addr, unsigned long size,
				 unsigned long mode,
				 unsigned long access_flags);

/** Dump domain details on the console */
void sbi_domain_dump(const struct sbi_domain *dom, const char *suffix);

/** Dump all domain details on the console */
void sbi_domain_dump_all(const char *suffix);

/**
 * Register a new domain
 * @param dom pointer to domain
 * @param assign_mask pointer to HART mask of HARTs assigned to the domain
 *
 * @return 0 on success and negative error code on failure
 */
int sbi_domain_register(struct sbi_domain *dom,
			const struct sbi_hartmask *assign_mask);

/**
 * Add a memory range with its flags to the root domain
 * @param addr start physical address of memory range
 * @param size physical size of memory range
 * @param align alignment of memory region
 * @param region_flags memory range flags
 *
 * @return 0 on success
 * @return SBI_EALREADY if memory region conflicts with the existing one
 * @return SBI_EINVAL otherwise
 */
int sbi_domain_root_add_memrange(unsigned long addr, unsigned long size,
			   unsigned long align, unsigned long region_flags);

/** Startup non-root domains */
int sbi_domain_startup(struct sbi_scratch *scratch, u32 cold_hartid);

/** Finalize domain tables */
int sbi_domain_finalize(struct sbi_scratch *scratch);

/** Initialize domains */
int sbi_domain_init(struct sbi_scratch *scratch, u32 cold_hartid);

#endif
