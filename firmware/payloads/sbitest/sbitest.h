/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Declarations of the S-mode test payload.
 */

#ifndef __SBITEST_H__
#define __SBITEST_H__

/*
 * Upper bound of the busy loops that wait for other harts. Normally they end
 * as soon as the harts are done. It is large because the harts of a virtual
 * machine can share a few host CPUs, and it only matters if something hangs.
 */
#define WAIT_SPINS 20000000000ULL

/*
 * The heap is assumed to be free memory: it begins 2 MiB behind the load
 * address of the payload and is 2 MiB long.
 */
#define HEAP_OFFSET (2UL * 1024 * 1024)
#define HEAP_SIZE (2UL * 1024 * 1024)

#ifndef __ASSEMBLER__

/* Link-time symbols and the entry point, see sbitest.elf.ldS and start.S */
extern char _payload_start[];
extern char _start[];

/* Incremented by every secondary hart that arrives in _start, see start.S */
extern volatile unsigned int secondary_count;

/* Route sbi_printf() to the SBI debug console (DBCN) */
void console_init(void);

/* Print the error, power off and do not return */
__attribute__((noreturn)) void test_abort(const char *what);

/* Tests, they return the number of failed checks */
unsigned int test_heap_basic(void);
unsigned int test_heap_stress(unsigned long hartid);
unsigned int test_fdt_harts(const void *fdt, unsigned long boot_hartid);

#endif /* __ASSEMBLER__ */

#endif /* __SBITEST_H__ */
