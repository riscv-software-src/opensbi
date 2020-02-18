/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_barrier.h>
#include <sbi/riscv_encoding.h>
#include <sbi/riscv_fp.h>
#include <sbi/riscv_locks.h>
#include <sbi/sbi_bits.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_platform.h>

/**
 * Return HART ID of the caller.
 */
unsigned int sbi_current_hartid()
{
	return (u32)csr_read(CSR_MHARTID);
}

static void mstatus_init(struct sbi_scratch *scratch, u32 hartid)
{
	const struct sbi_platform *plat = sbi_platform_ptr(scratch);

	/* Enable FPU */
	if (misa_extension('D') || misa_extension('F'))
		csr_write(CSR_MSTATUS, MSTATUS_FS);

	/* Enable user/supervisor use of perf counters */
	if (misa_extension('S') && sbi_platform_has_scounteren(plat))
		csr_write(CSR_SCOUNTEREN, -1);
	if (sbi_platform_has_mcounteren(plat))
		csr_write(CSR_MCOUNTEREN, -1);

	/* Disable all interrupts */
	csr_write(CSR_MIE, 0);

	/* Disable S-mode paging */
	if (misa_extension('S'))
		csr_write(CSR_SATP, 0);
}

static int fp_init(u32 hartid)
{
#ifdef __riscv_flen
	int i;
#endif

	if (!misa_extension('D') && !misa_extension('F'))
		return 0;

	if (!(csr_read(CSR_MSTATUS) & MSTATUS_FS))
		return SBI_EINVAL;

#ifdef __riscv_flen
	for (i = 0; i < 32; i++)
		init_fp_reg(i);
	csr_write(CSR_FCSR, 0);
#endif

	return 0;
}

static int delegate_traps(struct sbi_scratch *scratch, u32 hartid)
{
	const struct sbi_platform *plat = sbi_platform_ptr(scratch);
	unsigned long interrupts, exceptions;

	if (!misa_extension('S'))
		/* No delegation possible as mideleg does not exist*/
		return 0;

	/* Send M-mode interrupts and most exceptions to S-mode */
	interrupts = MIP_SSIP | MIP_STIP | MIP_SEIP;
	exceptions = (1U << CAUSE_MISALIGNED_FETCH) | (1U << CAUSE_BREAKPOINT) |
		     (1U << CAUSE_USER_ECALL);
	if (sbi_platform_has_mfaults_delegation(plat))
		exceptions |= (1U << CAUSE_FETCH_PAGE_FAULT) |
			      (1U << CAUSE_LOAD_PAGE_FAULT) |
			      (1U << CAUSE_STORE_PAGE_FAULT);

	/*
	 * If hypervisor extension available then we only handle hypervisor
	 * calls (i.e. ecalls from HS-mode) in M-mode.
	 *
	 * The HS-mode will additionally handle supervisor calls (i.e. ecalls
	 * from VS-mode), Guest page faults and Virtual interrupts.
	 */
	if (misa_extension('H')) {
		exceptions |= (1U << CAUSE_SUPERVISOR_ECALL);
		exceptions |= (1U << CAUSE_FETCH_GUEST_PAGE_FAULT);
		exceptions |= (1U << CAUSE_LOAD_GUEST_PAGE_FAULT);
		exceptions |= (1U << CAUSE_STORE_GUEST_PAGE_FAULT);
	}

	csr_write(CSR_MIDELEG, interrupts);
	csr_write(CSR_MEDELEG, exceptions);

	return 0;
}

void sbi_hart_delegation_dump(struct sbi_scratch *scratch)
{
#if __riscv_xlen == 32
	sbi_printf("MIDELEG : 0x%08lx\n", csr_read(CSR_MIDELEG));
	sbi_printf("MEDELEG : 0x%08lx\n", csr_read(CSR_MEDELEG));
#else
	sbi_printf("MIDELEG : 0x%016lx\n", csr_read(CSR_MIDELEG));
	sbi_printf("MEDELEG : 0x%016lx\n", csr_read(CSR_MEDELEG));
#endif
}

unsigned long log2roundup(unsigned long x)
{
	unsigned long ret = 0;

	while (ret < __riscv_xlen) {
		if (x <= (1UL << ret))
			break;
		ret++;
	}

	return ret;
}

