/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <sbi/riscv_encoding.h>
#include <sbi/sbi_const.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_platform.h>
#include <sbi_utils/irqchip/plic.h>
#include <sbi_utils/serial/sifive-uart.h>
#include <sbi_utils/sys/clint.h>

/* clang-format off */

#define SIFIVE_U_HART_COUNT			4
#define SIFIVE_U_HART_STACK_SIZE		8192

#define SIFIVE_U_SYS_CLK			1000000000
#define SIFIVE_U_PERIPH_CLK			(SIFIVE_U_SYS_CLK / 2)

#define SIFIVE_U_CLINT_ADDR			0x2000000

#define SIFIVE_U_PLIC_ADDR			0xc000000
#define SIFIVE_U_PLIC_NUM_SOURCES		0x35
#define SIFIVE_U_PLIC_NUM_PRIORITIES		7

#define SIFIVE_U_UART0_ADDR			0x10013000
#define SIFIVE_U_UART1_ADDR			0x10023000

/* clang-format on */

static int sifive_u_final_init(bool cold_boot)
{
	void *fdt;

	if (!cold_boot)
		return 0;

	fdt = sbi_scratch_thishart_arg1_ptr();
	plic_fdt_fixup(fdt, "riscv,plic0");

	return 0;
}

static u32 sifive_u_pmp_region_count(u32 hartid)
{
	return 1;
}

static int sifive_u_pmp_region_info(u32 hartid, u32 index, ulong *prot,
				    ulong *addr, ulong *log2size)
{
	int ret = 0;

	switch (index) {
	case 0:
		*prot	  = PMP_R | PMP_W | PMP_X;
		*addr	  = 0;
		*log2size = __riscv_xlen;
		break;
	default:
		ret = -1;
		break;
	};

	return ret;
}

static int sifive_u_console_init(void)
{
	return sifive_uart_init(SIFIVE_U_UART0_ADDR, SIFIVE_U_PERIPH_CLK,
				115200);
}

static int sifive_u_irqchip_init(bool cold_boot)
{
	int rc;
	u32 hartid = sbi_current_hartid();

	if (cold_boot) {
		rc = plic_cold_irqchip_init(SIFIVE_U_PLIC_ADDR,
					    SIFIVE_U_PLIC_NUM_SOURCES,
					    SIFIVE_U_HART_COUNT);
		if (rc)
			return rc;
	}

	return plic_warm_irqchip_init(hartid, (2 * hartid), (2 * hartid + 1));
}

static int sifive_u_ipi_init(bool cold_boot)
{
	int rc;

	if (cold_boot) {
		rc = clint_cold_ipi_init(SIFIVE_U_CLINT_ADDR,
					 SIFIVE_U_HART_COUNT);
		if (rc)
			return rc;
	}

	return clint_warm_ipi_init();
}

static int sifive_u_timer_init(bool cold_boot)
{
	int rc;

	if (cold_boot) {
		rc = clint_cold_timer_init(SIFIVE_U_CLINT_ADDR,
					   SIFIVE_U_HART_COUNT);
		if (rc)
			return rc;
	}

	return clint_warm_timer_init();
}

static int sifive_u_system_down(u32 type)
{
	/* For now nothing to do. */
	return 0;
}

const struct sbi_platform_operations platform_ops = {
	.pmp_region_count	= sifive_u_pmp_region_count,
	.pmp_region_info	= sifive_u_pmp_region_info,
	.final_init		= sifive_u_final_init,
	.console_putc		= sifive_uart_putc,
	.console_getc		= sifive_uart_getc,
	.console_init		= sifive_u_console_init,
	.irqchip_init		= sifive_u_irqchip_init,
	.ipi_send		= clint_ipi_send,
	.ipi_clear		= clint_ipi_clear,
	.ipi_init		= sifive_u_ipi_init,
	.timer_value		= clint_timer_value,
	.timer_event_stop	= clint_timer_event_stop,
	.timer_event_start	= clint_timer_event_start,
	.timer_init		= sifive_u_timer_init,
	.system_reboot		= sifive_u_system_down,
	.system_shutdown	= sifive_u_system_down
};

const struct sbi_platform platform = {
	.opensbi_version	= OPENSBI_VERSION,
	.platform_version	= SBI_PLATFORM_VERSION(0x0, 0x01),
	.name			= "QEMU SiFive Unleashed",
	.features		= SBI_PLATFORM_DEFAULT_FEATURES,
	.hart_count		= SIFIVE_U_HART_COUNT,
	.hart_stack_size	= SIFIVE_U_HART_STACK_SIZE,
	.disabled_hart_mask	= 0,
	.platform_ops_addr	= (unsigned long)&platform_ops
};
