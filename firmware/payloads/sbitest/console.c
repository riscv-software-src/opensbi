/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Console of the S-mode test payload: sbi_printf() of the OpenSBI library
 * writes through the SBI debug console extension (DBCN).
 */

#include <sbi/sbi_console.h>
#include <sbi/sbi_hart.h>
#include "ecall.h"
#include "sbitest.h"

static unsigned long dbcn_puts(const char *str, unsigned long len)
{
	struct sbiret ret = sbi_debug_console_write(len, str);

	/*
	 * The caller retries until all bytes are written. Drop the text on
	 * an error, otherwise the caller would loop forever.
	 */
	if (ret.error)
		return len;

	return ret.value;
}

static const struct sbi_console_device dbcn_console = {
	.name	      = "dbcn",
	.console_puts = dbcn_puts,
};

void console_init(void)
{
	sbi_console_set_device(&dbcn_console);
}

/* The OpenSBI library calls this function in sbi_panic(). */
void sbi_hart_hang(void)
{
	for (;;)
		asm volatile("wfi");
}
