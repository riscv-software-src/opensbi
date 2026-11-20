/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * S-mode test of the SBI debug trigger extension (DBTR).
 *
 * main() sets up the payload and runs test_dbtr_hart() on the boot hart. If
 * the device tree has other harts, they run it too, each with its own
 * triggers and its own shared memory. A system with a single hart is fine:
 * the boot hart is the only one that runs the test, and there is nothing to
 * wait for.
 *
 * Without the extension the test is skipped. That is not an error, a platform
 * need not have debug triggers.
 */

#include <libfdt.h>
#include <sbi/riscv_encoding.h>
#include <sbi/sbi_console.h>
#include "dbtr.h"
#include "ecall.h"
#include "hart.h"
#include "heap.h"
#include "sbitest.h"
#include "trap.h"

unsigned long dbtr_boot_hartid;

__attribute__((noreturn)) void main(unsigned long hartid, unsigned long fdt)
{
	unsigned int started, skipped, woken = 0, errors = 0;
	struct sbiret ret;
	struct hart *hart;

	console_init();
	sbi_printf("\nSBI test payload: DBTR\n======================\n\n");
	sbi_printf("Boot hart:     0x%lx\n", hartid);

	dbtr_boot_hartid = hartid;
	trap_init();
	trap_exception_hook = dbtr_trap_hook;
	heap_init(_payload_start + HEAP_OFFSET, HEAP_SIZE);

	if (fdt_check_header((const void *)fdt))
		test_abort("no valid device tree");
	harts_init((const void *)fdt);

	sbi_printf("Harts:         %u in the device tree\n", hart_count());
	for (hart = hart_first(); hart; hart = hart->next)
		sbi_printf("  hart %2lu: %u bit%s\n", hart->hartid,
			   hart->bitness,
			   hart->hartid == hartid ? " (boot hart)" : "");

	/* Debug triggers are optional, the device tree tells which hart has them */
	hart = hart_find(hartid);
	sbi_printf("Sdtrig:        %s in the device tree for the boot hart\n",
		   hart && hart->sdtrig ? "listed" : "not listed");
	if (!hart || !hart->sdtrig) {
		sbi_printf("\nThe boot hart has no Sdtrig\n");
		test_finish(0, "SKIPPED no Sdtrig in the device tree");
	}

	ret = sbi_probe_extension(SBI_EXT_DBTR);
	if (ret.error || !ret.value) {
		sbi_printf("\nDBTR is not available\n");
		test_finish(0, "SKIPPED DBTR is not available");
	}

	/*
	 * The other harts, if there are any. A hart that cannot be started
	 * is not tested, but it is not an error either.
	 */
	started = harts_start_secondary(hartid, &skipped);
	sbi_printf("\nHarts started: %u, arrived: %u, skipped: %u\n", started,
		   secondary_count, skipped);
	if (started)
		woken = harts_wake_secondary(test_dbtr_hart);

	errors += test_dbtr_hart(hartid);

	if (started) {
		errors += harts_wait_secondary(woken);
		if (woken != started)
			errors++;
		sbi_printf("Other harts:   %u of %u done\n", secondary_done,
			   started);
	}

	test_finish(errors, NULL);
}
