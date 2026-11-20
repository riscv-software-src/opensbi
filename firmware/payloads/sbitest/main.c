/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * S-mode test payload: print a greeting, wake up the other harts and power
 * off.
 */

#include <sbi/riscv_encoding.h>
#include <sbi/sbi_console.h>
#include "ecall.h"
#include "heap.h"
#include "sbitest.h"
#include "trap.h"

static unsigned long breakpoints;

/* Count breakpoints and continue behind the ebreak instruction */
static int breakpoint_hook(struct trap_regs *regs)
{
	if (regs->scause != CAUSE_BREAKPOINT)
		return 0;

	breakpoints++;
	trap_skip_insn(regs);

	return 1;
}

__attribute__((noreturn)) void main(unsigned long hartid, unsigned long fdt)
{
	struct sbiret ret;
	long start_ret[MAX_HARTS];
	unsigned int started = 0;
	unsigned int failed;
	unsigned int errors = 0;
	unsigned long long spin;
	unsigned long i;

	console_init();

	sbi_printf("\nSBI test payload\n================\n\n");
	sbi_printf("Hello world\n\n");
	sbi_printf("Load address:  0x%016lx\n", (unsigned long)_payload_start);
	sbi_printf("Boot hart:     0x%lx\n", hartid);
	sbi_printf("Device-tree:   0x%016lx\n", fdt);

	ret = sbi_get_spec_version();
	if (ret.error)
		sbi_printf("SBI version:   error %ld\n", ret.error);
	else
		sbi_printf("SBI version:   %ld.%ld\n", (ret.value >> 24) & 0x7f,
			   ret.value & 0xffffff);

	/*
	 * Traps. Interrupts and exceptions are delegated to S-mode by OpenSBI,
	 * trap_init() sets up the vector. Provoke one of each: an IPI to this
	 * hart is a supervisor software interrupt, ebreak is a breakpoint.
	 */
	trap_init();
	trap_exception_hook = breakpoint_hook;

	ret = sbi_send_ipi(1UL, hartid);
	for (spin = 0; spin < WAIT_SPINS && !trap_ssip_count(); spin++)
		asm volatile("" ::: "memory");
	sbi_printf("\nIPI to self:   error %ld, interrupts handled %lu\n",
		   ret.error, trap_ssip_count());

	asm volatile("ebreak");
	sbi_printf("Breakpoint:    handled %lu\n", breakpoints);

	/*
	 * The heap. The memory is assumed to be free, see HEAP_OFFSET.
	 */
	heap_init(_payload_start + HEAP_OFFSET, HEAP_SIZE);
	failed = test_heap_basic();
	sbi_printf("\nHeap:          %u of the basic checks failed\n", failed);
	errors += failed;

	/*
	 * Start the harts 0 to 15. Failures are not handled for now: the boot
	 * hart is already running and harts that do not exist are rejected.
	 * Every hart that starts enters _start, loses the lottery, prints '*'
	 * and waits for a stack.
	 */
	sbi_printf("Start harts:   ");
	for (i = 0; i < MAX_HARTS; i++) {
		ret	     = sbi_hart_start(i, (unsigned long)_start, 0);
		start_ret[i] = ret.error;
		if (!ret.error)
			started++;
	}

	/* Give the harts that were started time to arrive. */
	for (spin = 0; spin < WAIT_SPINS && secondary_count < started; spin++)
		asm volatile("" ::: "memory");

	sbi_printf("\nHarts started: %u, arrived: %u\n", started,
		   secondary_count);

	/*
	 * Only the boot hart assigns stacks, one for each hart that was
	 * started, then wakes the hart up with an IPI. All of them run the
	 * heap test, and so does this hart, at the same time.
	 */
	for (i = 0; i < MAX_HARTS; i++) {
		if (start_ret[i])
			continue;
		if (!assign_stack(i)) {
			sbi_printf("  hart %2lu: no stack\n", i);
			errors++;
			started--;
			continue;
		}
		sbi_send_ipi(1UL, i);
	}

	failed = test_heap_stress(hartid);
	sbi_printf("Heap stress:   boot hart %u failed checks\n", failed);
	errors += failed;

	for (spin = 0; spin < WAIT_SPINS && secondary_done < started; spin++)
		asm volatile("" ::: "memory");

	sbi_printf("Woken up:      %u of %u, on own stack %u, done %u\n",
		   secondary_awake, started, secondary_on_stack,
		   secondary_done);
	sbi_printf("Heap stress:   other harts %u failed checks\n",
		   secondary_errors);
	errors += secondary_errors;
	if (secondary_awake != started || secondary_on_stack != started ||
	    secondary_done != started)
		errors++;

	/* All stacks are in use, all other memory has to be given back */
	sbi_printf("Allocations:   %lu (one stack per woken hart)\n",
		   heap_count());
	if (heap_count() != started)
		errors++;

	sbi_printf("\nPowering off\n");

	/* The last line of the output tells the result */
	if (errors)
		sbi_printf("SBITEST: ERROR %u checks failed\n", errors);
	else
		sbi_printf("SBITEST: OK\n");

	sbi_system_reset(SBI_SRST_RESET_TYPE_SHUTDOWN,
			 errors ? SBI_SRST_RESET_REASON_SYSFAIL
				: SBI_SRST_RESET_REASON_NONE);

	/*
	 * The call does not return on success. Do not return either: OpenSBI
	 * does not expect a returning payload, and the platform may need some
	 * cycles to finish the shutdown.
	 */
	for (;;)
		asm volatile("wfi");
}
