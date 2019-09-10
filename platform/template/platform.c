/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 */

#include <sbi/riscv_encoding.h>
#include <sbi/sbi_const.h>
#include <sbi/sbi_platform.h>

/*
 * Include these files as needed.
 * See config.mk PLATFORM_xxx configuration parameters.
 */
#include <sbi_utils/irqchip/plic.h>
#include <sbi_utils/serial/uart8250.h>
#include <sbi_utils/sys/clint.h>

/*
 * Platform early initialization.
 */
static int platform_early_init(bool cold_boot)
{
	return 0;
}

/*
 * Platform final initialization.
 */
static int platform_final_init(bool cold_boot)
{
	return 0;
}

/*
 * Get number of PMP regions for given HART.
 */
static u32 platform_pmp_region_count(u32 hartid)
{
	return 0;
}

/*
 * Get PMP regions details (namely: protection, base address, and size) for
 * a given HART.
 */
static int platform_pmp_region_info(u32 hartid, u32 index, ulong *prot,
				    ulong *addr, ulong *log2size)
{
	return 0;
}

/*
 * Initialize the platform console.
 */
static int platform_console_init(void)
{
	/* Example if the generic UART8250 driver is used */
	return uart8250_init(PLATFORM_UART_ADDR, PLATFORM_UART_SHIFTREG_ADDR,
			     PLATFORM_UART_BAUDRATE, 0, 1);
}

/*
 * Write a character to the platform console output.
 */
static void platform_console_putc(char ch)
{
	/* Example if the generic UART8250 driver is used */
	uart8250_putc(ch);
}

/*
 * Read a character from the platform console input.
 */
static int platform_console_getc(void)
{
	return uart8250_getc();
}

/*
 * Initialize the platform interrupt controller for current HART.
 */
static int platform_irqchip_init(bool cold_boot)
{
	u32 hartid = sbi_current_hartid();
	int ret;

	/* Example if the generic PLIC driver is used */
	if (cold_boot) {
		ret = plic_cold_irqchip_init(PLATFORM_PLIC_ADDR,
					     PLATFORM_PLIC_NUM_SOURCES,
					     PLATFORM_HART_COUNT);
		if (ret)
			return ret;
	}

	return plic_warm_irqchip_init(hartid, 2 * hartid, 2 * hartid + 1);
}

/*
 * Initialize IPI for current HART.
 */
static int platform_ipi_init(bool cold_boot)
{
	int ret;

	/* Example if the generic CLINT driver is used */
	if (cold_boot) {
		ret = clint_cold_ipi_init(PLATFORM_CLINT_ADDR,
					  PLATFORM_HART_COUNT);
		if (ret)
			return ret;
	}

	return clint_warm_ipi_init();
}

/*
 * Send IPI to a target HART
 */
static void platform_ipi_send(u32 target_hart)
{
	/* Example if the generic CLINT driver is used */
	clint_ipi_send(target_hart);
}

/*
 * Clear IPI for a target HART.
 */
static void platform_ipi_clear(u32 target_hart)
{
	/* Example if the generic CLINT driver is used */
	clint_ipi_clear(target_hart);
}

/*
 * Initialize platform timer for current HART.
 */
static int platform_timer_init(bool cold_boot)
{
	int ret;

	/* Example if the generic CLINT driver is used */
	if (cold_boot) {
		ret = clint_cold_timer_init(PLATFORM_CLINT_ADDR,
					    PLATFORM_HART_COUNT);
		if (ret)
			return ret;
	}

	return clint_warm_timer_init();
}

/*
 * Get platform timer value.
 */
static u64 platform_timer_value(void)
{
	/* Example if the generic CLINT driver is used */
	return clint_timer_value();
}

/*
 * Start platform timer event for current HART.
 */
static void platform_timer_event_start(u64 next_event)
{
	/* Example if the generic CLINT driver is used */
	clint_timer_event_start(next_event);
}

/*
 * Stop platform timer event for current HART.
 */
static void platform_timer_event_stop(void)
{
	/* Example if the generic CLINT driver is used */
	clint_timer_event_stop();
}

/*
 * Reboot the platform.
 */
static int platform_system_reboot(u32 type)
{
	return 0;
}

/*
 * Shutdown or poweroff the platform.
 */
static int platform_system_shutdown(u32 type)
{
	return 0;
}

/*
 * Platform descriptor.
 */
const struct sbi_platform_operations platform_ops = {
	.early_init		= platform_early_init,
	.final_init		= platform_final_init,
	.pmp_region_count	= platform_pmp_region_count,
	.pmp_region_info	= platform_pmp_region_info,
	.console_putc		= platform_console_putc,
	.console_getc		= platform_console_getc,
	.console_init		= platform_console_init,
	.irqchip_init		= platform_irqchip_init,
	.ipi_send		= platform_ipi_send,
	.ipi_clear		= platform_ipi_clear,
	.ipi_init		= platform_ipi_init,
	.timer_value		= platform_timer_value,
	.timer_event_stop	= platform_timer_event_stop,
	.timer_event_start	= platform_timer_event_start,
	.timer_init		= platform_timer_init,
	.system_reboot		= platform_system_down,
	.system_shutdown	= platform_system_down
};
const struct sbi_platform platform = {
	.opensbi_version	= OPENSBI_VERSION,
	.platform_version	= SBI_PLATFORM_VERSION(0x0, 0x00),
	.name			= "platform-name",
	.features		= SBI_PLATFORM_DEFAULT_FEATURES,
	.hart_count		= 1,
	.hart_stack_size	= 4096,
	.disabled_hart_mask	= 0,
	.tlb_range_flush_limit	= 0,
	.platform_ops_addr	= (unsigned long)&platform_ops
};
