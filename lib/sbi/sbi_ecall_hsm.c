/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Atish Patra <atish.patra@wdc.com>
 */

#include <sbi/sbi_ecall.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_version.h>
#include <sbi/sbi_hsm.h>
#include <sbi/sbi_scratch.h>
#include <sbi/riscv_asm.h>

static int sbi_ecall_hsm_handler(unsigned long extid, unsigned long funcid,
				 unsigned long *args, unsigned long *out_val,
				 struct sbi_trap_info *out_trap)
{
	int ret = 0, hstate;
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();

	switch (funcid) {
	case SBI_EXT_HSM_HART_START:
		ret = sbi_hsm_hart_start(scratch, args[0], args[1], args[2]);
		break;
	case SBI_EXT_HSM_HART_STOP:
		ret = sbi_hsm_hart_stop(scratch, TRUE);
		break;
	case SBI_EXT_HSM_HART_GET_STATUS:
		hstate = sbi_hsm_hart_get_state(args[0]);
		ret = sbi_hsm_hart_state_to_status(hstate);
		break;
	default:
		ret = SBI_ENOTSUPP;
	};
	if (ret >= 0) {
		*out_val = ret;
		ret = 0;
	}

	return ret;
}

struct sbi_ecall_extension ecall_hsm = {
	.extid_start = SBI_EXT_HSM,
	.extid_end = SBI_EXT_HSM,
	.handle = sbi_ecall_hsm_handler,
};
