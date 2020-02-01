/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 */

#include <sbi/sbi_platform.h>
#include <sbi_utils/sys/htif.h>
#include <sbi_utils/sys/clint.h>

/* clang-format off */

#define SPIKE_HART_COUNT			8
#define SPIKE_HART_STACK_SIZE			8192

#define SPIKE_CLINT_ADDR			0x2000000

/* clang-format on */

static int spike_final_init(bool cold_boot)
{
	return 0;
}

static u32 spike_pmp_region_count(u32 hartid)
{
	return 1;
}

static int spike_pmp_region_info(u32 hartid, u32 index, ulong *prot, ulong *addr,
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

static int spike_console_init(void)
{
	return 0;
}

static int spike_irqchip_init(bool cold_boot)
{
	return 0;
}

static int spike_ipi_init(bool cold_boot)
{
	int ret;

	if (cold_boot) {
		ret = clint_cold_ipi_init(SPIKE_CLINT_ADDR,
					  SPIKE_HART_COUNT);
		if (ret)
			return ret;
	}

	return clint_warm_ipi_init();
}

static int spike_timer_init(bool cold_boot)
{
	int rc;

	if (cold_boot) {
		rc = clint_cold_timer_init(SPIKE_CLINT_ADDR,
					   SPIKE_HART_COUNT, TRUE);
		if (rc)
			return rc;
	}

	return clint_warm_timer_init();
}

const struct sbi_platform_operations platform_ops = {
	.pmp_region_count	= spike_pmp_region_count,
	.pmp_region_info	= spike_pmp_region_info,
	.final_init		= spike_final_init,
	.console_putc		= htif_putc,
	.console_getc		= htif_getc,
	.console_init		= spike_console_init,
	.irqchip_init		= spike_irqchip_init,
	.ipi_send		= clint_ipi_send,
	.ipi_clear		= clint_ipi_clear,
	.ipi_init		= spike_ipi_init,
	.timer_value		= clint_timer_value,
	.timer_event_stop	= clint_timer_event_stop,
	.timer_event_start	= clint_timer_event_start,
	.timer_init		= spike_timer_init,
	.system_reboot		= htif_system_down,
	.system_shutdown	= htif_system_down
};

const struct sbi_platform platform = {
	.opensbi_version	= OPENSBI_VERSION,
	.platform_version	= SBI_PLATFORM_VERSION(0x0, 0x01),
	.name			= "Spike",
	.features		= SBI_PLATFORM_DEFAULT_FEATURES,
	.hart_count		= SPIKE_HART_COUNT,
	.hart_stack_size	= SPIKE_HART_STACK_SIZE,
	.disabled_hart_mask	= 0,
	.platform_ops_addr	= (unsigned long)&platform_ops
};
