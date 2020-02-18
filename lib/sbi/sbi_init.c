/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
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
#include <sbi/sbi_tlb.h>
#include <sbi/sbi_version.h>

#define BANNER                                              \
	"   ____                    _____ ____ _____\n"     \
	"  / __ \\                  / ____|  _ \\_   _|\n"  \
	" | |  | |_ __   ___ _ __ | (___ | |_) || |\n"      \
	" | |  | | '_ \\ / _ \\ '_ \\ \\___ \\|  _ < | |\n" \
	" | |__| | |_) |  __/ | | |____) | |_) || |_\n"     \
	"  \\____/| .__/ \\___|_| |_|_____/|____/_____|\n"  \
	"        | |\n"                                     \
	"        |_|\n\n"

static void sbi_boot_prints(struct sbi_scratch *scratch, u32 hartid)
{
	int xlen;
	char str[64];
	const struct sbi_platform *plat = sbi_platform_ptr(scratch);

#ifdef OPENSBI_VERSION_GIT
	sbi_printf("\nOpenSBI %s\n", OPENSBI_VERSION_GIT);
#else
	sbi_printf("\nOpenSBI v%d.%d\n", OPENSBI_VERSION_MAJOR,
		   OPENSBI_VERSION_MINOR);
#endif

	sbi_printf(BANNER);

	/* Determine MISA XLEN and MISA string */
	xlen = misa_xlen();
	if (xlen < 1) {
		sbi_printf("Error %d getting MISA XLEN\n", xlen);
		sbi_hart_hang();
	}
	xlen = 16 * (1 << xlen);
	misa_string(str, sizeof(str));

	/* Platform details */
	sbi_printf("Platform Name          : %s\n", sbi_platform_name(plat));
	sbi_printf("Platform HART Features : RV%d%s\n", xlen, str);
	sbi_printf("Platform Max HARTs     : %d\n",
		   sbi_platform_hart_count(plat));
	sbi_printf("Current Hart           : %u\n", hartid);
	/* Firmware details */
	sbi_printf("Firmware Base          : 0x%lx\n", scratch->fw_start);
	sbi_printf("Firmware Size          : %d KB\n",
		   (u32)(scratch->fw_size / 1024));
	/* Generic details */
	sbi_printf("Runtime SBI Version    : %d.%d\n",
		   sbi_ecall_version_major(), sbi_ecall_version_minor());
	sbi_printf("\n");

	sbi_hart_delegation_dump(scratch);
	sbi_hart_pmp_dump(scratch);
}

static unsigned long init_count_offset;

static void __noreturn init_coldboot(struct sbi_scratch *scratch, u32 hartid)
{
	int rc;
	unsigned long *init_count;
	const struct sbi_platform *plat = sbi_platform_ptr(scratch);

	init_count_offset = sbi_scratch_alloc_offset(__SIZEOF_POINTER__,
						     "INIT_COUNT");
	if (!init_count_offset)
		sbi_hart_hang();

	rc = sbi_system_early_init(scratch, TRUE);
	if (rc)
		sbi_hart_hang();

	rc = sbi_hart_init(scratch, hartid, TRUE);
	if (rc)
		sbi_hart_hang();

	rc = sbi_console_init(scratch);
	if (rc)
		sbi_hart_hang();

	rc = sbi_platform_irqchip_init(plat, TRUE);
	if (rc)
		sbi_hart_hang();

	rc = sbi_ipi_init(scratch, TRUE);
	if (rc)
		sbi_hart_hang();

	rc = sbi_tlb_init(scratch, TRUE);
	if (rc)
		sbi_hart_hang();

	rc = sbi_timer_init(scratch, TRUE);
	if (rc)
		sbi_hart_hang();

	rc = sbi_ecall_init();
	if (rc)
		sbi_hart_hang();

	rc = sbi_system_final_init(scratch, TRUE);
	if (rc)
		sbi_hart_hang();

	if (!(scratch->options & SBI_SCRATCH_NO_BOOT_PRINTS))
		sbi_boot_prints(scratch, hartid);

	sbi_hart_wake_coldboot_harts(scratch, hartid);

	sbi_hart_mark_available(hartid);

	init_count = sbi_scratch_offset_ptr(scratch, init_count_offset);
	(*init_count)++;

	sbi_hart_switch_mode(hartid, scratch->next_arg1, scratch->next_addr,
			     scratch->next_mode, FALSE);
}

