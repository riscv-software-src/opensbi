// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_encoding.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_const.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_platform.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/fdt/fdt_fixup.h>
#include <sbi_utils/irqchip/plic.h>
#include <sbi_utils/serial/uart8250.h>
#include <sbi_utils/sys/clint.h>

#define OPENPITON_DEFAULT_UART_ADDR		0xfff0c2c000
#define OPENPITON_DEFAULT_UART_FREQ		60000000
#define OPENPITON_DEFAULT_UART_BAUDRATE		115200
#define OPENPITON_DEFAULT_UART_REG_SHIFT	0
#define OPENPITON_DEFAULT_UART_REG_WIDTH	1
#define OPENPITON_DEFAULT_PLIC_ADDR		0xfff1100000
#define OPENPITON_DEFAULT_PLIC_NUM_SOURCES	2
#define OPENPITON_DEFAULT_HART_COUNT		3
#define OPENPITON_DEFAULT_CLINT_ADDR		0xfff1020000

#define SBI_OPENPITON_FEATURES	\
	(SBI_PLATFORM_HAS_TIMER_VALUE | \
	 SBI_PLATFORM_HAS_SCOUNTEREN | \
	 SBI_PLATFORM_HAS_MCOUNTEREN | \
	 SBI_PLATFORM_HAS_MFAULTS_DELEGATION)

static struct platform_uart_data uart = {
		OPENPITON_DEFAULT_UART_ADDR,
		OPENPITON_DEFAULT_UART_FREQ,
		OPENPITON_DEFAULT_UART_BAUDRATE,
	};
static struct platform_plic_data plic = {
		OPENPITON_DEFAULT_PLIC_ADDR,
		OPENPITON_DEFAULT_PLIC_NUM_SOURCES,
	};
static unsigned long clint_addr = OPENPITON_DEFAULT_CLINT_ADDR;

/*
 * OpenPiton platform early initialization.
 */
static int openpiton_early_init(bool cold_boot)
{
	void *fdt;
	struct platform_uart_data uart_data;
	struct platform_plic_data plic_data;
	unsigned long clint_data;
	int rc;

	if (!cold_boot)
		return 0;
	fdt = sbi_scratch_thishart_arg1_ptr();

	rc = fdt_parse_uart8250(fdt, &uart_data, "ns16550");
	if (!rc)
		uart = uart_data;

	rc = fdt_parse_plic(fdt, &plic_data, "riscv,plic0");
	if (!rc)
		plic = plic_data;

	rc = fdt_parse_clint(fdt, &clint_data, "riscv,clint0");
	if (!rc)
		clint_addr = clint_data;

	return 0;
}

/*
 * OpenPiton platform final initialization.
 */
static int openpiton_final_init(bool cold_boot)
{
	void *fdt;

	if (!cold_boot)
		return 0;

	fdt = sbi_scratch_thishart_arg1_ptr();
	fdt_fixups(fdt);

	return 0;
}

/*
 * Initialize the openpiton console.
 */
static int openpiton_console_init(void)
{
	return uart8250_init(uart.addr,
			     uart.freq,
			     uart.baud,
			     OPENPITON_DEFAULT_UART_REG_SHIFT,
			     OPENPITON_DEFAULT_UART_REG_WIDTH);
}

static int plic_openpiton_warm_irqchip_init(u32 target_hart,
			   int m_cntx_id, int s_cntx_id)
{
	size_t i, ie_words = plic.num_src / 32 + 1;

	if (target_hart >= OPENPITON_DEFAULT_HART_COUNT)
		return -1;
	/* By default, enable all IRQs for M-mode of target HART */
	if (m_cntx_id > -1) {
		for (i = 0; i < ie_words; i++)
			plic_set_ie(m_cntx_id, i, 1);
	}
	/* Enable all IRQs for S-mode of target HART */
	if (s_cntx_id > -1) {
		for (i = 0; i < ie_words; i++)
			plic_set_ie(s_cntx_id, i, 1);
	}
	/* By default, enable M-mode threshold */
	if (m_cntx_id > -1)
		plic_set_thresh(m_cntx_id, 1);
	/* By default, disable S-mode threshold */
	if (s_cntx_id > -1)
		plic_set_thresh(s_cntx_id, 0);

	return 0;
}

/*
 * Initialize the openpiton interrupt controller for current HART.
 */
static int openpiton_irqchip_init(bool cold_boot)
{
	u32 hartid = current_hartid();
	int ret;

	if (cold_boot) {
		ret = plic_cold_irqchip_init(plic.addr,
					     plic.num_src,
					     OPENPITON_DEFAULT_HART_COUNT);
		if (ret)
			return ret;
	}
	return plic_openpiton_warm_irqchip_init(hartid,
					2 * hartid, 2 * hartid + 1);
}

/*
 * Initialize IPI for current HART.
 */
static int openpiton_ipi_init(bool cold_boot)
{
	int ret;

	if (cold_boot) {
		ret = clint_cold_ipi_init(clint_addr,
					  OPENPITON_DEFAULT_HART_COUNT);
		if (ret)
			return ret;
	}

	return clint_warm_ipi_init();
}

/*
 * Initialize openpiton timer for current HART.
 */
static int openpiton_timer_init(bool cold_boot)
{
	int ret;

	if (cold_boot) {
		ret = clint_cold_timer_init(clint_addr,
					    OPENPITON_DEFAULT_HART_COUNT, TRUE);
		if (ret)
			return ret;
	}

	return clint_warm_timer_init();
}

/*
 * Reboot the openpiton.
 */
static int openpiton_system_reboot(u32 type)
{
	/* For now nothing to do. */
	sbi_printf("System reboot\n");
	return 0;
}

/*
 * Shutdown or poweroff the openpiton.
 */
static int openpiton_system_shutdown(u32 type)
{
	/* For now nothing to do. */
	sbi_printf("System shutdown\n");
	return 0;
}

/*
 * Platform descriptor.
 */
const struct sbi_platform_operations platform_ops = {
	.early_init = openpiton_early_init,
	.final_init = openpiton_final_init,
	.console_init = openpiton_console_init,
	.console_putc = uart8250_putc,
	.console_getc = uart8250_getc,
	.irqchip_init = openpiton_irqchip_init,
	.ipi_init = openpiton_ipi_init,
	.ipi_send = clint_ipi_send,
	.ipi_clear = clint_ipi_clear,
	.timer_init = openpiton_timer_init,
	.timer_value = clint_timer_value,
	.timer_event_start = clint_timer_event_start,
	.timer_event_stop = clint_timer_event_stop,
	.system_reboot = openpiton_system_reboot,
	.system_shutdown = openpiton_system_shutdown
};

const struct sbi_platform platform = {
	.opensbi_version = OPENSBI_VERSION,
	.platform_version = SBI_PLATFORM_VERSION(0x0, 0x01),
	.name = "OPENPITON RISC-V",
	.features = SBI_OPENPITON_FEATURES,
	.hart_count = OPENPITON_DEFAULT_HART_COUNT,
	.hart_stack_size = SBI_PLATFORM_DEFAULT_HART_STACK_SIZE,
	.platform_ops_addr = (unsigned long)&platform_ops
};
