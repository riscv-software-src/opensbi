/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) IPADS@SJTU 2023. All rights reserved.
 */

#include <sbi/sbi_error.h>
#include <sbi/riscv_locks.h>
#include <sbi/riscv_asm.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_hsm.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_heap.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_string.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_domain_context.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_trap.h>

/** Context representation for a hart within a domain */
struct hart_context {
	/** Trap-related states such as GPRs, mepc, and mstatus */
	struct sbi_trap_context trap_ctx;

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
	/** Supervisor resource management configuration register */
	unsigned long srmcfg;

	/** Reference to the owning domain */
	struct sbi_domain *dom;
	/** Previous context (caller) to jump to during context exits */
	struct hart_context *prev_ctx;
	/** Is context initialized and runnable */
	bool initialized;
};

static struct sbi_domain_data dcpriv;

static inline struct hart_context *hart_context_get(struct sbi_domain *dom,
						    u32 hartindex)
{
	struct hart_context **dom_hartindex_to_context_table;

	dom_hartindex_to_context_table = sbi_domain_data_ptr(dom, &dcpriv);
	if (!dom_hartindex_to_context_table || !sbi_hartindex_valid(hartindex))
		return NULL;

	return dom_hartindex_to_context_table[hartindex];
}

static void hart_context_set(struct sbi_domain *dom, u32 hartindex,
			     struct hart_context *hc)
{
	struct hart_context **dom_hartindex_to_context_table;

	dom_hartindex_to_context_table = sbi_domain_data_ptr(dom, &dcpriv);
	if (!dom_hartindex_to_context_table || !sbi_hartindex_valid(hartindex))
		return;

	dom_hartindex_to_context_table[hartindex] = hc;
}

/** Macro to obtain the current hart's context pointer */
#define hart_context_thishart_get()					\
	hart_context_get(sbi_domain_thishart_ptr(),			\
			 current_hartindex())

/**
 * Switches the HART context from the current domain to the target domain.
 * This includes changing domain assignments and reconfiguring PMP, as well
 * as saving and restoring CSRs and trap states.
 *
 * @param ctx pointer to the current HART context
 * @param dom_ctx pointer to the target domain context
 *
 * @return 0 on success and negative error code on failure
 */
static int switch_to_next_domain_context(struct hart_context *ctx,
					  struct hart_context *dom_ctx)
{
	u32 hartindex = current_hartindex();
	struct sbi_trap_context *trap_ctx;
	struct sbi_domain *current_dom, *target_dom;
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();
	unsigned int pmp_count = sbi_hart_pmp_count(scratch);

	if (!ctx || !dom_ctx || ctx == dom_ctx)
		return SBI_EINVAL;

	current_dom = ctx->dom;
	target_dom = dom_ctx->dom;
	/* Assign current hart to target domain */
	spin_lock(&current_dom->assigned_harts_lock);
	sbi_hartmask_clear_hartindex(hartindex, &current_dom->assigned_harts);
	spin_unlock(&current_dom->assigned_harts_lock);

	sbi_update_hartindex_to_domain(hartindex, target_dom);

	spin_lock(&target_dom->assigned_harts_lock);
	sbi_hartmask_set_hartindex(hartindex, &target_dom->assigned_harts);
	spin_unlock(&target_dom->assigned_harts_lock);

	/* Reconfigure PMP settings for the new domain */
	for (int i = 0; i < pmp_count; i++) {
		/* Don't revoke firmware access permissions */
		if (sbi_hart_smepmp_is_fw_region(i))
			continue;

		sbi_platform_pmp_disable(sbi_platform_thishart_ptr(), i);
		pmp_disable(i);
	}
	sbi_hart_pmp_configure(scratch);

	/* Save current CSR context and restore target domain's CSR context */
	ctx->sstatus	= csr_swap(CSR_SSTATUS, dom_ctx->sstatus);
	ctx->sie	= csr_swap(CSR_SIE, dom_ctx->sie);
	ctx->stvec	= csr_swap(CSR_STVEC, dom_ctx->stvec);
	ctx->sscratch	= csr_swap(CSR_SSCRATCH, dom_ctx->sscratch);
	ctx->sepc	= csr_swap(CSR_SEPC, dom_ctx->sepc);
	ctx->scause	= csr_swap(CSR_SCAUSE, dom_ctx->scause);
	ctx->stval	= csr_swap(CSR_STVAL, dom_ctx->stval);
	ctx->sip	= csr_swap(CSR_SIP, dom_ctx->sip);
	ctx->satp	= csr_swap(CSR_SATP, dom_ctx->satp);
	if (sbi_hart_priv_version(scratch) >= SBI_HART_PRIV_VER_1_10)
		ctx->scounteren = csr_swap(CSR_SCOUNTEREN, dom_ctx->scounteren);
	if (sbi_hart_priv_version(scratch) >= SBI_HART_PRIV_VER_1_12)
		ctx->senvcfg	= csr_swap(CSR_SENVCFG, dom_ctx->senvcfg);
	if (sbi_hart_has_extension(scratch, SBI_HART_EXT_SSQOSID))
		ctx->srmcfg	= csr_swap(CSR_SRMCFG, dom_ctx->srmcfg);

