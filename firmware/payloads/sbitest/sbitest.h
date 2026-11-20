/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Declarations of the S-mode test payload.
 */

#ifndef __SBITEST_H__
#define __SBITEST_H__

/* Harts with an ID below this value are started and get a stack */
#define MAX_HARTS 16

/* Size of the stack of a secondary hart, it is allocated with malloc() */
#define HART_STACK_SIZE (128 * 1024)

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

/*
 * The top of the stack of a secondary hart, indexed by hart ID. The boot hart
 * sets it with assign_stack(), the parked hart waits for it, see start.S.
 */
extern unsigned long secondary_stack[MAX_HARTS];

/* Counters of the secondary harts that were woken up, see hart.c */
extern volatile unsigned int secondary_awake;
extern volatile unsigned int secondary_on_stack;
extern volatile unsigned int secondary_done;
extern volatile unsigned int secondary_errors;

/* Route sbi_printf() to the SBI debug console (DBCN) */
void console_init(void);

/*
 * Get a stack of HART_STACK_SIZE bytes for the hart from malloc() and publish
 * it in secondary_stack[]. Return the top of the stack, or 0 on failure. It
 * can be called from any hart. If the hart has a stack already, that one is
 * returned.
 */
unsigned long assign_stack(unsigned long hartid);

/* C entry of a secondary hart that was woken up, runs on the new stack */
__attribute__((noreturn)) void secondary_main(unsigned long hartid);

/* Print the error, power off and do not return */
__attribute__((noreturn)) void test_abort(const char *what);

/* Tests of the heap, return the number of failed checks */
unsigned int test_heap_basic(void);
unsigned int test_heap_stress(unsigned long hartid);

#endif /* __ASSEMBLER__ */

#endif /* __SBITEST_H__ */
