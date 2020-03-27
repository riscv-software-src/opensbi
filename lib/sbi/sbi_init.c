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
#include <sbi/riscv_locks.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_ecall.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_hartmask.h>
#include <sbi/sbi_hsm.h>
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

static spinlock_t coldboot_lock = SPIN_LOCK_INITIALIZER;
static unsigned long coldboot_done = 0;
static struct sbi_hartmask coldboot_wait_hmask = { 0 };

static void wait_for_coldboot(struct sbi_scratch *scratch, u32 hartid)
{
	unsigned long saved_mie, cmip;
	const struct sbi_platform *plat = sbi_platform_ptr(scratch);

	/* Save MIE CSR */
	saved_mie = csr_read(CSR_MIE);

	/* Set MSIE bit to receive IPI */
	csr_set(CSR_MIE, MIP_MSIP);

	/* Acquire coldboot lock */
	spin_lock(&coldboot_lock);

	/* Mark current HART as waiting */
	sbi_hartmask_set_hart(hartid, &coldboot_wait_hmask);

	/* Wait for coldboot to finish using WFI */
	while (!coldboot_done) {
		spin_unlock(&coldboot_lock);
		do {
			wfi();
			cmip = csr_read(CSR_MIP);
		 } while (!(cmip & MIP_MSIP));
		spin_lock(&coldboot_lock);
	};

	/* Unmark current HART as waiting */
	sbi_hartmask_clear_hart(hartid, &coldboot_wait_hmask);

	/* Release coldboot lock */
	spin_unlock(&coldboot_lock);

	/* Restore MIE CSR */
	csr_write(CSR_MIE, saved_mie);

	/* Clear current HART IPI */
	sbi_platform_ipi_clear(plat, hartid);
}

static void wake_coldboot_harts(struct sbi_scratch *scratch, u32 hartid)
{
	const struct sbi_platform *plat = sbi_platform_ptr(scratch);

	/* Acquire coldboot lock */
	spin_lock(&coldboot_lock);

	/* Mark coldboot done */
	coldboot_done = 1;

	/* Send an IPI to all HARTs waiting for coldboot */
	for (int i = 0; i <= sbi_scratch_last_hartid(); i++) {
		if ((i != hartid) &&
		    sbi_hartmask_test_hart(i, &coldboot_wait_hmask))
			sbi_platform_ipi_send(plat, i);
	}

	/* Release coldboot lock */
	spin_unlock(&coldboot_lock);
}

static unsigned long init_count_offset;

static void __noreturn init_coldboot(struct sbi_scratch *scratch, u32 hartid)
{
	int rc;
	unsigned long *init_count;
	const struct sbi_platform *plat = sbi_platform_ptr(scratch);

	/* Note: This has to be first thing in coldboot init sequence */
	rc = sbi_scratch_init(scratch);
	if (rc)
		sbi_hart_hang();

	init_count_offset = sbi_scratch_alloc_offset(__SIZEOF_POINTER__,
						     "INIT_COUNT");
	if (!init_count_offset)
		sbi_hart_hang();

	rc = sbi_hsm_init(scratch, hartid, TRUE);
	if (rc)
		sbi_hart_hang();

	rc = sbi_platform_early_init(plat, TRUE);
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

	rc = sbi_platform_final_init(plat, TRUE);
	if (rc)
		sbi_hart_hang();

	if (!(scratch->options & SBI_SCRATCH_NO_BOOT_PRINTS))
		sbi_boot_prints(scratch, hartid);

	wake_coldboot_harts(scratch, hartid);

	init_count = sbi_scratch_offset_ptr(scratch, init_count_offset);
	(*init_count)++;

	sbi_hsm_prepare_next_jump(scratch, hartid);
	sbi_hart_switch_mode(hartid, scratch->next_arg1, scratch->next_addr,
			     scratch->next_mode, FALSE);
}

static void __noreturn init_warmboot(struct sbi_scratch *scratch, u32 hartid)
{
	int rc;
	unsigned long *init_count;
	const struct sbi_platform *plat = sbi_platform_ptr(scratch);

	wait_for_coldboot(scratch, hartid);

	if (!init_count_offset)
		sbi_hart_hang();

	rc = sbi_hsm_init(scratch, hartid, FALSE);
	if (rc)
		sbi_hart_hang();

	rc = sbi_platform_early_init(plat, FALSE);
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

	rc = sbi_platform_final_init(plat, FALSE);
	if (rc)
		sbi_hart_hang();

	init_count = sbi_scratch_offset_ptr(scratch, init_count_offset);
	(*init_count)++;

	sbi_hsm_prepare_next_jump(scratch, hartid);
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
	u32 hartid			= current_hartid();
	const struct sbi_platform *plat = sbi_platform_ptr(scratch);

	if ((SBI_HARTMASK_MAX_BITS <= hartid) ||
	    sbi_platform_hart_invalid(plat, hartid))
		sbi_hart_hang();

	if (atomic_xchg(&coldboot_lottery, 1) == 0)
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

	if (!init_count_offset)
		return 0;

	scratch = sbi_hartid_to_scratch(hartid);
	if (!scratch)
		return 0;

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
	u32 hartid			= current_hartid();
	const struct sbi_platform *plat = sbi_platform_ptr(scratch);

	if (sbi_platform_hart_invalid(plat, hartid))
		sbi_hart_hang();

	sbi_platform_early_exit(plat);

	sbi_timer_exit(scratch);

	sbi_ipi_exit(scratch);

	sbi_platform_irqchip_exit(plat);

	sbi_platform_final_exit(plat);

	sbi_hsm_exit(scratch);
}
