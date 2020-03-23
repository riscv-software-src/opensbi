/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Atish Patra <atish.patra@wdc.com>
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

#define FU540_HART_COUNT			5

#define FU540_SYS_CLK				1000000000

#define FU540_CLINT_ADDR			0x2000000

#define FU540_PLIC_ADDR				0xc000000
#define FU540_PLIC_NUM_SOURCES			0x35
#define FU540_PLIC_NUM_PRIORITIES		7

#define FU540_UART0_ADDR			0x10010000
#define FU540_UART1_ADDR			0x10011000
#define FU540_UART_BAUDRATE			115200

/* PRCI clock related macros */
//TODO: Do we need a separate driver for this ?
#define FU540_PRCI_BASE_ADDR			0x10000000
#define FU540_PRCI_CLKMUXSTATUSREG		0x002C
#define FU540_PRCI_CLKMUX_STATUS_TLCLKSEL	(0x1 << 1)

/* Full tlb flush always */
#define FU540_TLB_RANGE_FLUSH_LIMIT		0

/* clang-format on */

static void fu540_modify_dt(void *fdt)
{
	fdt_cpu_fixup(fdt);

	fdt_fixups(fdt);
}

static int fu540_final_init(bool cold_boot)
{
	void *fdt;

	if (!cold_boot)
		return 0;

	fdt = sbi_scratch_thishart_arg1_ptr();
	fu540_modify_dt(fdt);

	return 0;
}

static u32 fu540_pmp_region_count(u32 hartid)
{
	return 1;
}

static int fu540_pmp_region_info(u32 hartid, u32 index, ulong *prot,
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

static int fu540_console_init(void)
{
	unsigned long peri_in_freq;

	if (readl((volatile void *)FU540_PRCI_BASE_ADDR +
		  FU540_PRCI_CLKMUXSTATUSREG) &
	    FU540_PRCI_CLKMUX_STATUS_TLCLKSEL) {
		peri_in_freq = FU540_SYS_CLK;
	} else {
		peri_in_freq = FU540_SYS_CLK / 2;
	}

	return sifive_uart_init(FU540_UART0_ADDR, peri_in_freq,
				FU540_UART_BAUDRATE);
}

static int fu540_irqchip_init(bool cold_boot)
{
	int rc;
	u32 hartid = current_hartid();

	if (cold_boot) {
		rc = plic_cold_irqchip_init(FU540_PLIC_ADDR,
					    FU540_PLIC_NUM_SOURCES,
					    FU540_HART_COUNT);
		if (rc)
			return rc;
	}

	return plic_warm_irqchip_init(hartid, (hartid) ? (2 * hartid - 1) : 0,
				      (hartid) ? (2 * hartid) : -1);
}

static int fu540_ipi_init(bool cold_boot)
{
	int rc;

	if (cold_boot) {
		rc = clint_cold_ipi_init(FU540_CLINT_ADDR, FU540_HART_COUNT);
		if (rc)
			return rc;
	}

	return clint_warm_ipi_init();
}

static u64 fu540_get_tlbr_flush_limit(void)
{
	return FU540_TLB_RANGE_FLUSH_LIMIT;
}

static int fu540_timer_init(bool cold_boot)
{
	int rc;

	if (cold_boot) {
		rc = clint_cold_timer_init(FU540_CLINT_ADDR,
					   FU540_HART_COUNT, TRUE);
		if (rc)
			return rc;
	}

	return clint_warm_timer_init();
}

static u32 fu540_hart_index2id[FU540_HART_COUNT - 1] = {
	[0] = 1,
	[1] = 2,
	[2] = 3,
	[3] = 4,
};

static int fu540_system_down(u32 type)
{
	/* For now nothing to do. */
	return 0;
}

const struct sbi_platform_operations platform_ops = {
	.pmp_region_count	= fu540_pmp_region_count,
	.pmp_region_info	= fu540_pmp_region_info,
	.final_init		= fu540_final_init,
	.console_putc		= sifive_uart_putc,
	.console_getc		= sifive_uart_getc,
	.console_init		= fu540_console_init,
	.irqchip_init		= fu540_irqchip_init,
	.ipi_send		= clint_ipi_send,
	.ipi_clear		= clint_ipi_clear,
	.ipi_init		= fu540_ipi_init,
	.get_tlbr_flush_limit	= fu540_get_tlbr_flush_limit,
	.timer_value		= clint_timer_value,
	.timer_event_stop	= clint_timer_event_stop,
	.timer_event_start	= clint_timer_event_start,
	.timer_init		= fu540_timer_init,
	.system_reboot		= fu540_system_down,
	.system_shutdown	= fu540_system_down
};

const struct sbi_platform platform = {
	.opensbi_version	= OPENSBI_VERSION,
	.platform_version	= SBI_PLATFORM_VERSION(0x0, 0x01),
	.name			= "SiFive Freedom U540",
	.features		= SBI_PLATFORM_DEFAULT_FEATURES,
	.hart_count		= (FU540_HART_COUNT - 1),
	.hart_index2id		= fu540_hart_index2id,
	.hart_stack_size	= SBI_PLATFORM_DEFAULT_HART_STACK_SIZE,
	.platform_ops_addr	= (unsigned long)&platform_ops
};
