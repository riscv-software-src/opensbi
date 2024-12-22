/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Anup Patel <apatel@ventanamicro.com>
 */

#include <sbi/sbi_console.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_ecall.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_trap.h>
#include <sbi/riscv_asm.h>
#include <sbi/sbi_hart.h>

static int sbi_ecall_dbcn_handler(unsigned long extid, unsigned long funcid,
				  struct sbi_trap_regs *regs,
				  struct sbi_ecall_return *out)
{
	ulong smode = (csr_read(CSR_MSTATUS) & MSTATUS_MPP) >>
			MSTATUS_MPP_SHIFT;

	switch (funcid) {
	case SBI_EXT_DBCN_CONSOLE_WRITE:
	case SBI_EXT_DBCN_CONSOLE_READ:
		/*
		 * On RV32, the M-mode can only access the first 4GB of
		 * the physical address space because M-mode does not have
		 * MMU to access full 34-bit physical address space.
		 *
		 * Based on above, we simply fail if the upper 32bits of
		 * the physical address (i.e. a2 register) is non-zero on
		 * RV32.
		 *
		 * Analogously, we fail if the upper 64bit of the
		 * physical address (i.e. a2 register) is non-zero on
		 * RV64.
		 */
		if (regs->a2)
			return SBI_ERR_FAILED;

		if (!sbi_domain_check_addr_range(sbi_domain_thishart_ptr(),
					regs->a1, regs->a0, smode,
					SBI_DOMAIN_READ|SBI_DOMAIN_WRITE))
			return SBI_ERR_INVALID_PARAM;
		sbi_hart_map_saddr(regs->a1, regs->a0);
		if (funcid == SBI_EXT_DBCN_CONSOLE_WRITE)
			out->value = sbi_nputs((const char *)regs->a1, regs->a0);
		else
			out->value = sbi_ngets((char *)regs->a1, regs->a0);
		sbi_hart_unmap_saddr();
		return 0;
	case SBI_EXT_DBCN_CONSOLE_WRITE_BYTE:
		sbi_putc(regs->a0);
		return 0;
	default:
		break;
	}

	return SBI_ENOTSUPP;
}

struct sbi_ecall_extension ecall_dbcn;

static int sbi_ecall_dbcn_register_extensions(void)
{
	if (!sbi_console_get_device())
		return 0;

	return sbi_ecall_register_extension(&ecall_dbcn);
}

struct sbi_ecall_extension ecall_dbcn = {
	.name			= "dbcn",
	.extid_start		= SBI_EXT_DBCN,
	.extid_end		= SBI_EXT_DBCN,
	.register_extensions	= sbi_ecall_dbcn_register_extensions,
	.handle			= sbi_ecall_dbcn_handler,
};
