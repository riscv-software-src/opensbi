/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Helpers of the S-mode test payload.
 */

#include <sbi/sbi_console.h>
#include "ecall.h"
#include "sbitest.h"

void test_abort(const char *what)
{
	/* The last line of the output tells the result */
	sbi_printf("SBITEST: ERROR %s\n", what);

	sbi_system_reset(SBI_SRST_RESET_TYPE_SHUTDOWN,
			 SBI_SRST_RESET_REASON_SYSFAIL);

	/* Do not return, see main() */
	for (;;)
		asm volatile("wfi");
}

void test_finish(unsigned int errors, const char *skipped)
{
	sbi_printf("\nPowering off\n");

	/* The last line of the output tells the result */
	if (errors)
		sbi_printf("SBITEST: ERROR %u checks failed\n", errors);
	else if (skipped)
		sbi_printf("SBITEST: %s\n", skipped);
	else
		sbi_printf("SBITEST: OK\n");

	sbi_system_reset(SBI_SRST_RESET_TYPE_SHUTDOWN,
			 errors ? SBI_SRST_RESET_REASON_SYSFAIL
				: SBI_SRST_RESET_REASON_NONE);

	/* Do not return, see main() of the self test */
	for (;;)
		asm volatile("wfi");
}
