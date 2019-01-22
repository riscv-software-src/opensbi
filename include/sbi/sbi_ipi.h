/*
 * Copyright (c) 2018 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef __SBI_IPI_H__
#define __SBI_IPI_H__

#include <sbi/sbi_types.h>

#define SBI_IPI_EVENT_SOFT			0x1
#define SBI_IPI_EVENT_FENCE_I			0x2
#define SBI_IPI_EVENT_SFENCE_VMA		0x4
#define SBI_IPI_EVENT_HALT			0x8

struct sbi_scratch;

int sbi_ipi_send_many(struct sbi_scratch *scratch,
		      ulong *pmask, u32 event);

void sbi_ipi_clear_smode(struct sbi_scratch *scratch);

void sbi_ipi_process(struct sbi_scratch *scratch);

int sbi_ipi_init(struct sbi_scratch *scratch, bool cold_boot);

#endif