	/* Save current trap state and restore target domain's trap state */
	trap_ctx = sbi_trap_get_context(scratch);
	sbi_memcpy(&ctx->trap_ctx, trap_ctx, sizeof(*trap_ctx));
	sbi_memcpy(trap_ctx, &dom_ctx->trap_ctx, sizeof(*trap_ctx));

	/* Mark current context structure initialized because context saved */
	ctx->initialized = true;

	/* If target domain context is not initialized or runnable */
	if (!dom_ctx->initialized) {
		/* Startup boot HART of target domain */
		if (current_hartid() == target_dom->boot_hartid)
			sbi_hart_switch_mode(target_dom->boot_hartid,
					     target_dom->next_arg1,
					     target_dom->next_addr,
					     target_dom->next_mode,
					     false);
		else
			sbi_hsm_hart_stop(scratch, true);
	}
	return 0;
}

static int hart_context_init(u32 hartindex)
{
	struct hart_context *ctx;
	struct sbi_domain *dom;

	sbi_domain_for_each(dom) {
		if (!sbi_hartmask_test_hartindex(hartindex,
						 dom->possible_harts))
			continue;

		ctx = sbi_zalloc(sizeof(struct hart_context));
		if (!ctx)
			return SBI_ENOMEM;

		/* Bind context and domain */
		ctx->dom = dom;
		hart_context_set(dom, hartindex, ctx);
	}

	return 0;
}

int sbi_domain_context_enter(struct sbi_domain *dom)
{
	int rc;
	struct hart_context *dom_ctx;
	struct hart_context *ctx = hart_context_thishart_get();

	/* Target domain must not be same as the current domain */
	if (!dom || dom == sbi_domain_thishart_ptr())
		return SBI_EINVAL;

	/*
	 * If it's first time to call `enter` on the current hart, no
	 * context allocated before. Allocate context for each valid
	 * domain on the current hart.
	 */
	if (!ctx) {
		rc = hart_context_init(current_hartindex());
		if (rc)
			return rc;

		ctx = hart_context_thishart_get();
		if (!ctx)
			return SBI_EINVAL;
	}

	dom_ctx = hart_context_get(dom, current_hartindex());
	/* Validate the domain context existence */
	if (!dom_ctx)
		return SBI_EINVAL;

	/* Update target context's previous context to indicate the caller */
	dom_ctx->prev_ctx = ctx;

	return switch_to_next_domain_context(ctx, dom_ctx);
}

int sbi_domain_context_exit(void)
{
	int rc;
	u32 hartindex = current_hartindex();
	struct sbi_domain *dom;
	struct hart_context *ctx = hart_context_thishart_get();
	struct hart_context *dom_ctx, *tmp;

	/*
	 * If it's first time to call `exit` on the current hart, no
	 * context allocated before. Loop through each domain to allocate
	 * its context on the current hart if valid.
	 */
	if (!ctx) {
		rc = hart_context_init(current_hartindex());
		if (rc)
			return rc;

		ctx = hart_context_thishart_get();
		if (!ctx)
			return SBI_EINVAL;
	}

	dom_ctx = ctx->prev_ctx;

	/* If no previous caller context */
	if (!dom_ctx) {
		/* Try to find next uninitialized user-defined domain's context */
		sbi_domain_for_each(dom) {
			if (dom == &root || dom == sbi_domain_thishart_ptr())
				continue;

			tmp = hart_context_get(dom, hartindex);
			if (tmp && !tmp->initialized) {
				dom_ctx = tmp;
				break;
			}
		}
	}

	/* Take the root domain context if fail to find */
	if (!dom_ctx)
		dom_ctx = hart_context_get(&root, hartindex);

	return switch_to_next_domain_context(ctx, dom_ctx);
}

int sbi_domain_context_init(void)
{
	/**
	 * Allocate per-domain and per-hart context data.
	 * The data type is "struct hart_context **" whose memory space will be
	 * dynamically allocated by domain_setup_data_one(). Calculate needed
	 * size of memory space here.
	 */
	dcpriv.data_size = sizeof(struct hart_context *) * sbi_hart_count();

	return sbi_domain_register_data(&dcpriv);
}

void sbi_domain_context_deinit(void)
{
	sbi_domain_unregister_data(&dcpriv);
}
