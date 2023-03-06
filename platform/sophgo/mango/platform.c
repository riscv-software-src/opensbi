/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_encoding.h>
#include <sbi/sbi_const.h>
#include <sbi/sbi_platform.h>

#include <sbi/riscv_io.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_hsm.h>
#include <sbi/sbi_trap.h>
#include <sbi/sbi_bitops.h>
#include <sbi/riscv_locks.h>

/*
 * Include these files as needed.
 * See objects.mk PLATFORM_xxx configuration parameters.
 */
#include <sbi_utils/ipi/aclint_mswi.h>
#include <sbi_utils/irqchip/plic.h>
#include <sbi_utils/serial/uart8250.h>
#include <sbi_utils/timer/aclint_mtimer.h>

#include "platform.h"

#define PLATFORM_ACLINT_MTIMER_FREQ	50000000

static struct aclint_mswi_data mswi = {
	.addr = SG2040_MSIP_BASE,
	.size = ACLINT_MSWI_SIZE,
	.first_hartid = 0,
	.hart_count = SG2040_HART_COUNT,
};

static struct aclint_mtimer_data mtimer = {
	.mtime_freq = PLATFORM_ACLINT_MTIMER_FREQ,
	.mtime_addr = SG2040_MTIMECMP_BASE +
		      ACLINT_DEFAULT_MTIME_OFFSET,
	.mtime_size = ACLINT_DEFAULT_MTIME_SIZE,
	.mtimecmp_addr = SG2040_MTIMECMP_BASE +
			 ACLINT_DEFAULT_MTIMECMP_OFFSET,
	.mtimecmp_size = ACLINT_DEFAULT_MTIMECMP_SIZE,
	.first_hartid = 0,
	.hart_count = SG2040_HART_COUNT,
	.has_64bit_mmio = false,
};

static struct sg2040_regs_struct sg2040_regs;

static spinlock_t init_lock = SPIN_LOCK_INITIALIZER;
int need_set_cpu = 1;
unsigned long mcpuid, sub_revision;
struct pmp pmp_addr[32] = {{0, 0}};
unsigned long pmp_attr[9] = {0};
unsigned long pmp_base, pmp_entry, pmp_cfg, mrvbr, mrmr;

#define PMP_CFG_R	BIT(0)
#define PMP_CFG_W	BIT(1)
#define PMP_CFG_X	BIT(2)

#define PMP_CFG_A_SHIFT	3
#define PMP_CFG_A_OFF	(0UL << PMP_CFG_A_SHIFT)
#define PMP_CFG_A_TOR	(1UL << PMP_CFG_A_SHIFT)
#define PMP_CFG_A_NA4	(2UL <<	PMP_CFG_A_SHIFT)/* not supported by c910 */
#define PMP_CFG_A_NAPOT	(3UL << PMP_CFG_A_SHIFT)

#define MANGO_PA_MAX	((1UL << 40) - 1UL)

void setup_pmp(void)
{
       csr_write(pmpaddr0, MANGO_PA_MAX >> 2);
       csr_write(pmpcfg0, PMP_CFG_R | PMP_CFG_W | PMP_CFG_X | PMP_CFG_A_TOR);
}

void setup_cpu(void)
{
	sg2040_regs.mcor     = 0x70013;
	sg2040_regs.mhcr     = 0x11ff;
	sg2040_regs.mccr2    = 0xe0410009;
	sg2040_regs.mhint    = 0x6e30c;
#ifndef MANGO_DVM
	/* disable sfence.vma broadcast */
	sg2040_regs.mhint    |= 1 << 21;
	/* disable fence rw broadcast */
	sg2040_regs.mhint    |= 1 << 22;
	/* disable fence.i broadcast */
	sg2040_regs.mhint    |= 1 << 23;
#endif
	/* workaround lr/sc livelock */	
	sg2040_regs.mhint2    = csr_read(CSR_MHINT2);
	sg2040_regs.mhint2   |= 3 << 7;
	/* enable MAEE */
	sg2040_regs.mxstatus = 0x638000;

	csr_write(CSR_MCOR, sg2040_regs.mcor);
	csr_write(CSR_MHCR, sg2040_regs.mhcr);
	csr_write(CSR_MHINT, sg2040_regs.mhint);
	csr_write(CSR_MHINT2, sg2040_regs.mhint2);
	csr_write(CSR_MXSTATUS, sg2040_regs.mxstatus);
	csr_write(CSR_MCCR2, sg2040_regs.mccr2);

}

