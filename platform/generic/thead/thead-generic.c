/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Authors:
 *   Inochi Amaoto <inochiama@outlook.com>
 *
 */

#include <platform_override.h>
#include <sbi/riscv_barrier.h>
#include <sbi/sbi_const.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_string.h>
#include <sbi_utils/fdt/fdt_helper.h>

/**
 * T-HEAD board with this quirk need to execute sfence.vma to flush
 * stale entrie avoid incorrect memory access.
 */
#define THEAD_QUIRK_TLB_FLUSH_FIXUP		BIT(0)

void _thead_tlb_flush_fixup_trap_handler(void);

void thead_register_tlb_flush_trap_handler(void)
{
	csr_write(CSR_MTVEC, &_thead_tlb_flush_fixup_trap_handler);
}

static int thead_generic_early_init(bool cold_boot,
				    const struct fdt_match *match)
{
	unsigned long quirks = (unsigned long)match->data;

	if (quirks & THEAD_QUIRK_TLB_FLUSH_FIXUP)
		thead_register_tlb_flush_trap_handler();

	return 0;
}

static const struct fdt_match thead_generic_match[] = {
	{ .compatible = "thead,th1520",
	  .data = (void*)THEAD_QUIRK_TLB_FLUSH_FIXUP },
	{ },
};

const struct platform_override thead_generic = {
	.match_table	= thead_generic_match,
	.early_init	= thead_generic_early_init,
};
