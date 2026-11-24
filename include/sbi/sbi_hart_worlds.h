/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2026 SiFive Inc.
 */

#ifndef __SBI_HART_WORLDS_H__
#define __SBI_HART_WORLDS_H__

struct sbi_scratch;

/**
 * Initialize WID hart protection for current HART
 *
 * Registers the WID protection mechanism if Smlwid
 * extension is present.
 *
 * @param scratch pointer to scratch space of current HART
 *
 * @return 0 on success and negative error code on failure
 */
int sbi_hart_worlds_init(struct sbi_scratch *scratch);

#endif /* __SBI_HART_WORLDS_H__ */
