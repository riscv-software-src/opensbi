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

/** Representation of hart protection mechanism */
struct sbi_hart_protection {
	/** List head */
	struct sbi_dlist head;

	/** Name of the hart protection mechanism */
	char name[32];

	/** Ratings of the hart protection mechanism (higher is better) */
	unsigned long rating;

	/** Configure protection for current HART (Mandatory) */
	int (*configure)(struct sbi_scratch *scratch, struct sbi_domain *dom);

	/** Unconfigure protection for current HART (Optional) */
	void (*unconfigure)(struct sbi_scratch *scratch, struct sbi_domain *dom);

	/** Create temporary mapping to access address range on current HART (Optional) */
	int (*map_range)(struct sbi_scratch *scratch,
			 unsigned long base, unsigned long size);

	/** Destroy temporary mapping on current HART (Optional) */
	int (*unmap_range)(struct sbi_scratch *scratch,
			   unsigned long base, unsigned long size);
};

/**
 * Get the best hart protection mechanism
 *
 * @return pointer to best hart protection mechanism
 */
struct sbi_hart_protection *sbi_hart_protection_best(void);

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
 * Create temporary mapping to access address range on current HART
 *
 * @param base base address of the temporary mapping
 * @param size size of the temporary mapping
 *
 * @return 0 on success and negative error code on failure
 */
int sbi_hart_protection_map_range(unsigned long base, unsigned long size);

/**
 * Destroy temporary mapping to access address range on current HART
 *
 * @param base base address of the temporary mapping
 * @param size size of the temporary mapping
 *
 * @return 0 on success and negative error code on failure
 */
int sbi_hart_protection_unmap_range(unsigned long base, unsigned long size);

#endif /* __SBI_HART_PROTECTION_H__ */
