/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 *   Nick Kossifidis <mick@ics.forth.gr>
 */

#include <sbi/riscv_encoding.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_const.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_platform.h>
#include <sbi_utils/irqchip/plic.h>
#include <sbi_utils/serial/uart8250.h>
#include <sbi_utils/sys/clint.h>

/* clang-format off */

#define VIRT_HART_COUNT			8
#define VIRT_HART_STACK_SIZE		8192

#define VIRT_TEST_ADDR			0x100000
#define VIRT_TEST_FINISHER_FAIL		0x3333
#define VIRT_TEST_FINISHER_PASS		0x5555

#define VIRT_CLINT_ADDR			0x2000000

#define VIRT_PLIC_ADDR			0xc000000
#define VIRT_PLIC_NUM_SOURCES		127
#define VIRT_PLIC_NUM_PRIORITIES	7

#define VIRT_UART16550_ADDR		0x10000000
#define VIRT_UART_BAUDRATE		115200
#define VIRT_UART_SHIFTREG_ADDR		1843200

/* clang-format on */

static int virt_final_init(bool cold_boot)
{
	void *fdt;

	if (!cold_boot)
		return 0;

	fdt = sbi_scratch_thishart_arg1_ptr();
	plic_fdt_fixup(fdt, "riscv,plic0");

	return 0;
}

static u32 virt_pmp_region_count(u32 hartid)
{
	return 1;
}

static int virt_pmp_region_info(u32 hartid, u32 index, ulong *prot, ulong *addr,
				ulong *log2size)
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

static int virt_console_init(void)
{
	return uart8250_init(VIRT_UART16550_ADDR, VIRT_UART_SHIFTREG_ADDR,
			     VIRT_UART_BAUDRATE, 0, 1);
}

static int virt_irqchip_init(bool cold_boot)
{
	int rc;
	u32 hartid = sbi_current_hartid();

	if (cold_boot) {
		rc = plic_cold_irqchip_init(
			VIRT_PLIC_ADDR, VIRT_PLIC_NUM_SOURCES, VIRT_HART_COUNT);
		if (rc)
			return rc;
	}

	return plic_warm_irqchip_init(hartid, (2 * hartid), (2 * hartid + 1));
}

static int virt_ipi_init(bool cold_boot)
{
	int rc;

	if (cold_boot) {
		rc = clint_cold_ipi_init(VIRT_CLINT_ADDR, VIRT_HART_COUNT);
		if (rc)
			return rc;
	}

	return clint_warm_ipi_init();
}

static int virt_timer_init(bool cold_boot)
{
	int rc;

	if (cold_boot) {
		rc = clint_cold_timer_init(VIRT_CLINT_ADDR, VIRT_HART_COUNT);
		if (rc)
			return rc;
	}

	return clint_warm_timer_init();
}

static int virt_system_down(u32 type)
{
	/* Tell the "finisher" that the simulation
	 * was successful so that QEMU exits
	 */
	writew(VIRT_TEST_FINISHER_PASS, (void *)VIRT_TEST_ADDR);

	return 0;
}

const struct sbi_platform_operations platform_ops = {
	.pmp_region_count	= virt_pmp_region_count,
	.pmp_region_info	= virt_pmp_region_info,
	.final_init		= virt_final_init,
	.console_putc		= uart8250_putc,
	.console_getc		= uart8250_getc,
	.console_init		= virt_console_init,
	.irqchip_init		= virt_irqchip_init,
	.ipi_send		= clint_ipi_send,
	.ipi_clear		= clint_ipi_clear,
	.ipi_init		= virt_ipi_init,
	.timer_value		= clint_timer_value,
	.timer_event_stop	= clint_timer_event_stop,
	.timer_event_start	= clint_timer_event_start,
	.timer_init		= virt_timer_init,
	.system_reboot		= virt_system_down,
	.system_shutdown	= virt_system_down
};

const struct sbi_platform platform = {
	.opensbi_version	= OPENSBI_VERSION,
	.platform_version	= SBI_PLATFORM_VERSION(0x0, 0x01),
	.name			= "QEMU Virt Machine",
	.features		= SBI_PLATFORM_DEFAULT_FEATURES,
	.hart_count		= VIRT_HART_COUNT,
	.hart_stack_size	= VIRT_HART_STACK_SIZE,
	.disabled_hart_mask	= 0,
	.platform_ops_addr	= (unsigned long)&platform_ops
};
