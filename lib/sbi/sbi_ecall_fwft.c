/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024 Rivos Inc.
 *
 * Authors:
 *   Clément Léger <cleger@rivosinc.com>
 */

#include <sbi/sbi_ecall.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_fwft.h>
#include <sbi/sbi_trap.h>

static int sbi_ecall_fwft_handler(unsigned long extid, unsigned long funcid,
				 struct sbi_trap_regs *regs,
				 struct sbi_ecall_return *out)
{
	int ret = 0;

	switch (funcid) {
	case SBI_EXT_FWFT_SET:
		ret = sbi_fwft_set(regs->a0, regs->a1, regs->a2);
		break;
	case SBI_EXT_FWFT_GET:
		ret = sbi_fwft_get(regs->a0, &out->value);
		break;
	default:
		ret = SBI_ENOTSUPP;
		break;
	}

	return ret;
}

struct sbi_ecall_extension ecall_fwft;

static int sbi_ecall_fwft_register_extensions(void)
{
	return sbi_ecall_register_extension(&ecall_fwft);
}

struct sbi_ecall_extension ecall_fwft = {
	.name			= "fwft",
	.extid_start		= SBI_EXT_FWFT,
	.extid_end		= SBI_EXT_FWFT,
	.register_extensions	= sbi_ecall_fwft_register_extensions,
	.handle			= sbi_ecall_fwft_handler,
};
