/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_encoding.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_const.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_platform.h>

/*
 * Include these files as needed.
 * See objects.mk PLATFORM_xxx configuration parameters.
 */
#include <sbi_utils/irqchip/plic.h>
#include <sbi_utils/timer/aclint_mtimer.h>

#define PLATFORM_PLIC_ADDR		0x00200100
#define PLATFORM_PLIC_SIZE		(0x200000 + \
					 (PLATFORM_HART_COUNT * 0x1000))
#define PLATFORM_PLIC_NUM_SOURCES	128
#define PLATFORM_HART_COUNT		1
// #define PLATFORM_CLINT_ADDR		0x2000000
#define PLATFORM_ACLINT_MTIMER_FREQ	10000000
#define PLATFORM_ACLINT_MSWI_ADDR 0x00000096
#define PLATFORM_ACLINT_MTIMER_ADDR	0x00000080
#define PLATFORM_UART_ADDR		0x1
// #define PLATFORM_UART_INPUT_FREQ	10000000
// #define PLATFORM_UART_BAUDRATE		115200
#define PLATFORM_UART_DATA_OFFSET	0
#define PLATFORM_UART_RXSTS_OFFSET	1
#define PLATFORM_UART_TXSTS_OFFSET	2
#define PLATFORM_UART_RX_AVAIL		0x1

static void ikr_uart_putc(char ch)
{
	while (readb((volatile void *)(PLATFORM_UART_ADDR +
				      PLATFORM_UART_TXSTS_OFFSET)) != 0)
		;

	writeb((u8)ch, (volatile void *)(PLATFORM_UART_ADDR +
					 PLATFORM_UART_DATA_OFFSET));
}

static void ikr_uart_puts(const char *s)
{
	while (*s) {
		if (*s == '\n')
			ikr_uart_putc('\r');
		ikr_uart_putc(*s++);
	}
}

static int ikr_uart_getc(void)
{
	if (readb((volatile void *)(PLATFORM_UART_ADDR +
				      PLATFORM_UART_RXSTS_OFFSET)) &
	    PLATFORM_UART_RX_AVAIL) {
		u8 ch = readb((volatile void *)(PLATFORM_UART_ADDR +
					      PLATFORM_UART_DATA_OFFSET));
		writeb(0, (volatile void *)(PLATFORM_UART_ADDR +
				    PLATFORM_UART_RXSTS_OFFSET));
		return ch;
	}

	return -1;
}

static struct sbi_console_device ikr_uart_console = {
	.name = "ikr-uart",
	.console_putc = ikr_uart_putc,
	.console_getc = ikr_uart_getc,
};

static struct plic_data plic = {
	.unique_id = 0,
	.addr = PLATFORM_PLIC_ADDR,
	.size = PLATFORM_PLIC_SIZE,
	.num_src = PLATFORM_PLIC_NUM_SOURCES,
	.context_map = {
		[0] = { 0, 1 },
	},
};

// With 1 hart this is usually fine. With multi-hart kernels you must add MSWI/IPI support.
// static struct aclint_mswi_data mswi = {
// 	.addr = PLATFORM_ACLINT_MSWI_ADDR,
// 	.size = ACLINT_MSWI_SIZE,
// 	.first_hartid = 0,
// 	.hart_count = PLATFORM_HART_COUNT,
// };

static struct aclint_mtimer_data mtimer = {
	.mtime_freq = PLATFORM_ACLINT_MTIMER_FREQ,
	.mtime_addr = PLATFORM_ACLINT_MTIMER_ADDR,
	.mtime_size = 0x8,
	.mtimecmp_addr = PLATFORM_ACLINT_MTIMER_ADDR + 0x8,
	.mtimecmp_size = 0x8,
	.first_hartid = 0,
	.hart_count = PLATFORM_HART_COUNT,
	.has_64bit_mmio = true,
};

/*
 * Platform early initialization.
 */
static int platform_early_init(bool cold_boot)
{

	if (!cold_boot)
		return 0;

	/* Raw UART banner before console registration. */
	ikr_uart_puts("Loading OpenSBI now...\n");

	sbi_console_set_device(&ikr_uart_console);

	/* UART and other low MMIO lives in the first page in rvemu. */
	sbi_domain_root_add_memrange(0x0, PAGE_SIZE, PAGE_SIZE,
				     (SBI_DOMAIN_MEMREGION_MMIO |
				      SBI_DOMAIN_MEMREGION_SHARED_SURW_MRW));

	/* rvemu does not provide a page-aligned MSWI window; with one hart we can
	 * skip MSWI initialization and still boot OpenSBI.
	 */
	return 0;
}

/*
 * Platform final initialization.
 */
static int platform_final_init(bool cold_boot)
{
	return 0;
}

/*
 * Initialize the platform interrupt controller during cold boot.
 */
static int platform_irqchip_init(void)
{
	/* Example if the generic PLIC driver is used */
	return plic_cold_irqchip_init(&plic);
}

/*
 * Initialize platform timer during cold boot.
 */
static int platform_timer_init(void)
{
	/* Example if the generic ACLINT driver is used */
	return aclint_mtimer_cold_init(&mtimer, NULL);
}

/*
 * Platform descriptor.
 */
const struct sbi_platform_operations platform_ops = {
	.early_init		= platform_early_init,
	.final_init		= platform_final_init,
	.irqchip_init		= platform_irqchip_init,
	.timer_init		= platform_timer_init
};
const struct sbi_platform platform = {
	.opensbi_version	= OPENSBI_VERSION,
	.platform_version	= SBI_PLATFORM_VERSION(0x0, 0x00),
	.name			= "ikr-linux",
	.features		= SBI_PLATFORM_DEFAULT_FEATURES,
	.hart_count		= PLATFORM_HART_COUNT,
	.hart_stack_size	= SBI_PLATFORM_DEFAULT_HART_STACK_SIZE,
	.heap_size		= SBI_PLATFORM_DEFAULT_HEAP_SIZE(1),
	.platform_ops_addr	= (unsigned long)&platform_ops
};
