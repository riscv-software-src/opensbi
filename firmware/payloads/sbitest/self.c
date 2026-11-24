/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Self test of the S-mode test payload: print a greeting, test the traps,
 * the heap and the hart list, wake up the other harts and power off.
 */

#include <sbi/riscv_encoding.h>
#include <sbi/sbi_console.h>
#include <libfdt.h>
#include "ecall.h"
#include "hart.h"
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
	struct hart *hart;
	unsigned int started = 0, skipped = 0, woken;
	unsigned int failed;
	unsigned int errors = 0;
	unsigned long long spin;

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
	 * The harts come from the device tree: children of /cpus with the
	 * compatible string "riscv". Their reg property is the hart ID, the
	 * property riscv,isa-base is the bitness. This payload is built for
	 * one bitness and cannot run on harts of the other one.
	 */
	if (fdt_check_header((const void *)fdt))
		test_abort("no valid device tree");
	harts_init((const void *)fdt);

	sbi_printf("Own bitness:   %u\n", OWN_BITNESS);
	sbi_printf("Harts:         %u in the device tree\n", hart_count());
	for (hart = hart_first(); hart; hart = hart->next)
		sbi_printf("  hart %2lu: %u bit%s\n", hart->hartid,
			   hart->bitness,
			   hart->hartid == hartid ? " (boot hart)" : "");

	failed = test_fdt_harts((const void *)fdt, hartid);
	sbi_printf("Hart list:     %u of the checks failed\n", failed);
	errors += failed;

	/*
	 * Start all other harts that can run this payload. Failures are not
	 * handled for now. Every hart that starts enters _start, loses the
	 * lottery, prints '*' and waits for a stack.
	 */
	sbi_printf("Start harts:   ");
	for (hart = hart_first(); hart; hart = hart->next) {
		if (hart->hartid == hartid)
			continue;
		if (!hart_can_run(hart)) {
			skipped++;
			continue;
		}
		ret = sbi_hart_start(hart->hartid, (unsigned long)_start, 0);
		if (!ret.error) {
			hart->started = 1;
			started++;
		}
	}

	/* Give the harts that were started time to arrive. */
	for (spin = 0; spin < WAIT_SPINS && secondary_count < started; spin++)
		asm volatile("" ::: "memory");

	sbi_printf("\nHarts started: %u, arrived: %u, skipped: %u\n", started,
		   secondary_count, skipped);

	/*
	 * Only the boot hart assigns stacks, one for each hart that was
	 * started, then wakes the hart up with an IPI. All of them run the
	 * heap test, and so does this hart, at the same time.
	 */
	/* Set before the first wake-up, the harts read it right away */
	__atomic_store_n(&secondary_task, test_heap_stress, __ATOMIC_RELEASE);

	woken = 0;
	for (hart = hart_first(); hart; hart = hart->next) {
		if (!hart->started)
			continue;
		if (!assign_stack(hart->hartid)) {
			sbi_printf("  hart %2lu: no stack\n", hart->hartid);
			errors++;
			continue;
		}
		sbi_send_ipi(1UL, hart->hartid);
		woken++;
	}

	failed = test_heap_stress(hartid);
	sbi_printf("Heap stress:   boot hart %u failed checks\n", failed);
	errors += failed;

	for (spin = 0; spin < WAIT_SPINS && secondary_done < woken; spin++)
		asm volatile("" ::: "memory");

	sbi_printf("Woken up:      %u of %u, on own stack %u, done %u\n",
		   secondary_awake, woken, secondary_on_stack, secondary_done);
	sbi_printf("Heap stress:   other harts %u failed checks\n",
		   secondary_errors);
	errors += secondary_errors;
	if (woken != started || secondary_awake != woken ||
	    secondary_on_stack != woken || secondary_done != woken)
		errors++;

	/*
	 * The stacks are in use and the list of harts is still allocated.
	 * Everything else has to be given back.
	 */
	sbi_printf("Allocations:   %lu (%u harts, %u stacks)\n", heap_count(),
		   hart_count(), woken);
	if (heap_count() != hart_count() + woken)
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
