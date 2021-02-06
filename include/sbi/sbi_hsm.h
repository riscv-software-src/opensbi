/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Atish Patra <atish.patra@wdc.com>
 */

#ifndef __SBI_HSM_H__
#define __SBI_HSM_H__

#include <sbi/sbi_types.h>

struct sbi_domain;
struct sbi_scratch;

int sbi_hsm_init(struct sbi_scratch *scratch, u32 hartid, bool cold_boot);
void __noreturn sbi_hsm_exit(struct sbi_scratch *scratch);

int sbi_hsm_hart_start(struct sbi_scratch *scratch,
		       const struct sbi_domain *dom,
		       u32 hartid, ulong saddr, ulong smode, ulong priv);
int sbi_hsm_hart_stop(struct sbi_scratch *scratch, bool exitnow);
void sbi_hsm_hart_resume_start(struct sbi_scratch *scratch);
void sbi_hsm_hart_resume_finish(struct sbi_scratch *scratch);
int sbi_hsm_hart_suspend(struct sbi_scratch *scratch, u32 suspend_type,
			 ulong raddr, ulong rmode, ulong priv);
int sbi_hsm_hart_get_state(const struct sbi_domain *dom, u32 hartid);
int sbi_hsm_hart_interruptible_mask(const struct sbi_domain *dom,
				    ulong hbase, ulong *out_hmask);
void sbi_hsm_prepare_next_jump(struct sbi_scratch *scratch, u32 hartid);

#endif
