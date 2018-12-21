/*
 * Copyright (c) 2018 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_barrier.h>
#include <sbi/riscv_encoding.h>
#include <sbi/riscv_locks.h>
#include <sbi/sbi_bits.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_platform.h>

static void mstatus_init(struct sbi_scratch *scratch, u32 hartid)
{
	struct sbi_platform *plat = sbi_platform_ptr(scratch);

	/* Enable FPU */
	if (misa_extension('D') || misa_extension('F'))
		csr_write(mstatus, MSTATUS_FS);

	/* Enable user/supervisor use of perf counters */
	if (misa_extension('S') &&
	     sbi_platform_has_scounteren(plat))
		csr_write(scounteren, -1);
	if (sbi_platform_has_mcounteren(plat))
		csr_write(mcounteren, -1);

	/* Disable all interrupts */
	csr_write(mie, 0);

	/* Disable S-mode paging */
	if (misa_extension('S'))
		csr_write(sptbr, 0);
}

#ifdef __riscv_flen
static void init_fp_reg(int i)
{
	/* TODO: */
}
#endif

static int fp_init(u32 hartid)
{
#ifdef __riscv_flen
	int i;
#else
	unsigned long fd_mask;
#endif

	if (!misa_extension('D') && !misa_extension('F'))
		return 0;

	if (!(csr_read(mstatus) & MSTATUS_FS))
		return SBI_EINVAL;

#ifdef __riscv_flen
	for (i = 0; i < 32; i++)
		init_fp_reg(i);
	csr_write(fcsr, 0);
#else
	fd_mask = (1 << ('F' - 'A')) | (1 << ('D' - 'A'));
	csr_clear(misa, fd_mask);
	if (csr_read(misa) & fd_mask)
		return SBI_ENOTSUPP;
#endif

	return 0;
}

static int delegate_traps(u32 hartid)
{
	unsigned long interrupts, exceptions;

	if (!misa_extension('S')) {
		/* No delegation possible */
		interrupts = 0;
		exceptions = 0;
	} else {
		/* Send M-mode interrupts and most exceptions to S-mode */
		interrupts = MIP_SSIP | MIP_STIP | MIP_SEIP;
		exceptions = (1U << CAUSE_MISALIGNED_FETCH) |
			     (1U << CAUSE_BREAKPOINT) |
			     (1U << CAUSE_USER_ECALL);
	}

	csr_write(mideleg, interrupts);
	csr_write(medeleg, exceptions);

	if (csr_read(mideleg) != interrupts)
		return SBI_EFAIL;
	if (csr_read(medeleg) != exceptions)
		return SBI_EFAIL;

	return 0;
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
	struct sbi_platform *plat = sbi_platform_ptr(scratch);
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
		sbi_printf("PMP%d: 0x%08lx-0x%08lx (A",
#else
		sbi_printf("PMP%d: 0x%016lx-0x%016lx (A",
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
	struct sbi_platform *plat = sbi_platform_ptr(scratch);

	if (!sbi_platform_has_pmp(plat))
		return 0;

	fw_size_log2 = log2roundup(scratch->fw_size);
	fw_start = scratch->fw_start & ~((1UL << fw_size_log2) - 1UL);

	pmp_set(0, 0, fw_start, fw_size_log2);

	count = sbi_platform_pmp_region_count(plat, hartid);
	if ((PMP_COUNT - 1) < count)
		count = (PMP_COUNT - 1);

	for (i = 0; i < count; i++) {
		if (sbi_platform_pmp_region_info(plat, hartid, i,
						 &prot, &addr, &log2size))
			continue;
		pmp_set(i + 1, prot, addr, log2size);
	}

	return 0;
}

int sbi_hart_init(struct sbi_scratch *scratch, u32 hartid)
{
	int rc;

	mstatus_init(scratch, hartid);

	rc = fp_init(hartid);
	if (rc)
		return rc;

	rc = delegate_traps(hartid);
	if (rc)
		return rc;

	return pmp_init(scratch, hartid);
}

void __attribute__((noreturn)) sbi_hart_hang(void)
{
	sbi_printf("\nHang !!\n");

	while (1)
		wfi();
	__builtin_unreachable();
}

void __attribute__((noreturn)) sbi_hart_switch_mode(unsigned long arg0,
						    unsigned long arg1,
						    unsigned long next_addr,
						    unsigned long next_mode)
{
	unsigned long val;
	char mode = 'M';

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
		sbi_printf("\nTrying to switch to unsupported mode\n");
		sbi_hart_hang();
	}

	val = csr_read(mstatus);
	val = INSERT_FIELD(val, MSTATUS_MPP, next_mode);
	val = INSERT_FIELD(val, MSTATUS_MPIE, 0);

	csr_write(mstatus, val);
	csr_write(mepc, next_addr);

	if (next_mode == PRV_S) {
		mode = 'S';
		csr_write(stvec, next_addr);
		csr_write(sscratch, 0);
		csr_write(sie, 0);
		csr_write(satp, 0);
	} else if (next_mode == PRV_U) {
		mode = 'U';
		csr_write(utvec, next_addr);
		csr_write(uscratch, 0);
		csr_write(uie, 0);
	}

	sbi_printf("\nSwitching to %c-mode...\n\n", mode);

	register unsigned long a0 asm ("a0") = arg0;
	register unsigned long a1 asm ("a1") = arg1;
	__asm__ __volatile__ ("mret" : : "r" (a0), "r" (a1));
	__builtin_unreachable();
}

static spinlock_t avail_hart_mask_lock = SPIN_LOCK_INITIALIZER;
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

#define COLDBOOT_WAIT_BITMAP_SIZE	__riscv_xlen
static spinlock_t coldboot_wait_bitmap_lock = SPIN_LOCK_INITIALIZER;
static unsigned long coldboot_wait_bitmap = 0;

void sbi_hart_wait_for_coldboot(struct sbi_scratch *scratch, u32 hartid)
{
	unsigned long mipval;
	struct sbi_platform *plat = sbi_platform_ptr(scratch);

	if ((sbi_platform_hart_count(plat) <= hartid) ||
	    (COLDBOOT_WAIT_BITMAP_SIZE <= hartid))
		sbi_hart_hang();
	do {
		spin_lock(&coldboot_wait_bitmap_lock);
		coldboot_wait_bitmap |= (1UL << hartid);
		spin_unlock(&coldboot_wait_bitmap_lock);

		wfi();
		mipval = csr_read(mip);

		spin_lock(&coldboot_wait_bitmap_lock);
		coldboot_wait_bitmap &= ~(1UL << hartid);
		spin_unlock(&coldboot_wait_bitmap_lock);
	} while (!(mipval && MIP_MSIP));

	csr_clear(mip, MIP_MSIP);
}

void sbi_hart_wake_coldboot_harts(struct sbi_scratch *scratch, u32 hartid)
{
	struct sbi_platform *plat = sbi_platform_ptr(scratch);
	int max_hart = sbi_platform_hart_count(plat);

	for(int i = 0; i < max_hart ; i++) {
		/* send an IPI to every other hart */
		spin_lock(&coldboot_wait_bitmap_lock);
		if ((i != hartid) && (coldboot_wait_bitmap & (1UL << i)))
			sbi_platform_ipi_inject(plat, i, hartid);
		spin_unlock(&coldboot_wait_bitmap_lock);
	}
}
