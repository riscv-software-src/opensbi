/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) IPADS@SJTU 2023. All rights reserved.
 */

#ifndef __SBI_DOMAIN_CONTEXT_H__
#define __SBI_DOMAIN_CONTEXT_H__

#include <sbi/sbi_types.h>
#include <sbi/sbi_trap.h>
#include <sbi/sbi_domain.h>

/** Context representation for a hart within a domain */
struct sbi_context {
	/** Trap-related states such as GPRs, mepc, and mstatus */
	struct sbi_trap_regs regs;

	/** Supervisor status register */
	unsigned long sstatus;
	/** Supervisor interrupt enable register */
	unsigned long sie;
	/** Supervisor trap vector base address register */
	unsigned long stvec;
	/** Supervisor scratch register for temporary storage */
	unsigned long sscratch;
	/** Supervisor exception program counter register */
	unsigned long sepc;
	/** Supervisor cause register */
	unsigned long scause;
	/** Supervisor trap value register */
	unsigned long stval;
	/** Supervisor interrupt pending register */
	unsigned long sip;
	/** Supervisor address translation and protection register */
	unsigned long satp;
	/** Counter-enable register */
	unsigned long scounteren;
	/** Supervisor environment configuration register */
	unsigned long senvcfg;

	/** Reference to the owning domain */
	struct sbi_domain *dom;
	/** Previous context (caller) to jump to during context exits */
	struct sbi_context *prev_ctx;
	/** Is context initialized and runnable */
	bool initialized;
};

/** Get the context pointer for a given hart index and domain */
#define sbi_hartindex_to_domain_context(__hartindex, __d) \
	(__d)->hartindex_to_context_table[__hartindex]

/** Macro to obtain the current hart's context pointer */
#define sbi_domain_context_thishart_ptr()                  \
	sbi_hartindex_to_domain_context(                   \
		sbi_hartid_to_hartindex(current_hartid()), \
		sbi_domain_thishart_ptr())

/**
 * Enter a specific domain context synchronously
 * @param dom pointer to domain
 *
 * @return 0 on success and negative error code on failure
 */
int sbi_domain_context_enter(struct sbi_domain *dom);

/**
 * Exit the current domain context, and then return to the caller
 * of sbi_domain_context_enter or attempt to start the next domain
 * context to be initialized
 *
 * @return 0 on success and negative error code on failure
 */
int sbi_domain_context_exit(void);

#endif // __SBI_DOMAIN_CONTEXT_H__
