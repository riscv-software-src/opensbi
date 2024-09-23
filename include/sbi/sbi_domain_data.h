/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024 Ventana Micro Systems Inc.
 */

#ifndef __SBI_DOMAIN_DATA_H__
#define __SBI_DOMAIN_DATA_H__

#include <sbi/sbi_types.h>
#include <sbi/sbi_list.h>

struct sbi_domain;

/** Maximum domain data per-domain */
#define SBI_DOMAIN_MAX_DATA_PTRS		32

/** Representation of per-domain data */
struct sbi_domain_data_priv {
	/** Array of domain data pointers indexed by domain data identifier */
	void *idx_to_data_ptr[SBI_DOMAIN_MAX_DATA_PTRS];
};

/** Representation of a domain data */
struct sbi_domain_data {
	/**
	 * Head is used for maintaining data list
	 *
	 * Note: initialized by domain framework
	 */
	struct sbi_dlist head;
	/**
	 * Identifier which used to locate per-domain data
	 *
	 * Note: initialized by domain framework
	 */
	unsigned long data_idx;
	/** Size of per-domain data */
	unsigned long data_size;
	/** Optional callback to setup domain data */
	int (*data_setup)(struct sbi_domain *dom,
			  struct sbi_domain_data *data, void *data_ptr);
	/** Optional callback to cleanup domain data */
	void (*data_cleanup)(struct sbi_domain *dom,
			     struct sbi_domain_data *data, void *data_ptr);
};

/**
 * Get per-domain data pointer for a given domain
 * @param dom pointer to domain
 * @param data pointer to domain data
 *
 * @return per-domain data pointer
 */
void *sbi_domain_data_ptr(struct sbi_domain *dom, struct sbi_domain_data *data);

/**
 * Setup all domain data for a domain
 * @param dom pointer to domain
 *
 * @return 0 on success and negative error code on failure
 *
 * Note: This function is used internally within domain framework.
 */
int sbi_domain_setup_data(struct sbi_domain *dom);

/**
 * Cleanup all domain data for a domain
 * @param dom pointer to domain
 *
 * Note: This function is used internally within domain framework.
 */
void sbi_domain_cleanup_data(struct sbi_domain *dom);

/**
 * Register a domain data
 * @param hndl pointer to domain data
 *
 * @return 0 on success and negative error code on failure
 *
 * Note: This function must be used only in cold boot path.
 */
int sbi_domain_register_data(struct sbi_domain_data *data);

/**
 * Unregister a domain data
 * @param hndl pointer to domain data
 *
 * Note: This function must be used only in cold boot path.
 */
void sbi_domain_unregister_data(struct sbi_domain_data *data);

#endif
