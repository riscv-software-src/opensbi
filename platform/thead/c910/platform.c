/*
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sbi/riscv_encoding.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_const.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_platform.h>
#include <sbi_utils/irqchip/plic.h>
#include <sbi_utils/serial/uart8250.h>
#include <sbi_utils/sys/clint.h>
#include "platform.h"

static struct c910_regs_struct c910_regs;

static int c910_early_init(bool cold_boot)
{
	if (cold_boot) {
		/* Load from boot core */
		c910_regs.pmpaddr0 = csr_read(CSR_PMPADDR0);
		c910_regs.pmpaddr1 = csr_read(CSR_PMPADDR1);
		c910_regs.pmpaddr2 = csr_read(CSR_PMPADDR2);
		c910_regs.pmpaddr3 = csr_read(CSR_PMPADDR3);
		c910_regs.pmpaddr4 = csr_read(CSR_PMPADDR4);
		c910_regs.pmpaddr5 = csr_read(CSR_PMPADDR5);
		c910_regs.pmpaddr6 = csr_read(CSR_PMPADDR6);
		c910_regs.pmpaddr7 = csr_read(CSR_PMPADDR7);
		c910_regs.pmpcfg0  = csr_read(CSR_PMPCFG0);

		c910_regs.mcor     = csr_read(CSR_MCOR);
		c910_regs.mhcr     = csr_read(CSR_MHCR);
		c910_regs.mccr2    = csr_read(CSR_MCCR2);
		c910_regs.mhint    = csr_read(CSR_MHINT);
		c910_regs.mxstatus = csr_read(CSR_MXSTATUS);

		c910_regs.plic_base_addr = csr_read(CSR_PLIC_BASE);
		c910_regs.clint_base_addr =
			c910_regs.plic_base_addr + C910_PLIC_CLINT_OFFSET;
	} else {
		/* Store to other core */
		csr_write(CSR_PMPADDR0, c910_regs.pmpaddr0);
		csr_write(CSR_PMPADDR1, c910_regs.pmpaddr1);
		csr_write(CSR_PMPADDR2, c910_regs.pmpaddr2);
		csr_write(CSR_PMPADDR3, c910_regs.pmpaddr3);
		csr_write(CSR_PMPADDR4, c910_regs.pmpaddr4);
		csr_write(CSR_PMPADDR5, c910_regs.pmpaddr5);
		csr_write(CSR_PMPADDR6, c910_regs.pmpaddr6);
		csr_write(CSR_PMPADDR7, c910_regs.pmpaddr7);
		csr_write(CSR_PMPCFG0, c910_regs.pmpcfg0);

		csr_write(CSR_MCOR, c910_regs.mcor);
		csr_write(CSR_MHCR, c910_regs.mhcr);
		csr_write(CSR_MHINT, c910_regs.mhint);
		csr_write(CSR_MXSTATUS, c910_regs.mxstatus);
	}

	return 0;
}

static int c910_final_init(bool cold_boot)
{
	return 0;
}

static int c910_irqchip_init(bool cold_boot)
{
	/* Delegate plic enable into S-mode */
	writel(C910_PLIC_DELEG_ENABLE,
		(void *)c910_regs.plic_base_addr + C910_PLIC_DELEG_OFFSET);

	return 0;
}

static int c910_ipi_init(bool cold_boot)
{
	int rc;

	if (cold_boot) {
		rc = clint_cold_ipi_init(c910_regs.clint_base_addr, C910_HART_COUNT);
		if (rc)
			return rc;
	}

	return clint_warm_ipi_init();
}

static int c910_timer_init(bool cold_boot)
{
	int ret;

	if (cold_boot) {
		ret = clint_cold_timer_init(c910_regs.clint_base_addr,
					C910_HART_COUNT, FALSE);
		if (ret)
			return ret;
	}

	return clint_warm_timer_init();
}

static int c910_system_shutdown(u32 type)
{
	asm volatile ("ebreak");
	return 0;
}

int c910_hart_start(u32 hartid, ulong saddr)
{
	csr_write(CSR_MRVBR, saddr);
	csr_write(CSR_MRMR, csr_read(CSR_MRMR) | (1 << hartid));

	return 0;
}

const struct sbi_platform_operations platform_ops = {
	.early_init          = c910_early_init,
	.final_init          = c910_final_init,

	.irqchip_init        = c910_irqchip_init,

	.ipi_init            = c910_ipi_init,
	.ipi_send            = clint_ipi_send,
	.ipi_clear           = clint_ipi_clear,

	.timer_init          = c910_timer_init,
	.timer_event_start   = clint_timer_event_start,

	.system_shutdown     = c910_system_shutdown,

	.hart_start          = c910_hart_start,
};

const struct sbi_platform platform = {
	.opensbi_version     = OPENSBI_VERSION,
	.platform_version    = SBI_PLATFORM_VERSION(0x0, 0x01),
	.name                = "T-HEAD Xuantie c910",
	.features            = SBI_THEAD_FEATURES,
	.hart_count          = C910_HART_COUNT,
	.hart_stack_size     = SBI_PLATFORM_DEFAULT_HART_STACK_SIZE,
	.platform_ops_addr   = (unsigned long)&platform_ops
};
