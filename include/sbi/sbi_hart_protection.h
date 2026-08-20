/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 Ventana Micro Systems Inc.
 */

#ifndef __SBI_HART_PROTECTION_H__
#define __SBI_HART_PROTECTION_H__

#include <sbi/sbi_types.h>
#include <sbi/sbi_list.h>

struct sbi_scratch;
struct sbi_domain;

/** Different types of hart protection mechanisms */
enum sbi_hart_protection_type {
	SBI_HART_PROTECTION_TYPE_MEMORY = 0,
	SBI_HART_PROTECTION_TYPE_ID,
	SBI_HART_PROTECTION_TYPE_MAX
};

/** Representation of hart protection mechanism */
struct sbi_hart_protection {
	/** List head */
	struct sbi_dlist head;

	/** Name of the hart protection mechanism */
	char name[32];

	/** Type of the hart protection mechanism */
	enum sbi_hart_protection_type type;

	/** Ratings of the hart protection mechanism (higher is better) */
	unsigned long rating;

	/** Configure protection for current HART (Mandatory) */
	int (*configure)(struct sbi_scratch *scratch, struct sbi_domain *dom);

	/** Unconfigure protection for current HART (Optional) */
	void (*unconfigure)(struct sbi_scratch *scratch, struct sbi_domain *dom);

	/**
	 * Give temporary M-mode access to an S/U-mode address range on current
	 * HART (Optional). Only applicable for the TYPE_MEMORY and the TYPE_ID
	 * mechanisms are not invoked.
	 */
	int (*temp_map_range)(struct sbi_scratch *scratch,
			      unsigned long base, unsigned long size);

	/** Remove the temporary M-mode access on the current HART (Optional) */
	int (*temp_unmap_range)(struct sbi_scratch *scratch,
				unsigned long base, unsigned long size);
};

/**
 * Get a string containing names of protection mechanisms
 *
 * @param out_str pointer to output string
 * @param out_str_size maximum size of output string
 *
 * @return pointer to best hart memory protection mechanism
 */
void sbi_hart_protection_get_str(char *out_str, int out_str_size);

/**
 * Register a hart protection mechanism
 *
 * @param hprot pointer to hart protection mechanism
 *
 * @return 0 on success and negative error code on failure
 */
int sbi_hart_protection_register(struct sbi_hart_protection *hprot);

/**
 * Unregister a hart protection mechanism
 *
 * @param hprot pointer to hart protection mechanism
 */
void sbi_hart_protection_unregister(struct sbi_hart_protection *hprot);

/**
 * Configure protection for current HART
 *
 * @param scratch pointer to scratch space of current HART
 * @param dom pointer to the domain for which HART protection
 *        is being configured
 *
 * @return 0 on success and negative error code on failure
 */
int sbi_hart_protection_configure(struct sbi_scratch *scratch,
				  struct sbi_domain *dom);

/**
 * Unconfigure protection for current HART
 *
 * @param scratch pointer to scratch space of current HART
 * @param dom pointer to the domain for which HART protection
 *        is being unconfigured
 */
void sbi_hart_protection_unconfigure(struct sbi_scratch *scratch,
				     struct sbi_domain *dom);

/**
 * Re-configure protection for current HART
 *
 * @param scratch pointer to scratch space of current HART
 * @param current_dom pointer to the current domain for which
 *        HART protection is being unconfigured
 * @param next_dom pointer to the next domain for which HART
 *        protection is being configured
 */
int sbi_hart_protection_reconfigure(struct sbi_scratch *scratch,
				    struct sbi_domain *current_dom,
				    struct sbi_domain *next_dom);

/**
 * Give temporary M-mode access to an S/U-mode address range on the
 * current HART. Only valid for TYPE_MEMORY.
 *
 * @param base base address of the temporary mapping
 * @param size size of the temporary mapping
 *
 * @return 0 on success and negative error code on failure
 */
int sbi_hart_protection_temp_map_range(unsigned long base, unsigned long size);

/**
 * Remove the temporary M-mode access on the current HART
 *
 * @param base base address of the temporary mapping
 * @param size size of the temporary mapping
 *
 * @return 0 on success and negative error code on failure
 */
int sbi_hart_protection_temp_unmap_range(unsigned long base, unsigned long size);

#endif /* __SBI_HART_PROTECTION_H__ */