/*
 * Platform early initialization.
 */
static int sg2040_early_init(bool cold_boot)
{
	if (cold_boot) {
		/* Read TWICE!!! */
		mcpuid = csr_read(CSR_MCPUID);
		mcpuid = csr_read(CSR_MCPUID);

		/* Get bit [23...18] */
		sub_revision = (mcpuid & 0xfc0000) >> 18;
        // to avoid changing sbi_init
		//sbi_printf("sub_revision: %ld\n", sub_revision);

		// parse_dts();
		sg2040_regs.plic_base_addr = SG2040_PLIC_BASE;
		sg2040_regs.msip_base_addr = SG2040_MSIP_BASE;
		sg2040_regs.mtimecmp_base_addr = SG2040_MTIMECMP_BASE;

		setup_pmp();
		setup_cpu();

	}

	if (!need_set_cpu)
		return 0;

	spin_lock(&init_lock);

	if (cold_boot) {
		/* Load from boot core */
		sg2040_regs.pmpaddr0 = csr_read(CSR_PMPADDR0);
		sg2040_regs.pmpaddr1 = csr_read(CSR_PMPADDR1);
		//sg2040_regs.pmpaddr2 = csr_read(CSR_PMPADDR2);
		//sg2040_regs.pmpaddr3 = csr_read(CSR_PMPADDR3);
		//sg2040_regs.pmpaddr4 = csr_read(CSR_PMPADDR4);
		//sg2040_regs.pmpaddr5 = csr_read(CSR_PMPADDR5);
		//sg2040_regs.pmpaddr6 = csr_read(CSR_PMPADDR6);
		//sg2040_regs.pmpaddr7 = csr_read(CSR_PMPADDR7);
		sg2040_regs.pmpcfg0  = csr_read(CSR_PMPCFG0);

		sg2040_regs.mcor     = csr_read(CSR_MCOR);
		sg2040_regs.mhcr     = csr_read(CSR_MHCR);
		sg2040_regs.mccr2    = csr_read(CSR_MCCR2);
		sg2040_regs.mhint    = csr_read(CSR_MHINT);
		sg2040_regs.mhint2   = csr_read(CSR_MHINT2);
		sg2040_regs.mxstatus = csr_read(CSR_MXSTATUS);
	} else {
		/* Store to other core */
		csr_write(CSR_PMPADDR0, sg2040_regs.pmpaddr0);
		csr_write(CSR_PMPADDR1, sg2040_regs.pmpaddr1);
		//csr_write(CSR_PMPADDR2, sg2040_regs.pmpaddr2);
		//csr_write(CSR_PMPADDR3, sg2040_regs.pmpaddr3);
		//csr_write(CSR_PMPADDR4, sg2040_regs.pmpaddr4);
		//csr_write(CSR_PMPADDR5, sg2040_regs.pmpaddr5);
		//csr_write(CSR_PMPADDR6, sg2040_regs.pmpaddr6);
		//csr_write(CSR_PMPADDR7, sg2040_regs.pmpaddr7);
		csr_write(CSR_PMPCFG0, sg2040_regs.pmpcfg0);

		csr_write(CSR_MCOR, sg2040_regs.mcor);
		csr_write(CSR_MHCR, sg2040_regs.mhcr);
		csr_write(CSR_MHINT, sg2040_regs.mhint);
		csr_write(CSR_MHINT2, sg2040_regs.mhint2);
		csr_write(CSR_MXSTATUS, sg2040_regs.mxstatus);
	}

	spin_unlock(&init_lock);

	return 0;
}

