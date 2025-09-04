/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#ifndef __SBI_PLATFORM_H__
#define __SBI_PLATFORM_H__

/**
 * OpenSBI 32-bit platform version with:
 * 1. upper 16-bits as major number
 * 2. lower 16-bits as minor number
 */
#define SBI_PLATFORM_VERSION(Major, Minor) ((Major << 16) | Minor)

/** Offset of opensbi_version in struct sbi_platform */
#define SBI_PLATFORM_OPENSBI_VERSION_OFFSET (0x00)
/** Offset of platform_version in struct sbi_platform */
#define SBI_PLATFORM_VERSION_OFFSET (0x04)
/** Offset of name in struct sbi_platform */
#define SBI_PLATFORM_NAME_OFFSET (0x08)
/** Offset of features in struct sbi_platform */
#define SBI_PLATFORM_FEATURES_OFFSET (0x48)
/** Offset of hart_count in struct sbi_platform */
#define SBI_PLATFORM_HART_COUNT_OFFSET (0x50)
/** Offset of hart_stack_size in struct sbi_platform */
#define SBI_PLATFORM_HART_STACK_SIZE_OFFSET (0x54)
/** Offset of heap_size in struct sbi_platform */
#define SBI_PLATFORM_HEAP_SIZE_OFFSET (0x58)
/** Offset of reserved in struct sbi_platform */
#define SBI_PLATFORM_RESERVED_OFFSET (0x5c)
/** Offset of platform_ops_addr in struct sbi_platform */
#define SBI_PLATFORM_OPS_OFFSET (0x60)
/** Offset of firmware_context in struct sbi_platform */
#define SBI_PLATFORM_FIRMWARE_CONTEXT_OFFSET (0x60 + __SIZEOF_POINTER__)
/** Offset of hart_index2id in struct sbi_platform */
#define SBI_PLATFORM_HART_INDEX2ID_OFFSET (0x60 + (__SIZEOF_POINTER__ * 2))
/** Offset of cbom_block_size in struct sbi_platform */
#define SBI_PLATFORM_CBOM_BLOCK_SIZE_OFFSET (0x60 + (__SIZEOF_POINTER__ * 3))

#define SBI_PLATFORM_TLB_RANGE_FLUSH_LIMIT_DEFAULT		(1UL << 12)

#ifndef __ASSEMBLER__

#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_version.h>
#include <sbi/sbi_trap_ldst.h>

struct sbi_domain_memregion;
struct sbi_ecall_return;
struct sbi_trap_regs;
struct sbi_hart_features;
union sbi_ldst_data;

/** Possible feature flags of a platform */
enum sbi_platform_features {
	/** Platform has fault delegation support */
	SBI_PLATFORM_HAS_MFAULTS_DELEGATION = (1 << 1),

	/** Last index of Platform features*/
	SBI_PLATFORM_HAS_LAST_FEATURE = SBI_PLATFORM_HAS_MFAULTS_DELEGATION,
};

/** Default feature set for a platform */
#define SBI_PLATFORM_DEFAULT_FEATURES                                \
	(SBI_PLATFORM_HAS_MFAULTS_DELEGATION)

/** Platform functions */
struct sbi_platform_operations {
	/* Check if specified HART is allowed to do cold boot */
	bool (*cold_boot_allowed)(u32 hartid);

	/* Platform nascent initialization */
	int (*nascent_init)(void);

	/** Platform early initialization */
	int (*early_init)(bool cold_boot);
	/** Platform final initialization */
	int (*final_init)(bool cold_boot);

	/** Platform early exit */
	void (*early_exit)(void);
	/** Platform final exit */
	void (*final_exit)(void);

	/**
	 * For platforms that do not implement misa, non-standard
	 * methods are needed to determine cpu extension.
	 */
	int (*misa_check_extension)(char ext);

	/**
	 * For platforms that do not implement misa, non-standard
	 * methods are needed to get MXL field of misa.
	 */
	int (*misa_get_xlen)(void);

	/** Initialize (or populate) HART extensions for the platform */
	int (*extensions_init)(struct sbi_hart_features *hfeatures);

	/** Initialize (or populate) domains for the platform */
	int (*domains_init)(void);

	/** Initialize hw performance counters */
	int (*pmu_init)(void);

	/** Get platform specific mhpmevent value */
	uint64_t (*pmu_xlate_to_mhpmevent)(uint32_t event_idx, uint64_t data);

	/** Initialize the platform interrupt controller during cold boot */
	int (*irqchip_init)(void);

	/** Get tlb flush limit value **/
	u64 (*get_tlbr_flush_limit)(void);

