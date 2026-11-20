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
#include <sbi/sbi_string.h>
#include "dbtr.h"
#include "ecall.h"
#include "hart.h"
#include "heap.h"
#include "sbitest.h"
#include "trap.h"

unsigned long dbtr_mmode_base;
unsigned long dbtr_mmode_size;
unsigned long dbtr_boot_hartid;

/* Read a base or size of 1 or 2 cells */
static unsigned long long read_cells(const fdt32_t *p, int cells)
{
	unsigned long long val = 0;
	int i;

	for (i = 0; i < cells; i++)
		val = (val << 32) | fdt32_to_cpu(p[i]);

	return val;
}

/*
 * OpenSBI adds the memory it protects to /reserved-memory as nodes named
 * mmode_resv<N>@<address>. S-mode must not be able to use it as shared
 * memory. Remember the first one.
 */
static void find_mmode_region(const void *fdt)
{
	int resv, node, ac, sc, len;
	const char *name;
	const fdt32_t *reg;

	resv = fdt_path_offset(fdt, "/reserved-memory");
	if (resv < 0)
		return;

	ac = fdt_address_cells(fdt, resv);
	sc = fdt_size_cells(fdt, resv);
	if (ac < 1 || ac > 2 || sc < 1 || sc > 2)
		return;

	for (node = fdt_first_subnode(fdt, resv); node >= 0;
	     node = fdt_next_subnode(fdt, node)) {
		name = fdt_get_name(fdt, node, NULL);
		if (!name || sbi_strncmp(name, "mmode_resv", 10))
			continue;
		reg = fdt_getprop(fdt, node, "reg", &len);
		if (!reg || len < (ac + sc) * (int)sizeof(fdt32_t))
			continue;

		dbtr_mmode_base = read_cells(reg, ac);
		dbtr_mmode_size = read_cells(reg + ac, sc);
		return;
	}
}

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
	find_mmode_region((const void *)fdt);

	sbi_printf("Harts:         %u in the device tree\n", hart_count());
	for (hart = hart_first(); hart; hart = hart->next)
		sbi_printf("  hart %2lu: %u bit%s\n", hart->hartid,
			   hart->bitness,
			   hart->hartid == hartid ? " (boot hart)" : "");
	if (dbtr_mmode_size)
		sbi_printf("M-mode region: 0x%lx, size 0x%lx\n", dbtr_mmode_base,
			   dbtr_mmode_size);
	else
		sbi_printf("M-mode region: none in the device tree\n");

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
