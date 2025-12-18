/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 Bo Gan <ganboing@gmail.com>
 *
 */

#include <sbi/riscv_io.h>
#include <sbi/sbi_string.h>
#include <sbi/sbi_system.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi_utils/serial/uart8250.h>
#include <eswin/eic770x.h>
#include <eswin/hfp.h>

/* HFP -> HiFive Premier P550 */

#define HFP_MCU_UART_PORT	2
#define HFP_MCU_UART_BAUDRATE	115200

static unsigned long eic770x_sysclk_rate(void)
{
	/* syscfg clock is a mux of 24Mhz xtal clock and spll0_fout3/divisor */
	uint32_t syscfg_clk = readl_relaxed((void*)EIC770X_SYSCRG_SYSCLK);

	if (EIC770X_SYSCLK_SEL(syscfg_clk))
		return EIC770X_XTAL_CLK_RATE;

	return EIC770X_SPLL0_OUT3_RATE / EIC770X_SYSCLK_DIV(syscfg_clk);
}

static void eic770x_enable_uart_clk(unsigned port)
{
	uint32_t lsp_clk_en = readl_relaxed((void*)EIC770X_SYSCRG_LSPCLK0);

	lsp_clk_en |= EIC770X_UART_CLK_BIT(port);
	writel(lsp_clk_en, (void*)EIC770X_SYSCRG_LSPCLK0);
}

static void hfp_send_bmc_msg(uint8_t type, uint8_t cmd,
			     const uint8_t *data, uint8_t len)
{
	unsigned long sysclk_rate;
	struct uart8250_device uart_dev;
	union {
		struct hfp_bmc_message msg;
		char as_char[sizeof(struct hfp_bmc_message)];
	} xmit = {{
		.header_magic = MAGIC_HEADER,
		.type = type,
		.cmd = cmd,
		.data_len = len,
		.tail_magic = MAGIC_TAIL,
	}};

	/**
	 * Re-initialize UART.
	 * S-mode OS may have changed the clock frequency of syscfg clock
	 * which is the clock of all low speed peripherals, including UARTs.
	 * S-mode OS may also have disabled the UART2 clock via clock gate.
	 * (lsp_clk_en0 bit 17-21 controls UART0-4). Thus, we re-calculate
	 * the clock rate, enable UART clock, and re-initialize UART.
	 */

	sysclk_rate = eic770x_sysclk_rate();
	eic770x_enable_uart_clk(HFP_MCU_UART_PORT);

	uart8250_device_init(&uart_dev,
			EIC770X_UART(HFP_MCU_UART_PORT),
			sysclk_rate,
			HFP_MCU_UART_BAUDRATE,
			EIC770X_UART_REG_SHIFT,
			EIC770X_UART_REG_WIDTH,
			0, 0);

	sbi_memcpy(&xmit.msg.data, data, len);
	hfp_bmc_checksum_msg(&xmit.msg);

	for (unsigned int i = 0; i < sizeof(xmit.as_char); i++)
		uart8250_device_putc(&uart_dev, xmit.as_char[i]);
}

static int hfp_system_reset_check(u32 type, u32 reason)
{
	switch (type) {
	case SBI_SRST_RESET_TYPE_COLD_REBOOT:
	case SBI_SRST_RESET_TYPE_SHUTDOWN:
		return 255;
	default:
		return 0;
	}
}

static void hfp_system_reset(u32 type, u32 reason)
{
	switch (type) {
	case SBI_SRST_RESET_TYPE_SHUTDOWN:
		hfp_send_bmc_msg(HFP_MSG_NOTIFY, HFP_CMD_POWER_OFF,
				NULL, 0);
		break;
	case SBI_SRST_RESET_TYPE_COLD_REBOOT:
		hfp_send_bmc_msg(HFP_MSG_NOTIFY, HFP_CMD_RESTART,
				NULL, 0);
		break;
	}
	sbi_hart_hang();
}

static struct sbi_system_reset_device hfp_reset = {
	.name = "hfp_reset",
	.system_reset_check = hfp_system_reset_check,
	.system_reset = hfp_system_reset,
};

const struct eic770x_board_override hfp_override = {
	.reset_dev = &hfp_reset,
};