	/** Get tlb fifo num entries*/
	u32 (*get_tlb_num_entries)(void);

	/** Initialize platform timer during cold boot */
	int (*timer_init)(void);

	/** Initialize the platform Message Proxy(MPXY) driver */
	int (*mpxy_init)(void);

	/** platform specific SBI extension implementation provider */
	int (*vendor_ext_provider)(long funcid,
				   struct sbi_trap_regs *regs,
				   struct sbi_ecall_return *out);

	/** platform specific handler to fixup load fault */
	int (*emulate_load)(int rlen, unsigned long addr,
			    union sbi_ldst_data *out_val);
	/** platform specific handler to fixup store fault */
	int (*emulate_store)(int wlen, unsigned long addr,
			     union sbi_ldst_data in_val);

	/** platform specific pmp setup on current HART */
	void (*pmp_set)(unsigned int n, unsigned long flags,
			unsigned long prot, unsigned long addr,
			unsigned long log2len);
	/** platform specific pmp disable on current HART */
	void (*pmp_disable)(unsigned int n);
};

/** Platform default per-HART stack size for exception/interrupt handling */
#define SBI_PLATFORM_DEFAULT_HART_STACK_SIZE	8192

/** Platform default heap size */
#define SBI_PLATFORM_DEFAULT_HEAP_SIZE(__num_hart)	\
					(0x8000 + 0x1000 * (__num_hart))

/** Representation of a platform */
struct sbi_platform {
	/**
	 * OpenSBI version this sbi_platform is based on.
	 * It's a 32-bit value where upper 16-bits are major number
	 * and lower 16-bits are minor number
	 */
	u32 opensbi_version;
	/**
	 * OpenSBI platform version released by vendor.
	 * It's a 32-bit value where upper 16-bits are major number
	 * and lower 16-bits are minor number
	 */
	u32 platform_version;
	/** Name of the platform */
	char name[64];
	/** Supported features */
	u64 features;
	/** Total number of HARTs (at most SBI_HARTMASK_MAX_BITS) */
	u32 hart_count;
	/** Per-HART stack size for exception/interrupt handling */
	u32 hart_stack_size;
	/** Size of heap shared by all HARTs */
	u32 heap_size;
	/** Reserved for future use */
	u32 reserved;
	/** Pointer to sbi platform operations */
	unsigned long platform_ops_addr;
	/** Pointer to system firmware specific context */
	unsigned long firmware_context;
	/**
	 * HART index to HART id table
	 *
	 * If hart_index2id != NULL then the table must contain a mapping
	 * for each HART index 0 <= <abc> < hart_count:
	 *     hart_index2id[<abc>] = some HART id
	 *
	 * If hart_index2id == NULL then we assume identity mapping
	 *     hart_index2id[<abc>] = <abc>
	 */
	const u32 *hart_index2id;
	/** Allocation alignment for Scratch */
	unsigned long cbom_block_size;
};

/**
 * Prevent modification of struct sbi_platform from affecting
 * SBI_PLATFORM_xxx_OFFSET
 */
assert_member_offset(struct sbi_platform, opensbi_version, SBI_PLATFORM_OPENSBI_VERSION_OFFSET);
assert_member_offset(struct sbi_platform, platform_version, SBI_PLATFORM_VERSION_OFFSET);
assert_member_offset(struct sbi_platform, name, SBI_PLATFORM_NAME_OFFSET);
assert_member_offset(struct sbi_platform, features, SBI_PLATFORM_FEATURES_OFFSET);
assert_member_offset(struct sbi_platform, hart_count, SBI_PLATFORM_HART_COUNT_OFFSET);
assert_member_offset(struct sbi_platform, hart_stack_size, SBI_PLATFORM_HART_STACK_SIZE_OFFSET);
assert_member_offset(struct sbi_platform, heap_size, SBI_PLATFORM_HEAP_SIZE_OFFSET);
assert_member_offset(struct sbi_platform, reserved, SBI_PLATFORM_RESERVED_OFFSET);
assert_member_offset(struct sbi_platform, platform_ops_addr, SBI_PLATFORM_OPS_OFFSET);
assert_member_offset(struct sbi_platform, firmware_context, SBI_PLATFORM_FIRMWARE_CONTEXT_OFFSET);
assert_member_offset(struct sbi_platform, hart_index2id, SBI_PLATFORM_HART_INDEX2ID_OFFSET);
assert_member_offset(struct sbi_platform, cbom_block_size, SBI_PLATFORM_CBOM_BLOCK_SIZE_OFFSET);

