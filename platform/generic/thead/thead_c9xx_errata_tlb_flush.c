/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Authors:
 *   Inochi Amaoto <inochiama@outlook.com>
 *
 */

#include <sbi/riscv_encoding.h>
#include <sbi/riscv_asm.h>

void _thead_tlb_flush_fixup_trap_handler(void);

void thead_register_tlb_flush_trap_handler(void)
{
	csr_write(CSR_MTVEC, &_thead_tlb_flush_fixup_trap_handler);
}
