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