static void sg2040_delegate_more_traps()
{
	unsigned long exceptions = csr_read(CSR_MEDELEG);

	/* Delegate 0 ~ 7 exceptions to S-mode */
	exceptions |= ((1U << CAUSE_MISALIGNED_FETCH) | (1U << CAUSE_FETCH_ACCESS) |
		(1U << CAUSE_BREAKPOINT) |
		(1U << CAUSE_MISALIGNED_LOAD) | (1U << CAUSE_LOAD_ACCESS) |
		(1U << CAUSE_MISALIGNED_STORE) | (1U << CAUSE_STORE_ACCESS));

	csr_write(CSR_MEDELEG, exceptions);
}

static int sg2040_final_init(bool cold_boot)
{
	sg2040_delegate_more_traps();

	return 0;
}

/*
 * Initialize the platform console.
 */
static int sg2040_console_init(void)
{
	return uart8250_init(
			SG2040_UART0_ADDRBASE,
			SG2040_UART0_FREQ,
			SG2040_CONSOLE_BDRATE,
			2, 0, 0);
}

/*
 * Initialize the platform interrupt controller for current HART.
 */
static int sg2040_irqchip_init(bool cold_boot)
{
	/* Delegate plic enable into S-mode */
	#ifndef __SG_QEMU__
	writel(SG2040_PLIC_DELEG_ENABLE,
		(void *)sg2040_regs.plic_base_addr + SG2040_PLIC_DELEG_OFFSET);
	#endif
	return 0;
}

/*
 * Initialize IPI for current HART.
 */
static int sg2040_ipi_init(bool cold_boot)
{
	int ret;

	/* Example if the generic ACLINT driver is used */
	if (cold_boot) {
		ret = aclint_mswi_cold_init(&mswi);
		if (ret)
			return ret;
	}

	return aclint_mswi_warm_init();
}

/*
 * Initialize platform timer for current HART.
 */
static int sg2040_timer_init(bool cold_boot)
{
	int ret;

	/* Example if the generic ACLINT driver is used */
	if (cold_boot) {
		ret = aclint_mtimer_cold_init(&mtimer, NULL);
		if (ret)
			return ret;
	}

	return aclint_mtimer_warm_init();
}

void sg2040_pmu_init(void)
{
	unsigned long interrupts;

	interrupts = csr_read(CSR_MIDELEG) | (1 << 17);
	csr_write(CSR_MIDELEG, interrupts);

	/* CSR_MCOUNTEREN has already been set in mstatus_init() */
	csr_write(CSR_MCOUNTERWEN, 0xffffffff);
	csr_write(CSR_MHPMEVENT3, 1);
	csr_write(CSR_MHPMEVENT4, 2);
	csr_write(CSR_MHPMEVENT5, 3);
	csr_write(CSR_MHPMEVENT6, 4);
	csr_write(CSR_MHPMEVENT7, 5);
	csr_write(CSR_MHPMEVENT8, 6);
	csr_write(CSR_MHPMEVENT9, 7);
	csr_write(CSR_MHPMEVENT10, 8);
	csr_write(CSR_MHPMEVENT11, 9);
	csr_write(CSR_MHPMEVENT12, 10);
	csr_write(CSR_MHPMEVENT13, 11);
	csr_write(CSR_MHPMEVENT14, 12);
	csr_write(CSR_MHPMEVENT15, 13);
	csr_write(CSR_MHPMEVENT16, 14);
	csr_write(CSR_MHPMEVENT17, 15);
	csr_write(CSR_MHPMEVENT18, 16);
	csr_write(CSR_MHPMEVENT19, 17);
	csr_write(CSR_MHPMEVENT20, 18);
	csr_write(CSR_MHPMEVENT21, 19);
	csr_write(CSR_MHPMEVENT22, 20);
	csr_write(CSR_MHPMEVENT23, 21);
	csr_write(CSR_MHPMEVENT24, 22);
	csr_write(CSR_MHPMEVENT25, 23);
	csr_write(CSR_MHPMEVENT26, 24);
	csr_write(CSR_MHPMEVENT27, 25);
	csr_write(CSR_MHPMEVENT28, 26);
}

