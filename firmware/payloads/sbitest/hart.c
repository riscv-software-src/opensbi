/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Harts of the S-mode test payload.
 *
 * A hart that enters _start after the boot hart is parked in a wait loop in
 * start.S. It has no stack there. It finds its own node in the list of harts
 * and waits until the boot hart has put the top of a stack into it. To wake
 * it up, the boot hart gets a stack for it with assign_stack() and sends it
 * an IPI. The hart switches to the stack and calls secondary_main().
 */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_encoding.h>
#include <sbi/sbi_types.h>
#include "hart.h"
#include "heap.h"
#include "sbitest.h"
#include "trap.h"

/* start.S reads the list with these offsets */
_Static_assert(offsetof(struct hart, next) == HART_NEXT * sizeof(unsigned long),
	       "HART_NEXT");
_Static_assert(offsetof(struct hart, hartid) == HART_ID * sizeof(unsigned long),
	       "HART_ID");
_Static_assert(offsetof(struct hart, stack_top) ==
		       HART_STACK * sizeof(unsigned long),
	       "HART_STACK");

struct hart *hart_list;
static unsigned int hart_total;

volatile unsigned int secondary_awake;
volatile unsigned int secondary_on_stack;
volatile unsigned int secondary_done;
volatile unsigned int secondary_errors;

void harts_init(const void *fdt)
{
	struct hart *list;
	unsigned int count;

	list	   = harts_parse(fdt, &count);
	hart_total = count;

	/*
	 * Publish the list. The release order makes the nodes visible before a
	 * parked hart can see the head of the list.
	 */
	__atomic_store_n(&hart_list, list, __ATOMIC_RELEASE);
}

struct hart *hart_first(void)
{
	return __atomic_load_n(&hart_list, __ATOMIC_ACQUIRE);
}

unsigned int hart_count(void)
{
	return hart_total;
}

struct hart *hart_find(unsigned long hartid)
{
	struct hart *hart;

	for (hart = hart_first(); hart; hart = hart->next)
		if (hart->hartid == hartid)
			return hart;

	return NULL;
}

int hart_can_run(const struct hart *hart)
{
	return hart->bitness == OWN_BITNESS;
}

unsigned long assign_stack(unsigned long hartid)
{
	struct hart *hart      = hart_find(hartid);
	unsigned long expected = 0, top;
	void *base;

	if (!hart)
		return 0;

	top = __atomic_load_n(&hart->stack_top, __ATOMIC_ACQUIRE);
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
	if (!__atomic_compare_exchange_n(&hart->stack_top, &expected, top,
					 false, __ATOMIC_RELEASE,
					 __ATOMIC_ACQUIRE)) {
		free(base);
		return expected;
	}

	return top;
}

__attribute__((noreturn)) void secondary_main(struct hart *hart)
{
	unsigned long top = hart->stack_top;
	unsigned long sp;
	unsigned int errors;

	asm volatile("mv %0, sp" : "=r"(sp));

	/* The IPI that woke this hart is still pending, drop it */
	csr_clear(CSR_SIP, MIP_SSIP);
	trap_init();

	if (sp > top - HART_STACK_SIZE && sp <= top)
		__atomic_fetch_add(&secondary_on_stack, 1, __ATOMIC_RELEASE);
	__atomic_fetch_add(&secondary_awake, 1, __ATOMIC_RELEASE);

	errors = test_heap_stress(hart->hartid);
	__atomic_fetch_add(&secondary_errors, errors, __ATOMIC_RELEASE);
	__atomic_fetch_add(&secondary_done, 1, __ATOMIC_RELEASE);

	for (;;)
		asm volatile("wfi");
}
