/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <sbi/riscv_io.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi_utils/sys/sifive_test.h>

#define FINISHER_FAIL		0x3333
#define FINISHER_PASS		0x5555
#define FINISHER_RESET		0x7777

static void *sifive_test_base;

int sifive_test_system_reset_check(u32 type, u32 reason)
{
	switch (type) {
	case SBI_SRST_RESET_TYPE_SHUTDOWN:
	case SBI_SRST_RESET_TYPE_COLD_REBOOT:
	case SBI_SRST_RESET_TYPE_WARM_REBOOT:
		return 1;
	}

	return 0;
}

void sifive_test_system_reset(u32 type, u32 reason)
{
	/*
	 * Tell the "finisher" that the simulation
	 * was successful so that QEMU exits
	 */
	switch (type) {
	case SBI_SRST_RESET_TYPE_SHUTDOWN:
		if (reason == SBI_SRST_RESET_REASON_NONE)
			writew(FINISHER_PASS, sifive_test_base);
		else
			writew(FINISHER_FAIL, sifive_test_base);
		break;
	case SBI_SRST_RESET_TYPE_COLD_REBOOT:
	case SBI_SRST_RESET_TYPE_WARM_REBOOT:
		writew(FINISHER_RESET, sifive_test_base);
		break;
	}
}

int sifive_test_init(unsigned long base)
{
	sifive_test_base = (void *)base;

	return 0;
}