/** Get pointer to sbi_platform for sbi_scratch pointer */
#define sbi_platform_ptr(__s) \
	((const struct sbi_platform *)((__s)->platform_addr))
/** Get pointer to sbi_platform for current HART */
#define sbi_platform_thishart_ptr() ((const struct sbi_platform *) \
	(sbi_scratch_thishart_ptr()->platform_addr))
/** Get pointer to platform_ops_addr from platform pointer **/
#define sbi_platform_ops(__p) \
	((const struct sbi_platform_operations *)(__p)->platform_ops_addr)

/** Check whether the platform supports fault delegation */
#define sbi_platform_has_mfaults_delegation(__p) \
	((__p)->features & SBI_PLATFORM_HAS_MFAULTS_DELEGATION)

/**
 * Get the platform features in string format
 *
 * @param plat pointer to struct sbi_platform
 * @param features_str pointer to a char array where the features string will be
 *		       updated
 * @param nfstr length of the features_str. The feature string will be truncated
 *		if nfstr is not long enough.
 */
void sbi_platform_get_features_str(const struct sbi_platform *plat,
				   char *features_str, int nfstr);

/**
 * Get name of the platform
 *
 * @param plat pointer to struct sbi_platform
 *
 * @return pointer to platform name on success and "Unknown" on failure
 */
static inline const char *sbi_platform_name(const struct sbi_platform *plat)
{
	if (plat)
		return plat->name;
	return "Unknown";
}

/**
 * Get the platform features
 *
 * @param plat pointer to struct sbi_platform
 *
 * @return the features value currently set for the given platform
 */
static inline unsigned long sbi_platform_get_features(
						const struct sbi_platform *plat)
{
	if (plat)
		return plat->features;
	return 0;
}

/**
 * Get platform specific tlb range flush maximum value. Any request with size
 * higher than this is upgraded to a full flush.
 *
 * @param plat pointer to struct sbi_platform
 *
 * @return tlb range flush limit value. Returns a default (page size) if not
 * defined by platform.
 */
static inline u64 sbi_platform_tlbr_flush_limit(const struct sbi_platform *plat)
{
	if (plat && sbi_platform_ops(plat)->get_tlbr_flush_limit)
		return sbi_platform_ops(plat)->get_tlbr_flush_limit();
	return SBI_PLATFORM_TLB_RANGE_FLUSH_LIMIT_DEFAULT;
}

/**
 * Get platform specific tlb fifo num entries.
 *
 * @param plat pointer to struct sbi_platform
 *
 * @return number of tlb fifo entries
*/
static inline u32 sbi_platform_tlb_fifo_num_entries(const struct sbi_platform *plat)
{
	if (plat && sbi_platform_ops(plat)->get_tlb_num_entries)
		return sbi_platform_ops(plat)->get_tlb_num_entries();
	return sbi_hart_count();
}

/**
 * Get total number of HARTs supported by the platform
 *
 * @param plat pointer to struct sbi_platform
 *
 * @return total number of HARTs
 */
static inline u32 sbi_platform_hart_count(const struct sbi_platform *plat)
{
	if (plat)
		return plat->hart_count;
	return 0;
}

/**
 * Get per-HART stack size for exception/interrupt handling
 *
 * @param plat pointer to struct sbi_platform
 *
 * @return stack size in bytes
 */
static inline u32 sbi_platform_hart_stack_size(const struct sbi_platform *plat)
{
	if (plat)
		return plat->hart_stack_size;
	return 0;
}

/**
 * Check whether given HART is allowed to do cold boot
 *
 * @param plat pointer to struct sbi_platform
 * @param hartid HART ID
 *
 * @return true if HART is allowed to do cold boot and false otherwise
 */
static inline bool sbi_platform_cold_boot_allowed(
					const struct sbi_platform *plat,
					u32 hartid)
{
	if (plat && sbi_platform_ops(plat)->cold_boot_allowed)
		return sbi_platform_ops(plat)->cold_boot_allowed(hartid);
	return true;
}

/**
 * Nascent (very early) initialization for current HART
 *
 * NOTE: This function can be used to do very early initialization of
 * platform specific per-HART CSRs and devices.
 *
 * @param plat pointer to struct sbi_platform
 *
 * @return 0 on success and negative error code on failure
 */
static inline int sbi_platform_nascent_init(const struct sbi_platform *plat)
{
	if (plat && sbi_platform_ops(plat)->nascent_init)
		return sbi_platform_ops(plat)->nascent_init();
	return 0;
}

/**
 * Early initialization for current HART
 *
 * @param plat pointer to struct sbi_platform
 * @param cold_boot whether cold boot (true) or warm_boot (false)
 *
 * @return 0 on success and negative error code on failure
 */
