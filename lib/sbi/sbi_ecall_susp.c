// SPDX-License-Identifier: BSD-2-Clause
#include <sbi/sbi_ecall.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_trap.h>
#include <sbi/sbi_system.h>

static int sbi_ecall_susp_handler(unsigned long extid, unsigned long funcid,
				  const struct sbi_trap_regs *regs,
				  unsigned long *out_val,
				  struct sbi_trap_info *out_trap)
{
	int ret = SBI_ENOTSUPP;

	if (funcid == SBI_EXT_SUSP_SUSPEND)
		ret = sbi_system_suspend(regs->a0, regs->a1, regs->a2);

	if (ret >= 0) {
		*out_val = ret;
		ret = 0;
	}

	return ret;
}

static int sbi_ecall_susp_probe(unsigned long extid, unsigned long *out_val)
{
	u32 type, count = 0;

	/*
	 * At least one suspend type should be supported by the
	 * platform for the SBI SUSP extension to be usable.
	 */
	for (type = 0; type <= SBI_SUSP_SLEEP_TYPE_LAST; type++) {
		if (sbi_system_suspend_supported(type))
			count++;
	}

	*out_val = count ? 1 : 0;
	return 0;
}

struct sbi_ecall_extension ecall_susp = {
	.extid_start = SBI_EXT_SUSP,
	.extid_end = SBI_EXT_SUSP,
	.handle = sbi_ecall_susp_handler,
	.probe = sbi_ecall_susp_probe,
};
