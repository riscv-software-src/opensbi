/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 SiFive Inc.
 */

#ifndef __FDT_HSM_SIFIVE_INST_H__
#define __FDT_HSM_SIFIVE_INST_H__

static inline void sifive_cease(void)
{
	__asm__ __volatile__(".insn 0x30500073" ::: "memory");
}

static inline void sifive_cflush(void)
{
	__asm__ __volatile__(".insn 0xfc000073" ::: "memory");
}

#endif
