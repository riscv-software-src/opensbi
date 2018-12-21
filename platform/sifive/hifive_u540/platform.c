/*
 * Copyright (c) 2018 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sbi/riscv_encoding.h>
#include <sbi/sbi_const.h>
#include <sbi/sbi_platform.h>
#include <sbi/riscv_io.h>
#include <plat/irqchip/plic.h>
#include <plat/serial/sifive-uart.h>
#include <plat/sys/clint.h>

#define SIFIVE_U_SYS_CLK			1000000000

#define SIFIVE_U_CLINT_ADDR			0x2000000

#define SIFIVE_U_PLIC_ADDR			0xc000000
#define SIFIVE_U_PLIC_NUM_SOURCES		0x35
#define SIFIVE_U_PLIC_NUM_PRIORITIES		7

#define SIFIVE_U_UART0_ADDR			0x10010000
#define SIFIVE_U_UART1_ADDR			0x10011000
#define SIFIVE_UART_BAUDRATE			115200

/* PRCI clock related macros */
//TODO: Do we need a separate driver for this ?
#define SIFIVE_PRCI_BASE_ADDR			0x10000000
#define SIFIVE_PRCI_CLKMUXSTATUSREG    		0x002C
#define SIFIVE_PRCI_CLKMUX_STATUS_TLCLKSEL      (0x1 << 1)

static int sifive_u_cold_final_init(void)
{
	u32 i;
	void *fdt = sbi_scratch_thishart_arg1_ptr();

	plic_fdt_fixup(fdt, "riscv,plic0", 0);
	for (i = 1; i < PLAT_HART_COUNT; i++)
		plic_fdt_fixup(fdt, "riscv,plic0", 2 * i - 1);

	return 0;
}

static u32 sifive_u_pmp_region_count(u32 target_hart)
{
	return 1;
}

static int sifive_u_pmp_region_info(u32 target_hart, u32 index,
				    ulong *prot, ulong *addr, ulong *log2size)
{
	int ret = 0;

	switch (index) {
	case 0:
		*prot = PMP_R | PMP_W | PMP_X;
		*addr = 0;
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
	unsigned long peri_in_freq;

	if (readl((volatile void *)SIFIVE_PRCI_BASE_ADDR +
		  SIFIVE_PRCI_CLKMUXSTATUSREG) &
		  SIFIVE_PRCI_CLKMUX_STATUS_TLCLKSEL){
		peri_in_freq = SIFIVE_U_SYS_CLK;
	} else {
		peri_in_freq = SIFIVE_U_SYS_CLK / 2;
	}

	return sifive_uart_init(SIFIVE_U_UART0_ADDR,
				peri_in_freq, SIFIVE_UART_BAUDRATE);
}

static int sifive_u_cold_irqchip_init(void)
{
	return plic_cold_irqchip_init(SIFIVE_U_PLIC_ADDR,
				      SIFIVE_U_PLIC_NUM_SOURCES,
				      PLAT_HART_COUNT);
}

static int sifive_u_warm_irqchip_init(u32 target_hart)
{
	return plic_warm_irqchip_init(target_hart,
			(target_hart) ? (2 * target_hart - 1) : 0,
			(target_hart) ? (2 * target_hart) : -1);
}

static int sifive_u_cold_ipi_init(void)
{
	return clint_cold_ipi_init(SIFIVE_U_CLINT_ADDR,
				   PLAT_HART_COUNT);
}

static int sifive_u_cold_timer_init(void)
{
	return clint_cold_timer_init(SIFIVE_U_CLINT_ADDR,
				     PLAT_HART_COUNT);
}

static int sifive_u_system_down(u32 type)
{
	/* For now nothing to do. */
	return 0;
}

struct sbi_platform platform = {
	.name = STRINGIFY(PLAT_NAME),
	.features = SBI_PLATFORM_DEFAULT_FEATURES,
	.hart_count = PLAT_HART_COUNT,
	.hart_stack_size = PLAT_HART_STACK_SIZE,
	.pmp_region_count = sifive_u_pmp_region_count,
	.pmp_region_info = sifive_u_pmp_region_info,
	.cold_final_init = sifive_u_cold_final_init,
	.console_putc = sifive_uart_putc,
	.console_getc = sifive_uart_getc,
	.console_init = sifive_u_console_init,
	.cold_irqchip_init = sifive_u_cold_irqchip_init,
	.warm_irqchip_init = sifive_u_warm_irqchip_init,
	.ipi_inject = clint_ipi_inject,
	.ipi_sync = clint_ipi_sync,
	.ipi_clear = clint_ipi_clear,
	.warm_ipi_init = clint_warm_ipi_init,
	.cold_ipi_init = sifive_u_cold_ipi_init,
	.timer_value = clint_timer_value,
	.timer_event_stop = clint_timer_event_stop,
	.timer_event_start = clint_timer_event_start,
	.warm_timer_init = clint_warm_timer_init,
	.cold_timer_init = sifive_u_cold_timer_init,
	.system_reboot = sifive_u_system_down,
	.system_shutdown = sifive_u_system_down
};
