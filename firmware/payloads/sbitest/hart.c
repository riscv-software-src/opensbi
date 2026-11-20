/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Secondary harts of the S-mode test payload.
 *
 * A hart that enters _start after the boot hart is parked in a wait loop in
 * start.S. It has no stack there. To wake it up, the boot hart gets a stack
 * for it with assign_stack() and sends it an IPI. The hart finds the stack in
 * secondary_stack[], switches to it and calls secondary_main().
 */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_encoding.h>
#include "heap.h"
#include "sbitest.h"
#include "trap.h"

unsigned long secondary_stack[MAX_HARTS];

volatile unsigned int secondary_awake;
volatile unsigned int secondary_on_stack;
volatile unsigned int secondary_done;
volatile unsigned int secondary_errors;

unsigned long assign_stack(unsigned long hartid)
{
	unsigned long expected = 0, top;
	void *base;

	if (hartid >= MAX_HARTS)
		return 0;

	top = __atomic_load_n(&secondary_stack[hartid], __ATOMIC_ACQUIRE);
	if (top)
		return top;

	base = malloc(HART_STACK_SIZE);
	if (!base)
		return 0;

	top = (unsigned long)base + HART_STACK_SIZE;

	/*
	 * Publish the stack. The release order makes the memory of the stack
	 * visible before the hart can see the address. If another call was
	 * faster, use its stack and give ours back.
	 */
	if (!__atomic_compare_exchange_n(&secondary_stack[hartid], &expected,
					 top, false, __ATOMIC_RELEASE,
					 __ATOMIC_ACQUIRE)) {
		free(base);
		return expected;
	}

	return top;
}

__attribute__((noreturn)) void secondary_main(unsigned long hartid)
{
	unsigned long top = secondary_stack[hartid];
	unsigned long sp;
	unsigned int errors;

	asm volatile("mv %0, sp" : "=r"(sp));

	/* The IPI that woke this hart is still pending, drop it */
	csr_clear(CSR_SIP, MIP_SSIP);
	trap_init();

	if (sp > top - HART_STACK_SIZE && sp <= top)
		__atomic_fetch_add(&secondary_on_stack, 1, __ATOMIC_RELEASE);
	__atomic_fetch_add(&secondary_awake, 1, __ATOMIC_RELEASE);

	errors = test_heap_stress(hartid);
	__atomic_fetch_add(&secondary_errors, errors, __ATOMIC_RELEASE);
	__atomic_fetch_add(&secondary_done, 1, __ATOMIC_RELEASE);

	for (;;)
		asm volatile("wfi");
}