static inline int sbi_platform_early_init(const struct sbi_platform *plat,
					  bool cold_boot)
{
	if (plat && sbi_platform_ops(plat)->early_init)
		return sbi_platform_ops(plat)->early_init(cold_boot);
	return 0;
}

/**
 * Final initialization for current HART
 *
 * @param plat pointer to struct sbi_platform
 * @param cold_boot whether cold boot (true) or warm_boot (false)
 *
 * @return 0 on success and negative error code on failure
 */
static inline int sbi_platform_final_init(const struct sbi_platform *plat,
					  bool cold_boot)
{
	if (plat && sbi_platform_ops(plat)->final_init)
		return sbi_platform_ops(plat)->final_init(cold_boot);
	return 0;
}

/**
 * Early exit for current HART
 *
 * @param plat pointer to struct sbi_platform
 */
static inline void sbi_platform_early_exit(const struct sbi_platform *plat)
{
	if (plat && sbi_platform_ops(plat)->early_exit)
		sbi_platform_ops(plat)->early_exit();
}

/**
 * Final exit for current HART
 *
 * @param plat pointer to struct sbi_platform
 */
static inline void sbi_platform_final_exit(const struct sbi_platform *plat)
{
	if (plat && sbi_platform_ops(plat)->final_exit)
		sbi_platform_ops(plat)->final_exit();
}

/**
 * Check CPU extension in MISA
 *
 * @param plat pointer to struct sbi_platform
 * @param ext shorthand letter for CPU extensions
 *
 * @return zero for not-supported and non-zero for supported
 */
static inline int sbi_platform_misa_extension(const struct sbi_platform *plat,
					      char ext)
{
	if (plat && sbi_platform_ops(plat)->misa_check_extension)
		return sbi_platform_ops(plat)->misa_check_extension(ext);
	return 0;
}

/**
 * Get MXL field of MISA
 *
 * @param plat pointer to struct sbi_platform
 *
 * @return 1/2/3 on success and error code on failure
 */
static inline int sbi_platform_misa_xlen(const struct sbi_platform *plat)
{
	if (plat && sbi_platform_ops(plat)->misa_get_xlen)
		return sbi_platform_ops(plat)->misa_get_xlen();
	return -1;
}

/**
 * Initialize (or populate) HART extensions for the platform
 *
 * @param plat pointer to struct sbi_platform
 *
 * @return 0 on success and negative error code on failure
 */
static inline int sbi_platform_extensions_init(
					const struct sbi_platform *plat,
					struct sbi_hart_features *hfeatures)
{
	if (plat && sbi_platform_ops(plat)->extensions_init)
		return sbi_platform_ops(plat)->extensions_init(hfeatures);
	return 0;
}

/**
 * Initialize (or populate) domains for the platform
 *
 * @param plat pointer to struct sbi_platform
 *
 * @return 0 on success and negative error code on failure
 */
static inline int sbi_platform_domains_init(const struct sbi_platform *plat)
{
	if (plat && sbi_platform_ops(plat)->domains_init)
		return sbi_platform_ops(plat)->domains_init();
	return 0;
}

/**
 * Setup hw PMU events for the platform
 *
 * @param plat pointer to struct sbi_platform
 *
 * @return 0 on success and negative error code on failure
 */
static inline int sbi_platform_pmu_init(const struct sbi_platform *plat)
{
	if (plat && sbi_platform_ops(plat)->pmu_init)
		return sbi_platform_ops(plat)->pmu_init();
	return 0;
}

/**
 * Get the value to be written in mhpmeventx for event_idx
 *
 * @param plat pointer to struct sbi_platform
 * @param event_idx ID of the PMU event
 * @param data Additional configuration data passed from supervisor software
 *
 * @return expected value by the platform or 0 if platform doesn't know about
 * the event
 */
static inline uint64_t sbi_platform_pmu_xlate_to_mhpmevent(const struct sbi_platform *plat,
						      uint32_t event_idx, uint64_t data)
{
	if (plat && sbi_platform_ops(plat)->pmu_xlate_to_mhpmevent)
		return sbi_platform_ops(plat)->pmu_xlate_to_mhpmevent(event_idx,
								      data);
	return 0;
}

/**
 * Initialize the platform interrupt controller during cold boot
 *
 * @param plat pointer to struct sbi_platform
 *
 * @return 0 on success and negative error code on failure
 */
static inline int sbi_platform_irqchip_init(const struct sbi_platform *plat)
{
	if (plat && sbi_platform_ops(plat)->irqchip_init)
		return sbi_platform_ops(plat)->irqchip_init();
	return 0;
}

