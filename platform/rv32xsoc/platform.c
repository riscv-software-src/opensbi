/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2021 Chihiro Koyama(koyamanX)
 */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_encoding.h>
#include <sbi/sbi_const.h>
#include <sbi/sbi_platform.h>

/*
 * Include these files as needed.
 * See config.mk PLATFORM_xxx configuration parameters.
 */
#include <sbi_utils/irqchip/plic.h>
#include <sbi_utils/sys/clint.h>
#include <sbi_utils/serial/rv32xsoc_uart.h>

#define RV32XSOC_PLIC_ADDR		0xc000000
#define RV32XSOC_PLIC_NUM_SOURCES	32
#define RV32XSOC_HART_COUNT		1
#define RV32XSOC_CLINT_ADDR		0x2000000

static struct plic_data plic = {
	.addr = RV32XSOC_PLIC_ADDR,
	.num_src = RV32XSOC_PLIC_NUM_SOURCES,
};

static struct clint_data clint = {
	.addr = RV32XSOC_CLINT_ADDR,
	.first_hartid = 0,
	.hart_count = RV32XSOC_HART_COUNT,
	.has_64bit_mmio = FALSE,
};

/*
 * Platform early initialization.
 */
static int rv32xsoc_early_init(bool cold_boot)
{
	return 0;
}

/*
 * Platform final initialization.
 */
static int rv32xsoc_final_init(bool cold_boot)
{
	return 0;
}

/*
 * Initialize the rv32xsoc console.
 */
static int rv32xsoc_console_init(void)
{
	return rv32xsoc_uart_init();
}

/*
 * Write a character to the rv32xsoc console output.
 */
static void rv32xsoc_console_putc(char ch)
{
	rv32xsoc_uart_putchar(ch);
}

/*
 * Read a character from the rv32xsoc console input.
 */
static int rv32xsoc_console_getc(void)
{
	return rv32xsoc_uart_getchar();
}

/*
 * Initialize the rv32xsoc interrupt controller for current HART.
 */
static int rv32xsoc_irqchip_init(bool cold_boot)
{
	u32 hartid = current_hartid();
	int ret;

	/* Example if the generic PLIC driver is used */
	if (cold_boot) {
		ret = plic_cold_irqchip_init(&plic);
		if (ret)
			return ret;
	}

	return plic_warm_irqchip_init(&plic, 2 * hartid, 2 * hartid + 1);
}

/*
 * Initialize IPI for current HART.
 */
static int rv32xsoc_ipi_init(bool cold_boot)
{
	int ret;

	/* Example if the generic CLINT driver is used */
	if (cold_boot) {
		ret = clint_cold_ipi_init(&clint);
		if (ret)
			return ret;
	}

	return clint_warm_ipi_init();
}

/*
 * Send IPI to a target HART
 */
static void rv32xsoc_ipi_send(u32 target_hart)
{
	/* Example if the generic CLINT driver is used */
	clint_ipi_send(target_hart);
}

/*
 * Clear IPI for a target HART.
 */
static void rv32xsoc_ipi_clear(u32 target_hart)
{
	/* Example if the generic CLINT driver is used */
	clint_ipi_clear(target_hart);
}

/*
 * Initialize rv32xsoc timer for current HART.
 */
static int rv32xsoc_timer_init(bool cold_boot)
{
	int ret;

	/* Example if the generic CLINT driver is used */
	if (cold_boot) {
		ret = clint_cold_timer_init(&clint, NULL);
		if (ret)
			return ret;
	}

	return clint_warm_timer_init();
}

/*
 * Get rv32xsoc timer value.
 */
static u64 rv32xsoc_timer_value(void)
{
	/* Example if the generic CLINT driver is used */
	return clint_timer_value();
}

/*
 * Start rv32xsoc timer event for current HART.
 */
static void rv32xsoc_timer_event_start(u64 next_event)
{
	/* Example if the generic CLINT driver is used */
	clint_timer_event_start(next_event);
}

/*
 * Stop rv32xsoc timer event for current HART.
 */
static void rv32xsoc_timer_event_stop(void)
{
	/* Example if the generic CLINT driver is used */
	clint_timer_event_stop();
}

/*
 * Check reset type and reason supported by the rv32xsoc.
 */
static int rv32xsoc_system_reset_check(u32 type, u32 reason)
{
	return 0;
}

/*
 * Reset the rv32xsoc.
 */
static void rv32xsoc_system_reset(u32 type, u32 reason)
{
}

/*
 * Platform descriptor.
 */
const struct sbi_platform_operations platform_ops = {
	.early_init		= rv32xsoc_early_init,
	.final_init		= rv32xsoc_final_init,
	.console_putc		= rv32xsoc_console_putc,
	.console_getc		= rv32xsoc_console_getc,
	.console_init		= rv32xsoc_console_init,
	.irqchip_init		= rv32xsoc_irqchip_init,
	.ipi_send		= rv32xsoc_ipi_send,
	.ipi_clear		= rv32xsoc_ipi_clear,
	.ipi_init		= rv32xsoc_ipi_init,
	.timer_value		= rv32xsoc_timer_value,
	.timer_event_stop	= rv32xsoc_timer_event_stop,
	.timer_event_start	= rv32xsoc_timer_event_start,
	.timer_init		= rv32xsoc_timer_init,
	.system_reset_check	= rv32xsoc_system_reset_check,
	.system_reset		= rv32xsoc_system_reset
};
const struct sbi_platform platform = {
	.opensbi_version	= OPENSBI_VERSION,
	.platform_version	= SBI_PLATFORM_VERSION(0x0, 0x00),
	.name			= "rv32xsoc-name",
	.features		= SBI_PLATFORM_DEFAULT_FEATURES,
	.hart_count		= 1,
	.hart_stack_size	= SBI_PLATFORM_DEFAULT_HART_STACK_SIZE,
	.platform_ops_addr	= (unsigned long)&platform_ops
};
