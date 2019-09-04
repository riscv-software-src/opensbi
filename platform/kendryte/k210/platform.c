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
#include <sbi_utils/serial/sifive-uart.h>
#include "platform.h"

static u32 k210_get_clk_freq(void)
{
	u32 clksel0, pll0;
	u64 pll0_freq, clkr0, clkf0, clkod0, div;

	/*
	 * If the clock selector is not set, use the base frequency.
	 * Otherwise, use PLL0 frequency with a frequency divisor.
	 */
	clksel0 = k210_read_sysreg(K210_CLKSEL0);
	if (!(clksel0 & 0x1))
		return K210_CLK0_FREQ;

	/*
	 * Get PLL0 frequency:
	 * freq = base frequency * clkf0 / (clkr0 * clkod0)
	 */
	pll0 = k210_read_sysreg(K210_PLL0);
	clkr0 = 1 + (pll0 & 0x0000000f);
	clkf0 = 1 + ((pll0 & 0x000003f0) >> 4);
	clkod0 = 1 + ((pll0 & 0x00003c00) >> 10);
	pll0_freq = clkf0 * K210_CLK0_FREQ / (clkr0 * clkod0);

	/* Get the frequency divisor from the clock selector */
	div = 2ULL << ((clksel0 & 0x00000006) >> 1);

	return pll0_freq / div;
}

static int k210_console_init(void)
{
	return sifive_uart_init(K210_UART_BASE_ADDR, k210_get_clk_freq(),
				K210_UART_BAUDRATE);
}

static int k210_irqchip_init(bool cold_boot)
{
	int rc;
	u32 hartid = sbi_current_hartid();

	if (cold_boot) {
		rc = plic_cold_irqchip_init(K210_PLIC_BASE_ADDR,
					    K210_PLIC_NUM_SOURCES,
					    K210_HART_COUNT);
		if (rc)
			return rc;
	}

	return plic_warm_irqchip_init(hartid, hartid * 2, hartid * 2 + 1);
}

static int k210_ipi_init(bool cold_boot)
{
	int rc;

	if (cold_boot) {
		rc = clint_cold_ipi_init(K210_CLINT_BASE_ADDR,
					 K210_HART_COUNT);
		if (rc)
			return rc;
	}

	return clint_warm_ipi_init();
}

static int k210_timer_init(bool cold_boot)
{
	int rc;

	if (cold_boot) {
		rc = clint_cold_timer_init(K210_CLINT_BASE_ADDR,
					   K210_HART_COUNT);
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
	.console_putc	= sifive_uart_putc,
	.console_getc	= sifive_uart_getc,

	.irqchip_init = k210_irqchip_init,

	.ipi_init  = k210_ipi_init,
	.ipi_send  = clint_ipi_send,
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
