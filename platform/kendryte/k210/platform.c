/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Damien Le Moal <damien.lemoal@wdc.com>
 */

#include <sbi/riscv_encoding.h>
#include <sbi/sbi_const.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_console.h>
#include <sbi_utils/irqchip/plic.h>
#include <sbi_utils/sys/clint.h>
#include "platform.h"
#include "uarths.h"

#define K210_UART_BAUDRATE 115200

static int k210_console_init(void)
{
	uarths_init(K210_UART_BAUDRATE, UARTHS_STOP_1);

	return 0;
}

static void k210_console_putc(char c)
{
	uarths_putc(c);
}

static int k210_console_getc(void)
{
	return uarths_getc();
}

static int k210_irqchip_init(bool cold_boot)
{
	int rc;
	u32 hartid = sbi_current_hartid();

	if (cold_boot) {
		rc = plic_cold_irqchip_init(PLIC_BASE_ADDR, PLIC_NUM_SOURCES,
					    K210_HART_COUNT);
		if (rc)
			return rc;
	}

	return plic_warm_irqchip_init(hartid, (2 * hartid), (2 * hartid + 1));
}

static int k210_ipi_init(bool cold_boot)
{
	int rc;

	if (cold_boot) {
		rc = clint_cold_ipi_init(CLINT_BASE_ADDR, K210_HART_COUNT);
		if (rc)
			return rc;
	}

	return clint_warm_ipi_init();
}

static int k210_timer_init(bool cold_boot)
{
	int rc;

	if (cold_boot) {
		rc = clint_cold_timer_init(CLINT_BASE_ADDR, K210_HART_COUNT);
		if (rc)
			return rc;
	}

	return clint_warm_timer_init();
}

static int k210_system_reboot(u32 type)
{
	/* For now nothing to do. */
	sbi_printf("System reboot\n");

	return 0;
}

static int k210_system_shutdown(u32 type)
{
	/* For now nothing to do. */
	sbi_printf("System shutdown\n");

	return 0;
}

const struct sbi_platform_operations platform_ops = {
	.console_init	= k210_console_init,
	.console_putc	= k210_console_putc,
	.console_getc	= k210_console_getc,

	.irqchip_init = k210_irqchip_init,

	.ipi_init  = k210_ipi_init,
	.ipi_send  = clint_ipi_send,
	.ipi_sync  = clint_ipi_sync,
	.ipi_clear = clint_ipi_clear,

	.timer_init	   = k210_timer_init,
	.timer_value	   = clint_timer_value,
	.timer_event_stop  = clint_timer_event_stop,
	.timer_event_start = clint_timer_event_start,

	.system_reboot	 = k210_system_reboot,
	.system_shutdown = k210_system_shutdown
};

const struct sbi_platform platform = {
	.opensbi_version	= OPENSBI_VERSION,
	.platform_version   	= SBI_PLATFORM_VERSION(0x0, 0x01),
	.name			= "Kendryte K210",
	.features		= SBI_PLATFORM_HAS_TIMER_VALUE,
	.hart_count		= K210_HART_COUNT,
	.hart_stack_size	= K210_HART_STACK_SIZE,
	.disabled_hart_mask	= 0,
	.platform_ops_addr	= (unsigned long)&platform_ops
};