static void __noreturn init_warmboot(struct sbi_scratch *scratch, u32 hartid)
{
	int rc;
	unsigned long *init_count;
	const struct sbi_platform *plat = sbi_platform_ptr(scratch);

	sbi_hart_wait_for_coldboot(scratch, hartid);

	if (!init_count_offset)
		sbi_hart_hang();

	rc = sbi_system_early_init(scratch, FALSE);
	if (rc)
		sbi_hart_hang();

	rc = sbi_hart_init(scratch, hartid, FALSE);
	if (rc)
		sbi_hart_hang();

	rc = sbi_platform_irqchip_init(plat, FALSE);
	if (rc)
		sbi_hart_hang();

	rc = sbi_ipi_init(scratch, FALSE);
	if (rc)
		sbi_hart_hang();

	rc = sbi_tlb_init(scratch, FALSE);
	if (rc)
		sbi_hart_hang();

	rc = sbi_timer_init(scratch, FALSE);
	if (rc)
		sbi_hart_hang();

	rc = sbi_system_final_init(scratch, FALSE);
	if (rc)
		sbi_hart_hang();

	sbi_hart_mark_available(hartid);

	init_count = sbi_scratch_offset_ptr(scratch, init_count_offset);
	(*init_count)++;

	sbi_hart_switch_mode(hartid, scratch->next_arg1,
			     scratch->next_addr,
			     scratch->next_mode, FALSE);
}

static atomic_t coldboot_lottery = ATOMIC_INITIALIZER(0);

/**
 * Initialize OpenSBI library for current HART and jump to next
 * booting stage.
 *
 * The function expects following:
 * 1. The 'mscratch' CSR is pointing to sbi_scratch of current HART
 * 2. Stack pointer (SP) is setup for current HART
 * 3. Interrupts are disabled in MSTATUS CSR
 * 4. All interrupts are disabled in MIE CSR
 *
 * @param scratch pointer to sbi_scratch of current HART
 */
void __noreturn sbi_init(struct sbi_scratch *scratch)
{
	bool coldboot			= FALSE;
	u32 hartid			= sbi_current_hartid();
	const struct sbi_platform *plat = sbi_platform_ptr(scratch);

	if (sbi_platform_hart_disabled(plat, hartid))
		sbi_hart_hang();

	if (atomic_add_return(&coldboot_lottery, 1) == 1)
		coldboot = TRUE;

	if (coldboot)
		init_coldboot(scratch, hartid);
	else
		init_warmboot(scratch, hartid);
}

unsigned long sbi_init_count(u32 hartid)
{
	struct sbi_scratch *scratch;
	unsigned long *init_count;

	if (sbi_platform_hart_count(sbi_platform_thishart_ptr()) <= hartid ||
	    !init_count_offset)
		return 0;

	scratch = sbi_hart_id_to_scratch(sbi_scratch_thishart_ptr(), hartid);
	init_count = sbi_scratch_offset_ptr(scratch, init_count_offset);

	return *init_count;
}

/**
 * Exit OpenSBI library for current HART and stop HART
 *
 * The function expects following:
 * 1. The 'mscratch' CSR is pointing to sbi_scratch of current HART
 * 2. Stack pointer (SP) is setup for current HART
 *
 * @param scratch pointer to sbi_scratch of current HART
 */
void __noreturn sbi_exit(struct sbi_scratch *scratch)
{
	u32 hartid			= sbi_current_hartid();
	const struct sbi_platform *plat = sbi_platform_ptr(scratch);

	if (sbi_platform_hart_disabled(plat, hartid))
		sbi_hart_hang();

	sbi_hart_unmark_available(hartid);

	sbi_platform_early_exit(plat);

	sbi_timer_exit(scratch);

	sbi_ipi_exit(scratch);

	sbi_platform_irqchip_exit(plat);

	sbi_platform_final_exit(plat);

	sbi_hart_hang();
}
