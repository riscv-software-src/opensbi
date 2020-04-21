/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) Nuclei Corporation or its affiliates.
 *
 * Authors:
 *   lujun <lujun@nucleisys.com>
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
#include <sbi_utils/sys/nuclei_timer.h>

/* clang-format off */

#define UX600_HART_COUNT			1

#define UX600_SYS_CLK				1000000000

#define UX600_RV_TIMER_ADDR			0x2000000

#define UX600_PLIC_ADDR				0x8000000
#define UX600_PLIC_NUM_SOURCES		0x35
#define UX600_PLIC_NUM_PRIORITIES	7

#define UX600_UART0_ADDR			0x10013000
#define UX600_UART1_ADDR			0x10023000
#define UX600_UART_BAUDRATE			115200

/* Full tlb flush always */
#define UX600_TLB_RANGE_FLUSH_LIMIT	0

/* clang-format on */

static void ux600_modify_dt(void *fdt)
{
	fdt_cpu_fixup(fdt);

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

static u32 ux600_pmp_region_count(u32 hartid)
{
	return 1;
}

static int ux600_pmp_region_info(u32 hartid, u32 index, ulong *prot,
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

static int ux600_console_init(void)
{
	return sifive_uart_init(UX600_UART0_ADDR, UX600_SYS_CLK,
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
		rc = nuclei_timer_cold_ipi_init(UX600_RV_TIMER_ADDR + 0xffc, UX600_HART_COUNT);
		if (rc)
			return rc;
	}

	return nuclei_timer_warm_ipi_init();
}

static u64 ux600_get_tlbr_flush_limit(void)
{
	return UX600_TLB_RANGE_FLUSH_LIMIT;
}

static int ux600_timer_init(bool cold_boot)
{
	int rc;

	if (cold_boot) {
		rc = nuclei_timer_cold_timer_init(UX600_RV_TIMER_ADDR,
					   UX600_HART_COUNT, TRUE);
		if (rc)
			return rc;
	}

	return nuclei_timer_warm_timer_init();
}

static int ux600_system_down(u32 type)
{
	/* For now nothing to do. */
	return 0;
}

const struct sbi_platform_operations platform_ops = {
	.pmp_region_count	= ux600_pmp_region_count,
	.pmp_region_info	= ux600_pmp_region_info,
	.final_init		= ux600_final_init,
	.console_putc		= sifive_uart_putc,
	.console_getc		= sifive_uart_getc,
	.console_init		= ux600_console_init,
	.irqchip_init		= ux600_irqchip_init,
	.ipi_send		= nuclei_timer_ipi_send,
	.ipi_clear		= nuclei_timer_ipi_clear,
	.ipi_init		= ux600_ipi_init,
	.get_tlbr_flush_limit	= ux600_get_tlbr_flush_limit,
	.timer_value		= nuclei_timer_timer_value,
	.timer_event_stop	= nuclei_timer_timer_event_stop,
	.timer_event_start	= nuclei_timer_timer_event_start,
	.timer_init		= ux600_timer_init,
	.system_reboot		= ux600_system_down,
	.system_shutdown	= ux600_system_down
};

const struct sbi_platform platform = {
	.opensbi_version	= OPENSBI_VERSION,
	.platform_version	= SBI_PLATFORM_VERSION(0x0U, 0x01U),
	.name			= "Nuclei UX600",
	.features		= SBI_PLATFORM_DEFAULT_FEATURES,
	.hart_count		= (UX600_HART_COUNT),
	.hart_stack_size	= SBI_PLATFORM_DEFAULT_HART_STACK_SIZE,
	.platform_ops_addr	= (unsigned long)&platform_ops
};
