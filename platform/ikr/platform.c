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
#define PLATFORM_PLIC_SIZE		(0x200000 + (2 * PLATFORM_HART_COUNT * 0x1000))
#define PLATFORM_PLIC_NUM_SOURCES	128
#define PLATFORM_HART_COUNT		1

#define PLATFORM_ACLINT_MTIMER_FREQ	10000000
#define PLATFORM_ACLINT_MSWI_ADDR 0x00000096
#define PLATFORM_ACLINT_MTIMER_ADDR	0x00000080

#define PLATFORM_UART_ADDR		0x1
#define PLATFORM_UART_INPUT_FREQ	10000000
#define PLATFORM_UART_RXSTS_OFFSET	1
#define PLATFORM_UART_TXSTS_OFFSET	2	
#define PLATFORM_UART_DATA_OFFSET   0x00
#define PLATFORM_UART_IER_OFFSET    0x01
#define PLATFORM_UART_FCR_OFFSET    0x02
#define PLATFORM_UART_LCR_OFFSET    0x03
#define PLATFORM_UART_MCR_OFFSET    0x04
#define PLATFORM_UART_LSR_OFFSET    0x05
#define PLATFORM_UART_RX_AVAIL      0x01 // DR bit in LSR
/* Bit masks for the LSR (Offset 5) */
#define PLATFORM_UART_LSR_RX_READY  0x01 // Bit 0: Data Ready
#define PLATFORM_UART_LSR_TX_EMPTY  0x20 // Bit 5: Transmit Holding Register Empty

//For Qemu
// static void ikr_uart_putc(char ch)
// {
//     // Wait until THR is empty (Bit 5 of LSR becomes 1)
//     while ((readb((volatile void *)(PLATFORM_UART_ADDR + PLATFORM_UART_LSR_OFFSET)) & 
//             PLATFORM_UART_LSR_TX_EMPTY) == 0)
//         ;

//     writeb((u8)ch, (volatile void *)(PLATFORM_UART_ADDR + PLATFORM_UART_DATA_OFFSET));
// }

// //For Qemu
// static int ikr_uart_getc(void)
// {
//     // Check LSR Bit 0 (Data Ready)
//     u8 status = readb((volatile void *)(PLATFORM_UART_ADDR + PLATFORM_UART_LSR_OFFSET));
// //
//     if (status & PLATFORM_UART_LSR_RX_READY) {
//         // Read the data; this hardware-clears the Ready bit
//         return (int)readb((volatile void *)(PLATFORM_UART_ADDR + PLATFORM_UART_DATA_OFFSET));
//     }
//     return -1;
// }


static void ikr_uart_putc(char ch)
{
	while (readb((volatile void *)(PLATFORM_UART_ADDR +
				      PLATFORM_UART_TXSTS_OFFSET)) != 0);

	writeb((u8)ch, (volatile void *)(PLATFORM_UART_ADDR +
					 PLATFORM_UART_DATA_OFFSET));
}

static int ikr_uart_getc(void)
{
	/*
	 * Check RX_STATE bit 0 (data available). Reading UART_DATA
	 * automatically clears the RX_AVAIL bit in hardware, so no
	 * explicit status-register write is needed here. Writing 0
	 * to RXSTS after the DATA read would race with rvemu's input
	 * thread (which sets RX_AVAIL for the next queued character
	 * between the DATA read and such a write), silently dropping
	 * characters when multiple bytes arrive before a poll cycle.
	 */
	if (readb((volatile void *)(PLATFORM_UART_ADDR +
				      PLATFORM_UART_RXSTS_OFFSET)) &
	    PLATFORM_UART_RX_AVAIL) {
		return (int)(u8)readb((volatile void *)(PLATFORM_UART_ADDR +
					      PLATFORM_UART_DATA_OFFSET));
	}
	return -1;
}

static void ikr_uart_puts(const char *s)
{
	while (*s) {
		if (*s == '\n')
			ikr_uart_putc('\r');
		ikr_uart_putc(*s++);
	}
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
	/* Allow access to the new UART location */
	sbi_domain_root_add_memrange(PLATFORM_UART_ADDR, PAGE_SIZE, PAGE_SIZE,
					(SBI_DOMAIN_MEMREGION_MMIO |
					SBI_DOMAIN_MEMREGION_SHARED_SURW_MRW));

	/* Keep access to the PLIC/Timer area (assuming they are near 0x0c000000 or 0x02000000) */
	sbi_domain_root_add_memrange(PLATFORM_PLIC_ADDR, 0x400000, PAGE_SIZE,
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
