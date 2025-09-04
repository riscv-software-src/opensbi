/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2021 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_atomic.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_ipi.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_timer.h>
#include <sbi_utils/ipi/aclint_mswi.h>

static unsigned long mswi_ptr_offset;

#define mswi_get_hart_data_ptr(__scratch)				\
	sbi_scratch_read_type((__scratch), void *, mswi_ptr_offset)

#define mswi_set_hart_data_ptr(__scratch, __mswi)			\
	sbi_scratch_write_type((__scratch), void *, mswi_ptr_offset, (__mswi))

static void mswi_ipi_send(u32 hart_index)
{
	u32 *msip;
	struct sbi_scratch *scratch;
	struct aclint_mswi_data *mswi;

	scratch = sbi_hartindex_to_scratch(hart_index);
	if (!scratch)
		return;

	mswi = mswi_get_hart_data_ptr(scratch);
	if (!mswi)
		return;

	/* Set ACLINT IPI */
	msip = (void *)mswi->addr;
	writel_relaxed(1, &msip[sbi_hartindex_to_hartid(hart_index) -
			mswi->first_hartid]);
}

static void mswi_ipi_clear(void)
{
	u32 *msip;
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();
	struct aclint_mswi_data *mswi;

	mswi = mswi_get_hart_data_ptr(scratch);
	if (!mswi)
		return;

	/* Clear ACLINT IPI */
	msip = (void *)mswi->addr;
	writel_relaxed(0, &msip[current_hartid() - mswi->first_hartid]);
}

static struct sbi_ipi_device aclint_mswi = {
	.name = "aclint-mswi",
	.rating = 100,
	.ipi_send = mswi_ipi_send,
	.ipi_clear = mswi_ipi_clear
};

int aclint_mswi_cold_init(struct aclint_mswi_data *mswi)
{
	u32 i;
	int rc;
	struct sbi_scratch *scratch;

	/* Sanity checks */
	if (!mswi || (mswi->addr & (ACLINT_MSWI_ALIGN - 1)) ||
	    (mswi->size < (mswi->hart_count * sizeof(u32))) ||
	    (!mswi->hart_count || mswi->hart_count > ACLINT_MSWI_MAX_HARTS))
		return SBI_EINVAL;

	/* Allocate scratch space pointer */
	if (!mswi_ptr_offset) {
		mswi_ptr_offset = sbi_scratch_alloc_type_offset(void *);
		if (!mswi_ptr_offset)
			return SBI_ENOMEM;
	}

	/* Update MSWI pointer in scratch space */
	for (i = 0; i < mswi->hart_count; i++) {
		scratch = sbi_hartid_to_scratch(mswi->first_hartid + i);
		/*
		 * We don't need to fail if scratch pointer is not available
		 * because we might be dealing with hartid of a HART disabled
		 * in the device tree.
		 */
		if (!scratch)
			continue;
		mswi_set_hart_data_ptr(scratch, mswi);
	}

	/* Add MSWI regions to the root domain */
	rc = sbi_domain_root_add_memrange(mswi->addr, mswi->size, ACLINT_MSWI_ALIGN,
					  SBI_DOMAIN_MEMREGION_MMIO |
					  SBI_DOMAIN_MEMREGION_M_READABLE |
					  SBI_DOMAIN_MEMREGION_M_WRITABLE);
	if (rc)
		return rc;

	sbi_ipi_add_device(&aclint_mswi);

	return 0;
}