void sg2040_pmu_map(unsigned long idx, unsigned long event_id)
{
	switch (idx) {
	case 3:
		csr_write(CSR_MHPMEVENT3, event_id);
		break;
	case 4:
		csr_write(CSR_MHPMEVENT4, event_id);
		break;
	case 5:
		csr_write(CSR_MHPMEVENT5, event_id);
		break;
	case 6:
		csr_write(CSR_MHPMEVENT6, event_id);
		break;
	case 7:
		csr_write(CSR_MHPMEVENT7, event_id);
		break;
	case 8:
		csr_write(CSR_MHPMEVENT8, event_id);
		break;
	case 9:
		csr_write(CSR_MHPMEVENT9, event_id);
		break;
	case 10:
		csr_write(CSR_MHPMEVENT10, event_id);
		break;
	case 11:
		csr_write(CSR_MHPMEVENT11, event_id);
		break;
	case 12:
		csr_write(CSR_MHPMEVENT12, event_id);
		break;
	case 13:
		csr_write(CSR_MHPMEVENT13, event_id);
		break;
	case 14:
		csr_write(CSR_MHPMEVENT14, event_id);
		break;
	case 15:
		csr_write(CSR_MHPMEVENT15, event_id);
		break;
	case 16:
		csr_write(CSR_MHPMEVENT16, event_id);
		break;
	case 17:
		csr_write(CSR_MHPMEVENT17, event_id);
		break;
	case 18:
		csr_write(CSR_MHPMEVENT18, event_id);
		break;
	case 19:
		csr_write(CSR_MHPMEVENT19, event_id);
		break;
	case 20:
		csr_write(CSR_MHPMEVENT20, event_id);
		break;
	case 21:
		csr_write(CSR_MHPMEVENT21, event_id);
		break;
	case 22:
		csr_write(CSR_MHPMEVENT22, event_id);
		break;
	case 23:
		csr_write(CSR_MHPMEVENT23, event_id);
		break;
	case 24:
		csr_write(CSR_MHPMEVENT24, event_id);
		break;
	case 25:
		csr_write(CSR_MHPMEVENT25, event_id);
		break;
	case 26:
		csr_write(CSR_MHPMEVENT26, event_id);
		break;
	case 27:
		csr_write(CSR_MHPMEVENT27, event_id);
		break;
	case 28:
		csr_write(CSR_MHPMEVENT28, event_id);
		break;
	case 29:
		csr_write(CSR_MHPMEVENT29, event_id);
		break;
	case 30:
		csr_write(CSR_MHPMEVENT30, event_id);
		break;
	case 31:
		csr_write(CSR_MHPMEVENT31, event_id);
		break;
	}
}

void sg2040_set_pmu(unsigned long type, unsigned long idx, unsigned long event_id)
{
	switch (type) {
	case 2:
		sg2040_pmu_map(idx, event_id);
		break;
	default:
		sg2040_pmu_init();
		break;
	}
}

static int sg2040_vendor_ext_provider(long extid, long funcid,
				const struct sbi_trap_regs *regs,
				unsigned long *out_value,
				struct sbi_trap_info *out_trap)
{
	switch (extid) {
	case SBI_EXT_VENDOR_SG2040_SET_PMU:
		sg2040_set_pmu(regs->a0, regs->a1, regs->a2);
		break;
	default:
		sbi_printf("Unsupported private sbi call: %ld\n", extid);
		asm volatile ("ebreak");
	}
	return 0;
}

/*
 * Platform descriptor.
 */
const struct sbi_platform_operations platform_ops = {
	.early_init		= sg2040_early_init,
	.final_init		= sg2040_final_init,
	.console_init		= sg2040_console_init,
	.irqchip_init		= sg2040_irqchip_init,
	.ipi_init		= sg2040_ipi_init,
	.timer_init		= sg2040_timer_init,
	.vendor_ext_provider = sg2040_vendor_ext_provider,
};
const struct sbi_platform platform = {
	.opensbi_version	= OPENSBI_VERSION,
	.platform_version	= SBI_PLATFORM_VERSION(0x0, 0x01),
	.name		= "Sophgo manGo sg2040",
	.features		= SBI_SOPHGO_FEATURES,
	.hart_count		= SG2040_HART_COUNT,
	.hart_stack_size	= SBI_PLATFORM_DEFAULT_HART_STACK_SIZE,
	.platform_ops_addr	= (unsigned long)&platform_ops
};
