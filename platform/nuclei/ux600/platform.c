/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) Nuclei Corporation or its affiliates.
 *
 * Authors:
 *   lujun <lujun@nucleisys.com>
 *   hqfang <578567190@qq.com>
 */

#include <libfdt.h>
#include <sbi/riscv_asm.h>
#include <sbi/riscv_io.h>
#include <sbi/riscv_encoding.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_const.h>
#include <sbi/sbi_platform.h>
#include <sbi_utils/fdt/fdt_fixup.h>
#include <sbi_utils/irqchip/plic.h>
#include <sbi_utils/serial/sifive-uart.h>
#include <sbi_utils/sys/clint.h>

/* clang-format off */

#define UX600_HART_COUNT		1

#define UX600_SYS_CLK			1000000000

/* Nuclei timer base address */
#define UX600_NUCLEI_TIMER_ADDR		0x2000000
/* The clint compatiable timer offset is 0x1000 against nuclei timer */
#define UX600_CLINT_TIMER_ADDR		(UX600_NUCLEI_TIMER_ADDR + 0x1000)

#define UX600_PLIC_ADDR			0x8000000
#define UX600_PLIC_NUM_SOURCES		0x35
#define UX600_PLIC_NUM_PRIORITIES	7

#define UX600_UART0_ADDR		0x10013000
#define UX600_UART1_ADDR		0x10023000

#define UX600_DEBUG_UART		UX600_UART0_ADDR
#define UX600_UART_BAUDRATE		115200

/* clang-format on */

static void ux600_modify_dt(void *fdt)
{
	fdt_fixups(fdt);
}

static int ux600_final_init(bool cold_boot)
{
	void *fdt;

	if (!cold_boot)
		return 0;

	fdt = sbi_scratch_thishart_arg1_ptr();
	ux600_modify_dt(fdt);

	return 0;
}

static int ux600_console_init(void)
{
	return sifive_uart_init(UX600_DEBUG_UART, UX600_SYS_CLK,
				UX600_UART_BAUDRATE);
}

static int ux600_irqchip_init(bool cold_boot)
{
	int rc;
	u32 hartid = current_hartid();

	if (cold_boot) {
		rc = plic_cold_irqchip_init(UX600_PLIC_ADDR,
					    UX600_PLIC_NUM_SOURCES,
					    UX600_HART_COUNT);
		if (rc)
			return rc;
	}

	return plic_warm_irqchip_init(hartid, (hartid) ? (2 * hartid - 1) : 0,
				      (hartid) ? (2 * hartid) : -1);
}

static int ux600_ipi_init(bool cold_boot)
{
	int rc;

	if (cold_boot) {
		rc = clint_cold_ipi_init(UX600_CLINT_TIMER_ADDR, UX600_HART_COUNT);
		if (rc)
			return rc;
	}

	return clint_warm_ipi_init();
}

static int ux600_timer_init(bool cold_boot)
{
	int rc;

	if (cold_boot) {
		rc = clint_cold_timer_init(UX600_CLINT_TIMER_ADDR,
					   UX600_HART_COUNT, TRUE);
		if (rc)
			return rc;
	}

	return clint_warm_timer_init();
}

static int ux600_system_reset(u32 type)
{
	/* For now nothing to do. */
	return 0;
}

const struct sbi_platform_operations platform_ops = {
	.final_init		= ux600_final_init,
	.console_putc		= sifive_uart_putc,
	.console_getc		= sifive_uart_getc,
	.console_init		= ux600_console_init,
	.irqchip_init		= ux600_irqchip_init,
	.ipi_send		= clint_ipi_send,
	.ipi_clear		= clint_ipi_clear,
	.ipi_init		= ux600_ipi_init,
	.timer_value		= clint_timer_value,
	.timer_event_stop	= clint_timer_event_stop,
	.timer_event_start	= clint_timer_event_start,
	.timer_init		= ux600_timer_init,
	.system_reset		= ux600_system_reset
};

const struct sbi_platform platform = {
	.opensbi_version	= OPENSBI_VERSION,
	.platform_version	= SBI_PLATFORM_VERSION(0x0U, 0x01U),
	.name			= "Nuclei UX600",
	.features		= SBI_PLATFORM_DEFAULT_FEATURES,
	.hart_count		= UX600_HART_COUNT,
	.hart_stack_size	= SBI_PLATFORM_DEFAULT_HART_STACK_SIZE,
	.platform_ops_addr	= (unsigned long)&platform_ops
};
