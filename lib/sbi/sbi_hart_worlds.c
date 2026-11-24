/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2026 SiFive Inc.
 */

#include <sbi/riscv_encoding.h>
#include <sbi/sbi_bitops.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_hart_protection.h>
#include <sbi/sbi_hart_worlds.h>
#include <sbi/sbi_scratch.h>

/**
 * Initialize the per-hart slwid values of a domain using
 * the lowest delegated WID as the initial value.
 *
 * S-mode OS should choose the desire slwid for it's
 * U-mode.
 */
static int sbi_hart_worlds_state_setup(struct sbi_domain *dom,
				       struct sbi_domain_state *state,
				       void *state_ptr)
{
	unsigned long widdeleg = dom->widdeleg;
	unsigned long *slwid = state_ptr;
	unsigned long init_wid;

	if (!widdeleg)
		return SBI_OK;

	init_wid = sbi_ffs(widdeleg);
	sbi_for_each_hartindex(i)
		slwid[i] = init_wid;

	return SBI_OK;
}

static struct sbi_domain_state wid_state = {
	.state_setup = sbi_hart_worlds_state_setup,
};

static unsigned long *sbi_hart_worlds_slwid_ptr(struct sbi_domain *dom)
{
	unsigned long *slwid;

	slwid = sbi_domain_state_ptr(dom, &wid_state);
	if (!slwid || !sbi_hartindex_valid(current_hartindex()))
		return NULL;

	return &slwid[current_hartindex()];
}

static int hart_worlds_validate(struct sbi_scratch *scratch,
				struct sbi_domain *dom)
{
	struct sbi_hart_features *hf = sbi_hart_features_ptr(scratch);
	unsigned long allowed = -1UL;

	if (hf->has_pmlwidlist)
		allowed &= hf->pmlwidlist;
	if (sbi_hart_has_extension(scratch, SBI_HART_EXT_SMLWIDLIST))
		allowed &= csr_read(CSR_MLWIDLIST);

	if (!(allowed & BIT_ULL(dom->wid))) {
		sbi_printf("%s: domain%d: WID %u unauthorized for mlwid\n",
			   __func__, dom->index, dom->wid);
		return SBI_EINVAL;
	}

	/*
	 * mwiddeleg is a M-mode WARL register, so an S-mode OS must rely
	 * on the "widdeleg" DT property to determine which WIDs it may
	 * delegate to U-mode via slwid. If pmlwidlist or mlwidlist silently
	 * filters out any of those WIDs, the S-mode OS's view of the WIDs
	 * available for delegation would be incorrect. Treat this mismatch
	 * as a misconfiguration.
	 */
	if (sbi_hart_has_extension(scratch, SBI_HART_EXT_SMWIDDELEG) &&
	    (dom->widdeleg & ~allowed)) {
		sbi_printf("%s: domain%d: widdeleg 0x%llx unauthorized for"
			   " mwiddeleg\n", __func__, dom->index,
			   (unsigned long long)dom->widdeleg);
		return SBI_EINVAL;
	}

	return SBI_OK;
}

static int sbi_hart_worlds_configure(struct sbi_scratch *scratch,
				     struct sbi_domain *dom)
{
	unsigned long *slwid = NULL;
	int rc;

	if (!sbi_hart_has_extension(scratch, SBI_HART_EXT_SMLWID))
		return SBI_OK;

	rc = hart_worlds_validate(scratch, dom);
	if (rc)
		return rc;

	csr_write(CSR_MLWID, dom->wid);

	if (!sbi_hart_has_extension(scratch, SBI_HART_EXT_SMWIDDELEG))
		return SBI_OK;

	/*
	 * Enable Sswid before restoring slwid. A zero mwiddeleg
	 * disables access to slwid, so the order here is required.
	 */
	csr_write(CSR_MWIDDELEG, dom->widdeleg);

	if (dom->widdeleg) {
		slwid = sbi_hart_worlds_slwid_ptr(dom);
		if (!slwid)
			return SBI_EINVAL;
		csr_write(CSR_SLWID, *slwid);
	}

	return SBI_OK;
}

static void sbi_hart_worlds_unconfigure(struct sbi_scratch *scratch,
					struct sbi_domain *dom)
{
	unsigned long *slwid;

	if (!sbi_hart_has_extension(scratch, SBI_HART_EXT_SMWIDDELEG) ||
	    !dom->widdeleg)
		return;

	/*
	 * Save slwid value before disabling Sswid since the
	 * CSR value may not retain through the disabled period.
	 */
	slwid = sbi_hart_worlds_slwid_ptr(dom);
	if (!slwid) {
		sbi_panic("%s: domain%d has no slwid state\n",
			  __func__, dom->index);
	}
	*slwid = csr_read(CSR_SLWID);

	/* Disable Sswid */
	csr_write(CSR_MWIDDELEG, 0);
}

static struct sbi_hart_protection wid_protection = {
	.name		= "wid",
	.rating		= 100,
	.type		= SBI_HART_PROTECTION_TYPE_ID,
	.configure	= sbi_hart_worlds_configure,
	.unconfigure	= sbi_hart_worlds_unconfigure
};

int sbi_hart_worlds_init(struct sbi_scratch *scratch)
{
	int rc;

	if (!sbi_hart_has_extension(scratch, SBI_HART_EXT_SMLWID))
		return 0;

	if (sbi_hart_has_extension(scratch, SBI_HART_EXT_SMWIDDELEG)) {
		wid_state.state_size = sizeof(unsigned long) * sbi_hart_count();
		rc = sbi_domain_register_state(&wid_state);
		if (rc)
			return rc;
	}

	return sbi_hart_protection_register(&wid_protection);
}
