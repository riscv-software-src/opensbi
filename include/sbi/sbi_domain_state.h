/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024 Ventana Micro Systems Inc.
 */

#ifndef __SBI_DOMAIN_STATE_H__
#define __SBI_DOMAIN_STATE_H__

#include <sbi/sbi_types.h>
#include <sbi/sbi_list.h>

struct sbi_domain;

/** Maximum number of per-domain state areas */
#define SBI_DOMAIN_MAX_STATE_PTRS		32

/** Internal per-domain state areas */
struct sbi_domain_state_priv {
	/** Array of per-domain state pointers indexed by state identifier */
	void *idx_to_state_ptr[SBI_DOMAIN_MAX_STATE_PTRS];
};

/** Representation of a domain state */
struct sbi_domain_state {
	/**
	 * Head is used for maintaining state list
	 *
	 * Note: initialized by domain framework
	 */
	struct sbi_dlist head;
	/**
	 * Identifier which used to locate per-domain state
	 *
	 * Note: initialized by domain framework
	 */
	unsigned long state_idx;
	/** Size of per-domain state */
	unsigned long state_size;
	/** Optional callback to setup domain state */
	int (*state_setup)(struct sbi_domain *dom,
			  struct sbi_domain_state *state, void *state_ptr);
	/** Optional callback to cleanup domain state */
	void (*state_cleanup)(struct sbi_domain *dom,
			     struct sbi_domain_state *state, void *state_ptr);
};

/**
 * Get per-domain state pointer for a given domain
 * @param dom pointer to domain
 * @param state pointer to domain state
 *
 * @return per-domain state pointer
 */
void *sbi_domain_state_ptr(struct sbi_domain *dom, struct sbi_domain_state *state);

/**
 * Setup all domain state for a domain
 * @param dom pointer to domain
 *
 * @return 0 on success and negative error code on failure
 *
 * Note: This function is used internally within domain framework.
 */
int sbi_domain_setup_state(struct sbi_domain *dom);

/**
 * Cleanup all domain state for a domain
 * @param dom pointer to domain
 *
 * Note: This function is used internally within domain framework.
 */
void sbi_domain_cleanup_state(struct sbi_domain *dom);

/**
 * Register a domain state
 * @param hndl pointer to domain state
 *
 * @return 0 on success and negative error code on failure
 *
 * Note: This function must be used only in cold boot path.
 */
int sbi_domain_register_state(struct sbi_domain_state *state);

/**
 * Unregister a domain state
 * @param hndl pointer to domain state
 *
 * Note: This function must be used only in cold boot path.
 */
void sbi_domain_unregister_state(struct sbi_domain_state *state);

#endif
