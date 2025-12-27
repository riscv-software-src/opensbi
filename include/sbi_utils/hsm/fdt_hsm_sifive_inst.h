/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 SiFive Inc.
 */

#ifndef __FDT_HSM_SIFIVE_INST_H__
#define __FDT_HSM_SIFIVE_INST_H__

static inline void sifive_cease(void)
{
	__asm__ __volatile__(".word 0x30500073" ::: "memory");
}

static inline void sifive_cflush(void)
{
	__asm__ __volatile__(".word 0xfc000073" ::: "memory");
}

#endif
