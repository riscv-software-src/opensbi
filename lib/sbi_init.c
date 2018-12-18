/*
 * Copyright (c) 2018 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_atomic.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_ecall.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_ipi.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_system.h>
#include <sbi/sbi_timer.h>

static void __attribute__((noreturn)) init_coldboot(struct sbi_scratch *scratch,
						    u32 hartid)
{
	int rc;
	char str[64];
	struct sbi_platform *plat = sbi_platform_ptr(scratch);

	rc = sbi_system_cold_early_init(scratch);
	if (rc)
		sbi_hart_hang();

	rc = sbi_system_warm_early_init(scratch, hartid);
	if (rc)
		sbi_hart_hang();

	rc = sbi_hart_init(scratch, hartid);
	if (rc)
		sbi_hart_hang();

	rc = sbi_console_init(scratch);
	if (rc)
		sbi_hart_hang();

	rc = sbi_platform_cold_irqchip_init(plat);
	if (rc)
		sbi_hart_hang();

	rc = sbi_platform_warm_irqchip_init(plat, hartid);
	if (rc)
		sbi_hart_hang();

	rc = sbi_ipi_cold_init(scratch);
	if (rc)
		sbi_hart_hang();

	rc = sbi_ipi_warm_init(scratch, hartid);
	if (rc)
		sbi_hart_hang();

	rc = sbi_timer_cold_init(scratch);
	if (rc)
		sbi_hart_hang();

	rc = sbi_timer_warm_init(scratch, hartid);
	if (rc)
		sbi_hart_hang();

	rc = sbi_system_cold_final_init(scratch);
	if (rc)
		sbi_hart_hang();

	rc = sbi_system_warm_final_init(scratch, hartid);
	if (rc)
		sbi_hart_hang();

	misa_string(str, sizeof(str));
	sbi_printf("OpenSBI v%d.%d (%s %s)\n",
		   OPENSBI_MAJOR, OPENSBI_MINOR,
		   __DATE__, __TIME__);
	sbi_printf("\n");
	/* Platform details */
	sbi_printf("Platform Name          : %s\n", sbi_platform_name(plat));
	sbi_printf("Platform HART Features : RV%d%s\n", misa_xlen(), str);
	sbi_printf("Platform Max HARTs     : %d\n",
		   sbi_platform_hart_count(plat));
	/* Firmware details */
	sbi_printf("Firmware Base          : 0x%lx\n", scratch->fw_start);
	sbi_printf("Firmware Size          : %d KB\n",
		   (u32)(scratch->fw_size / 1024));
	/* Generic details */
	sbi_printf("Runtime SBI Version    : %d.%d\n",
		   sbi_ecall_version_major(), sbi_ecall_version_minor());
	sbi_printf("\n");

	sbi_hart_pmp_dump();

	sbi_hart_mark_available(hartid);

	if (!sbi_platform_has_hart_hotplug(plat))
		sbi_hart_wake_coldboot_harts(scratch, hartid);

	sbi_hart_boot_next(hartid, scratch->next_arg1,
			   scratch->next_addr, scratch->next_mode);
}

static void __attribute__((noreturn)) init_warmboot(struct sbi_scratch *scratch,
						    u32 hartid)
{
	int rc;
	struct sbi_platform *plat = sbi_platform_ptr(scratch);

	if (!sbi_platform_has_hart_hotplug(plat))
		sbi_hart_wait_for_coldboot(scratch, hartid);

	rc = sbi_system_warm_early_init(scratch, hartid);
	if (rc)
		sbi_hart_hang();

	rc = sbi_hart_init(scratch, hartid);
	if (rc)
		sbi_hart_hang();

	rc = sbi_platform_warm_irqchip_init(plat, hartid);
	if (rc)
		sbi_hart_hang();

	rc = sbi_ipi_warm_init(scratch, hartid);
	if (rc)
		sbi_hart_hang();

	rc = sbi_timer_warm_init(scratch, hartid);
	if (rc)
		sbi_hart_hang();

	rc = sbi_system_warm_final_init(scratch, hartid);
	if (rc)
		sbi_hart_hang();

	sbi_hart_mark_available(hartid);

	if (sbi_platform_has_hart_hotplug(plat))
		/* TODO: To be implemented in-future. */
		sbi_hart_hang();
	else
		sbi_hart_boot_next(hartid, scratch->next_arg1,
				   scratch->next_addr, scratch->next_mode);
}

static atomic_t coldboot_lottery = ATOMIC_INITIALIZER(0);

void __attribute__((noreturn)) sbi_init(struct sbi_scratch *scratch)
{
	bool coldboot = FALSE;
	u32 hartid = csr_read(mhartid);

	if (atomic_add_return(&coldboot_lottery, 1) == 1)
		coldboot = TRUE;

	if (coldboot)
		init_coldboot(scratch, hartid);
	else
		init_warmboot(scratch, hartid);
}
