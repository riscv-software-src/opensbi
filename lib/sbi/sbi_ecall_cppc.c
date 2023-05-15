/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Ventana Micro Systems Inc.
 *
 */

#include <sbi/sbi_ecall.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_trap.h>
#include <sbi/sbi_cppc.h>

static int sbi_ecall_cppc_handler(unsigned long extid, unsigned long funcid,
				  const struct sbi_trap_regs *regs,
				  unsigned long *out_val,
				  struct sbi_trap_info *out_trap)
{
	int ret = 0;
	uint64_t temp;

	switch (funcid) {
	case SBI_EXT_CPPC_READ:
		ret = sbi_cppc_read(regs->a0, &temp);
		*out_val = temp;
		break;
	case SBI_EXT_CPPC_READ_HI:
#if __riscv_xlen == 32
		ret = sbi_cppc_read(regs->a0, &temp);
		*out_val = temp >> 32;
#else
		*out_val = 0;
#endif
		break;
	case SBI_EXT_CPPC_WRITE:
		ret = sbi_cppc_write(regs->a0, regs->a1);
		break;
	case SBI_EXT_CPPC_PROBE:
		ret = sbi_cppc_probe(regs->a0);
		if (ret >= 0) {
			*out_val = ret;
			ret = 0;
		}
		break;
	default:
		ret = SBI_ENOTSUPP;
	}

	return ret;
}

struct sbi_ecall_extension ecall_cppc;

static int sbi_ecall_cppc_register_extensions(void)
{
	if (!sbi_cppc_get_device())
		return 0;

	return sbi_ecall_register_extension(&ecall_cppc);
}

struct sbi_ecall_extension ecall_cppc = {
	.extid_start		= SBI_EXT_CPPC,
	.extid_end		= SBI_EXT_CPPC,
	.register_extensions	= sbi_ecall_cppc_register_extensions,
	.handle			= sbi_ecall_cppc_handler,
};