/**
 * Initialize the platform timer during cold boot
 *
 * @param plat pointer to struct sbi_platform
 *
 * @return 0 on success and negative error code on failure
 */
static inline int sbi_platform_timer_init(const struct sbi_platform *plat)
{
	if (plat && sbi_platform_ops(plat)->timer_init)
		return sbi_platform_ops(plat)->timer_init();
	return 0;
}

/**
 * Initialize the platform Message Proxy drivers
 *
 * @param plat pointer to struct sbi_platform
 *
 * @return 0 on success and negative error code on failure
 */
static inline int sbi_platform_mpxy_init(const struct sbi_platform *plat)
{
	if (plat && sbi_platform_ops(plat)->mpxy_init)
		return sbi_platform_ops(plat)->mpxy_init();
	return 0;
}

/**
 * Check if SBI vendor extension is implemented or not.
 *
 * @param plat pointer to struct sbi_platform
 *
 * @return false if not implemented and true if implemented
 */
static inline bool sbi_platform_vendor_ext_check(
					const struct sbi_platform *plat)
{
	return plat && sbi_platform_ops(plat)->vendor_ext_provider;
}

/**
 * Invoke platform specific vendor SBI extension implementation.
 *
 * @param plat pointer to struct sbi_platform
 * @param funcid SBI function id within the extension id
 * @param regs pointer to trap registers passed by the caller
 * @param out_value output value that can be filled by the callee
 * @param out_trap trap info that can be filled by the callee
 *
 * @return 0 on success and negative error code on failure
 */
static inline int sbi_platform_vendor_ext_provider(
					const struct sbi_platform *plat,
					long funcid,
					struct sbi_trap_regs *regs,
					struct sbi_ecall_return *out)
{
	if (plat && sbi_platform_ops(plat)->vendor_ext_provider)
		return sbi_platform_ops(plat)->vendor_ext_provider(funcid,
								regs, out);

	return SBI_ENOTSUPP;
}

/**
 * Ask platform to emulate the trapped load
 *
 * @param plat pointer to struct sbi_platform
 * @param rlen length of the load: 1/2/4/8...
 * @param addr virtual address of the load. Platform needs to page-walk and
 *        find the physical address if necessary
 * @param out_val value loaded
 *
 * @return 0 on success and negative error code on failure
 */
static inline int sbi_platform_emulate_load(const struct sbi_platform *plat,
					    int rlen, unsigned long addr,
					    union sbi_ldst_data *out_val)
{
	if (plat && sbi_platform_ops(plat)->emulate_load) {
		return sbi_platform_ops(plat)->emulate_load(rlen, addr,
							    out_val);
	}
	return SBI_ENOTSUPP;
}

/**
 * Ask platform to emulate the trapped store
 *
 * @param plat pointer to struct sbi_platform
 * @param wlen length of the store: 1/2/4/8...
 * @param addr virtual address of the store. Platform needs to page-walk and
 *        find the physical address if necessary
 * @param in_val value to store
 *
 * @return 0 on success and negative error code on failure
 */
static inline int sbi_platform_emulate_store(const struct sbi_platform *plat,
					     int wlen, unsigned long addr,
					     union sbi_ldst_data in_val)
{
	if (plat && sbi_platform_ops(plat)->emulate_store) {
		return sbi_platform_ops(plat)->emulate_store(wlen, addr,
							     in_val);
	}
	return SBI_ENOTSUPP;
}

/**
 * Platform specific PMP setup on current HART
 *
 * @param plat pointer to struct sbi_platform
 * @param n index of the pmp entry
 * @param flags domain memregion flags
 * @param prot attribute of the pmp entry
 * @param addr address of the pmp entry
 * @param log2len size of the pmp entry as power-of-2
 */
static inline void sbi_platform_pmp_set(const struct sbi_platform *plat,
					unsigned int n, unsigned long flags,
					unsigned long prot, unsigned long addr,
					unsigned long log2len)
{
	if (plat && sbi_platform_ops(plat)->pmp_set)
		sbi_platform_ops(plat)->pmp_set(n, flags, prot, addr, log2len);
}

/**
 * Platform specific PMP disable on current HART
 *
 * @param plat pointer to struct sbi_platform
 * @param n index of the pmp entry
 */
static inline void sbi_platform_pmp_disable(const struct sbi_platform *plat,
					    unsigned int n)
{
	if (plat && sbi_platform_ops(plat)->pmp_disable)
		sbi_platform_ops(plat)->pmp_disable(n);
}

#endif

#endif
