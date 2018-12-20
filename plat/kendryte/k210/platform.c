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
#include <plat/sys/clint.h>
#include "platform.h"
#include "uarths.h"

#define K210_U_SYS_CLK			1000000000
#define K210_U_PERIPH_CLK		(K210_U_SYS_CLK / 2)

#define K210_U_PLIC_NUM_SOURCES		0x35
#define K210_U_PLIC_NUM_PRIORITIES	7

static int k210_console_init(void)
{
	uarths_init(115200, UARTHS_STOP_1);

	return 0;
}

static void k210_console_putc(char c)
{
	uarths_putc(c);
}

static char k210_console_getc(void)
{
	return uarths_getc();
}

static u32 k210_pmp_region_count(u32 target_hart)
{
	return 1;
}

static int k210_pmp_region_info(u32 target_hart, u32 index,
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

static int k210_cold_irqchip_init(void)
{
	return plic_cold_irqchip_init(PLIC_BASE_ADDR,
				      K210_U_PLIC_NUM_SOURCES,
				      PLAT_HART_COUNT);
}

static int k210_cold_ipi_init(void)
{
	return clint_cold_ipi_init(CLINT_BASE_ADDR,
				   PLAT_HART_COUNT);
}

static int k210_cold_timer_init(void)
{
	return clint_cold_timer_init(CLINT_BASE_ADDR,
				     PLAT_HART_COUNT);
}

static int k210_cold_final_init(void)
{
	return plic_fdt_fixup(sbi_scratch_thishart_arg1_ptr(), "riscv,plic0");
}

static int k210_system_down(u32 type)
{
	/* For now nothing to do. */
	return 0;
}

struct sbi_platform platform = {

	.name = STRINGIFY(PLAT_NAME),
	.features = SBI_PLATFORM_HAS_MMIO_TIMER_VALUE,

	.hart_count = PLAT_HART_COUNT,
	.hart_stack_size = PLAT_HART_STACK_SIZE,

	.pmp_region_count = k210_pmp_region_count,
	.pmp_region_info = k210_pmp_region_info,


	.console_init = k210_console_init,
	.console_putc = k210_console_putc,
	.console_getc = k210_console_getc,

	.cold_irqchip_init = k210_cold_irqchip_init,
	.warm_irqchip_init = plic_warm_irqchip_init,
	.ipi_inject = clint_ipi_inject,
	.ipi_sync = clint_ipi_sync,
	.ipi_clear = clint_ipi_clear,
	.warm_ipi_init = clint_warm_ipi_init,
	.cold_ipi_init = k210_cold_ipi_init,
	.cold_final_init = k210_cold_final_init,

	.timer_value = clint_timer_value,
	.timer_event_stop = clint_timer_event_stop,
	.timer_event_start = clint_timer_event_start,
	.warm_timer_init = clint_warm_timer_init,
	.cold_timer_init = k210_cold_timer_init,

	.system_reboot = k210_system_down,
	.system_shutdown = k210_system_down
};
