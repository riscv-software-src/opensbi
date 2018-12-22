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
#include <plat/irqchip/plic.h>
#include <plat/serial/uart8250.h>
#include <plat/sys/clint.h>

#define VIRT_TEST_ADDR			0x100000

#define VIRT_CLINT_ADDR			0x2000000

#define VIRT_PLIC_ADDR			0xc000000
#define VIRT_PLIC_NUM_SOURCES		127
#define VIRT_PLIC_NUM_PRIORITIES	7

#define VIRT_UART16550_ADDR		0x10000000
#define VIRT_UART_BAUDRATE		115200
#define VIRT_UART_SHIFTREG_ADDR		1843200

static int virt_cold_final_init(void)
{
	u32 i;
	void *fdt = sbi_scratch_thishart_arg1_ptr();

	for (i = 0; i < PLAT_HART_COUNT; i++)
		plic_fdt_fixup(fdt, "riscv,plic0", 2 * i);

	return 0;
}

static u32 virt_pmp_region_count(u32 target_hart)
{
	return 1;
}

static int virt_pmp_region_info(u32 target_hart, u32 index,
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

static int virt_console_init(void)
{
	return uart8250_init(VIRT_UART16550_ADDR,
			     VIRT_UART_SHIFTREG_ADDR,
			     VIRT_UART_BAUDRATE, 0, 1);
}

static int virt_cold_irqchip_init(void)
{
	return plic_cold_irqchip_init(VIRT_PLIC_ADDR,
				      VIRT_PLIC_NUM_SOURCES,
				      PLAT_HART_COUNT);
}

static int virt_warm_irqchip_init(u32 target_hart)
{
	return plic_warm_irqchip_init(target_hart,
				      (2 * target_hart),
				      (2 * target_hart + 1));
}

static int virt_cold_ipi_init(void)
{
	return clint_cold_ipi_init(VIRT_CLINT_ADDR,
				   PLAT_HART_COUNT);
}

static int virt_cold_timer_init(void)
{
	return clint_cold_timer_init(VIRT_CLINT_ADDR,
				     PLAT_HART_COUNT);
}

static int virt_system_down(u32 type)
{
	/* For now nothing to do. */
	return 0;
}

struct sbi_platform platform = {
	.name = "QEMU Virt Machine",
	.features = SBI_PLATFORM_DEFAULT_FEATURES,
	.hart_count = PLAT_HART_COUNT,
	.hart_stack_size = PLAT_HART_STACK_SIZE,
	.disabled_hart_mask = 0,
	.pmp_region_count = virt_pmp_region_count,
	.pmp_region_info = virt_pmp_region_info,
	.cold_final_init = virt_cold_final_init,
	.console_putc = uart8250_putc,
	.console_getc = uart8250_getc,
	.console_init = virt_console_init,
	.cold_irqchip_init = virt_cold_irqchip_init,
	.warm_irqchip_init = virt_warm_irqchip_init,
	.ipi_inject = clint_ipi_inject,
	.ipi_sync = clint_ipi_sync,
	.ipi_clear = clint_ipi_clear,
	.warm_ipi_init = clint_warm_ipi_init,
	.cold_ipi_init = virt_cold_ipi_init,
	.timer_value = clint_timer_value,
	.timer_event_stop = clint_timer_event_stop,
	.timer_event_start = clint_timer_event_start,
	.warm_timer_init = clint_warm_timer_init,
	.cold_timer_init = virt_cold_timer_init,
	.system_reboot = virt_system_down,
	.system_shutdown = virt_system_down
};
