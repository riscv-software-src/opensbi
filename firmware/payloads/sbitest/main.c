/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * S-mode test payload: print a greeting, wake up the other harts and power
 * off.
 */

#include <sbi/sbi_console.h>
#include "ecall.h"
#include "sbitest.h"

#define MAX_HARTID 15

__attribute__((noreturn)) void main(unsigned long hartid, unsigned long fdt)
{
	struct sbiret ret;
	long start_ret[MAX_HARTID + 1];
	unsigned int started = 0;
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
	 * Wake up the harts 0 to 15. Failures are not handled for now: the
	 * boot hart is already running and harts that do not exist are
	 * rejected. Every hart that starts enters _start, loses the lottery,
	 * prints '*' and waits.
	 */
	sbi_printf("\nWake up:       ");
	for (i = 0; i <= MAX_HARTID; i++) {
		ret	     = sbi_hart_start(i, (unsigned long)_start, 0);
		start_ret[i] = ret.error;
		if (!ret.error)
			started++;
	}

	/* Give the harts that were started time to arrive. */
	for (i = 0; i < 100000000UL && secondary_count < started; i++)
		asm volatile("" ::: "memory");

	sbi_printf("\n\nHarts started: %u, arrived: %u\n", started,
		   secondary_count);
	for (i = 0; i <= MAX_HARTID; i++)
		sbi_printf("  hart %2lu: %ld\n", i, start_ret[i]);

	sbi_printf("\nPowering off\n");

	sbi_system_reset(SBI_SRST_RESET_TYPE_SHUTDOWN,
			 SBI_SRST_RESET_REASON_NONE);

	/*
	 * The call does not return on success. Do not return either: OpenSBI
	 * does not expect a returning payload, and the platform may need some
	 * cycles to finish the shutdown.
	 */
	for (;;)
		asm volatile("wfi");
}