void sbi_hart_pmp_dump(struct sbi_scratch *scratch)
{
	const struct sbi_platform *plat = sbi_platform_ptr(scratch);
	unsigned long prot, addr, size, l2l;
	unsigned int i;

	if (!sbi_platform_has_pmp(plat))
		return;

	for (i = 0; i < PMP_COUNT; i++) {
		pmp_get(i, &prot, &addr, &l2l);
		if (!(prot & PMP_A))
			continue;
		if (l2l < __riscv_xlen)
			size = (1UL << l2l);
		else
			size = 0;
#if __riscv_xlen == 32
		sbi_printf("PMP%d    : 0x%08lx-0x%08lx (A",
#else
		sbi_printf("PMP%d    : 0x%016lx-0x%016lx (A",
#endif
			   i, addr, addr + size - 1);
		if (prot & PMP_L)
			sbi_printf(",L");
		if (prot & PMP_R)
			sbi_printf(",R");
		if (prot & PMP_W)
			sbi_printf(",W");
		if (prot & PMP_X)
			sbi_printf(",X");
		sbi_printf(")\n");
	}
}

static int pmp_init(struct sbi_scratch *scratch, u32 hartid)
{
	u32 i, count;
	unsigned long fw_start, fw_size_log2;
	ulong prot, addr, log2size;
	const struct sbi_platform *plat = sbi_platform_ptr(scratch);

	if (!sbi_platform_has_pmp(plat))
		return 0;

	fw_size_log2 = log2roundup(scratch->fw_size);
	fw_start     = scratch->fw_start & ~((1UL << fw_size_log2) - 1UL);

	pmp_set(0, 0, fw_start, fw_size_log2);

	count = sbi_platform_pmp_region_count(plat, hartid);
	if ((PMP_COUNT - 1) < count)
		count = (PMP_COUNT - 1);

	for (i = 0; i < count; i++) {
		if (sbi_platform_pmp_region_info(plat, hartid, i, &prot, &addr,
						 &log2size))
			continue;
		pmp_set(i + 1, prot, addr, log2size);
	}

	return 0;
}

static unsigned long trap_info_offset;

int sbi_hart_init(struct sbi_scratch *scratch, u32 hartid, bool cold_boot)
{
	int rc;

	if (cold_boot) {
		trap_info_offset = sbi_scratch_alloc_offset(__SIZEOF_POINTER__,
							    "HART_TRAP_INFO");
		if (!trap_info_offset)
			return SBI_ENOMEM;
	}

	mstatus_init(scratch, hartid);

	rc = fp_init(hartid);
	if (rc)
		return rc;

	rc = delegate_traps(scratch, hartid);
	if (rc)
		return rc;

	return pmp_init(scratch, hartid);
}

void *sbi_hart_get_trap_info(struct sbi_scratch *scratch)
{
	unsigned long *trap_info;

	if (!trap_info_offset)
		return NULL;

	trap_info = sbi_scratch_offset_ptr(scratch, trap_info_offset);

	return (void *)(*trap_info);
}

void sbi_hart_set_trap_info(struct sbi_scratch *scratch, void *data)
{
	unsigned long *trap_info;

	if (!trap_info_offset)
		return;

	trap_info = sbi_scratch_offset_ptr(scratch, trap_info_offset);
	*trap_info = (unsigned long)data;
}

void __attribute__((noreturn)) sbi_hart_hang(void)
{
	while (1)
		wfi();
	__builtin_unreachable();
}

void __attribute__((noreturn))
sbi_hart_switch_mode(unsigned long arg0, unsigned long arg1,
		     unsigned long next_addr, unsigned long next_mode,
		     bool next_virt)
{
#if __riscv_xlen == 32
	unsigned long val, valH;
#else
	unsigned long val;
#endif

	switch (next_mode) {
	case PRV_M:
		break;
	case PRV_S:
		if (!misa_extension('S'))
			sbi_hart_hang();
		break;
	case PRV_U:
		if (!misa_extension('U'))
			sbi_hart_hang();
		break;
	default:
		sbi_hart_hang();
	}

	val = csr_read(CSR_MSTATUS);
	val = INSERT_FIELD(val, MSTATUS_MPP, next_mode);
	val = INSERT_FIELD(val, MSTATUS_MPIE, 0);
#if __riscv_xlen == 32
	if (misa_extension('H')) {
		valH = csr_read(CSR_MSTATUSH);
		if (next_virt)
			valH = INSERT_FIELD(valH, MSTATUSH_MPV, 1);
		else
			valH = INSERT_FIELD(valH, MSTATUSH_MPV, 0);
		csr_write(CSR_MSTATUSH, valH);
	}
#else
	if (misa_extension('H')) {
		if (next_virt)
			val = INSERT_FIELD(val, MSTATUS_MPV, 1);
		else
			val = INSERT_FIELD(val, MSTATUS_MPV, 0);
	}
#endif
	csr_write(CSR_MSTATUS, val);
	csr_write(CSR_MEPC, next_addr);

	if (next_mode == PRV_S) {
		csr_write(CSR_STVEC, next_addr);
		csr_write(CSR_SSCRATCH, 0);
		csr_write(CSR_SIE, 0);
		csr_write(CSR_SATP, 0);
	} else if (next_mode == PRV_U) {
		csr_write(CSR_UTVEC, next_addr);
		csr_write(CSR_USCRATCH, 0);
		csr_write(CSR_UIE, 0);
	}

	register unsigned long a0 asm("a0") = arg0;
	register unsigned long a1 asm("a1") = arg1;
	__asm__ __volatile__("mret" : : "r"(a0), "r"(a1));
	__builtin_unreachable();
}

static spinlock_t avail_hart_mask_lock	      = SPIN_LOCK_INITIALIZER;
static volatile unsigned long avail_hart_mask = 0;

void sbi_hart_mark_available(u32 hartid)
{
	spin_lock(&avail_hart_mask_lock);
	avail_hart_mask |= (1UL << hartid);
	spin_unlock(&avail_hart_mask_lock);
}

void sbi_hart_unmark_available(u32 hartid)
{
	spin_lock(&avail_hart_mask_lock);
	avail_hart_mask &= ~(1UL << hartid);
	spin_unlock(&avail_hart_mask_lock);
}

ulong sbi_hart_available_mask(void)
{
	ulong ret;

	spin_lock(&avail_hart_mask_lock);
	ret = avail_hart_mask;
	spin_unlock(&avail_hart_mask_lock);

	return ret;
}

typedef struct sbi_scratch *(*h2s)(ulong hartid);

struct sbi_scratch *sbi_hart_id_to_scratch(struct sbi_scratch *scratch,
					   u32 hartid)
{
	return ((h2s)scratch->hartid_to_scratch)(hartid);
}

#define COLDBOOT_WAIT_BITMAP_SIZE __riscv_xlen
static spinlock_t coldboot_lock = SPIN_LOCK_INITIALIZER;
static unsigned long coldboot_done = 0;
static unsigned long coldboot_wait_bitmap = 0;

void sbi_hart_wait_for_coldboot(struct sbi_scratch *scratch, u32 hartid)
{
	unsigned long saved_mie;
	const struct sbi_platform *plat = sbi_platform_ptr(scratch);

	if ((sbi_platform_hart_count(plat) <= hartid) ||
	    (COLDBOOT_WAIT_BITMAP_SIZE <= hartid))
		sbi_hart_hang();

	/* Save MIE CSR */
	saved_mie = csr_read(CSR_MIE);

	/* Set MSIE bit to receive IPI */
	csr_set(CSR_MIE, MIP_MSIP);

	/* Acquire coldboot lock */
	spin_lock(&coldboot_lock);

	/* Mark current HART as waiting */
	coldboot_wait_bitmap |= (1UL << hartid);

	/* Wait for coldboot to finish using WFI */
	while (!coldboot_done) {
		spin_unlock(&coldboot_lock);
		wfi();
		spin_lock(&coldboot_lock);
	};

	/* Unmark current HART as waiting */
	coldboot_wait_bitmap &= ~(1UL << hartid);

	/* Release coldboot lock */
	spin_unlock(&coldboot_lock);

	/* Restore MIE CSR */
	csr_write(CSR_MIE, saved_mie);

	/* Clear current HART IPI */
	sbi_platform_ipi_clear(plat, hartid);
}

void sbi_hart_wake_coldboot_harts(struct sbi_scratch *scratch, u32 hartid)
{
	const struct sbi_platform *plat = sbi_platform_ptr(scratch);
	int max_hart			= sbi_platform_hart_count(plat);

	/* Acquire coldboot lock */
	spin_lock(&coldboot_lock);

	/* Mark coldboot done */
	coldboot_done = 1;

	/* Send an IPI to all HARTs waiting for coldboot */
	for (int i = 0; i < max_hart; i++) {
		if ((i != hartid) && (coldboot_wait_bitmap & (1UL << i)))
			sbi_platform_ipi_send(plat, i);
	}

	/* Release coldboot lock */
	spin_unlock(&coldboot_lock);
}
