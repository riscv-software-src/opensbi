/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Declarations of the S-mode test payload.
 */

#ifndef __SBITEST_H__
#define __SBITEST_H__

/* Link-time symbols and the entry point, see sbitest.elf.ldS and start.S */
extern char _payload_start[];
extern char _start[];

/* Incremented by every secondary hart that arrives in _start, see start.S */
extern volatile unsigned int secondary_count;

/* Route sbi_printf() to the SBI debug console (DBCN) */
void console_init(void);

#endif /* __SBITEST_H__ */
