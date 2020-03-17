/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_encoding.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_platform.h>

/* determine CPU extension, return non-zero support */
int misa_extension_imp(char ext)
{
	unsigned long misa = csr_read(CSR_MISA);

	if (misa)
		return misa & (1 << (ext - 'A'));

	return sbi_platform_misa_extension(sbi_platform_thishart_ptr(), ext);
}

int misa_xlen(void)
{
	long r;

	if (csr_read(CSR_MISA) == 0)
		return sbi_platform_misa_xlen(sbi_platform_thishart_ptr());

	__asm__ __volatile__(
		"csrr   t0, misa\n\t"
		"slti   t1, t0, 0\n\t"
		"slli   t1, t1, 1\n\t"
		"slli   t0, t0, 1\n\t"
		"slti   t0, t0, 0\n\t"
		"add    %0, t0, t1"
		: "=r"(r)
		:
		: "t0", "t1");

	return r ? r : -1;
}

unsigned long csr_read_num(int csr_num)
{
	unsigned long ret = 0;

	switch (csr_num) {
	case CSR_PMPCFG0:
		ret = csr_read(CSR_PMPCFG0);
		break;
	case CSR_PMPCFG1:
		ret = csr_read(CSR_PMPCFG1);
		break;
	case CSR_PMPCFG2:
		ret = csr_read(CSR_PMPCFG2);
		break;
	case CSR_PMPCFG3:
		ret = csr_read(CSR_PMPCFG3);
		break;
	case CSR_PMPADDR0:
		ret = csr_read(CSR_PMPADDR0);
		break;
	case CSR_PMPADDR1:
		ret = csr_read(CSR_PMPADDR1);
		break;
	case CSR_PMPADDR2:
		ret = csr_read(CSR_PMPADDR2);
		break;
	case CSR_PMPADDR3:
		ret = csr_read(CSR_PMPADDR3);
		break;
	case CSR_PMPADDR4:
		ret = csr_read(CSR_PMPADDR4);
		break;
	case CSR_PMPADDR5:
		ret = csr_read(CSR_PMPADDR5);
		break;
	case CSR_PMPADDR6:
		ret = csr_read(CSR_PMPADDR6);
		break;
	case CSR_PMPADDR7:
		ret = csr_read(CSR_PMPADDR7);
		break;
	case CSR_PMPADDR8:
		ret = csr_read(CSR_PMPADDR8);
		break;
	case CSR_PMPADDR9:
		ret = csr_read(CSR_PMPADDR9);
		break;
	case CSR_PMPADDR10:
		ret = csr_read(CSR_PMPADDR10);
		break;
	case CSR_PMPADDR11:
		ret = csr_read(CSR_PMPADDR11);
		break;
	case CSR_PMPADDR12:
		ret = csr_read(CSR_PMPADDR12);
		break;
	case CSR_PMPADDR13:
		ret = csr_read(CSR_PMPADDR13);
		break;
	case CSR_PMPADDR14:
		ret = csr_read(CSR_PMPADDR14);
		break;
	case CSR_PMPADDR15:
		ret = csr_read(CSR_PMPADDR15);
		break;
	default:
		break;
	};

	return ret;
}

void csr_write_num(int csr_num, unsigned long val)
{
	switch (csr_num) {
	case CSR_PMPCFG0:
		csr_write(CSR_PMPCFG0, val);
		break;
	case CSR_PMPCFG1:
		csr_write(CSR_PMPCFG1, val);
		break;
	case CSR_PMPCFG2:
		csr_write(CSR_PMPCFG2, val);
		break;
	case CSR_PMPCFG3:
		csr_write(CSR_PMPCFG3, val);
		break;
	case CSR_PMPADDR0:
		csr_write(CSR_PMPADDR0, val);
		break;
	case CSR_PMPADDR1:
		csr_write(CSR_PMPADDR1, val);
		break;
	case CSR_PMPADDR2:
		csr_write(CSR_PMPADDR2, val);
		break;
	case CSR_PMPADDR3:
		csr_write(CSR_PMPADDR3, val);
		break;
	case CSR_PMPADDR4:
		csr_write(CSR_PMPADDR4, val);
		break;
	case CSR_PMPADDR5:
		csr_write(CSR_PMPADDR5, val);
		break;
	case CSR_PMPADDR6:
		csr_write(CSR_PMPADDR6, val);
		break;
	case CSR_PMPADDR7:
		csr_write(CSR_PMPADDR7, val);
		break;
	case CSR_PMPADDR8:
		csr_write(CSR_PMPADDR8, val);
		break;
	case CSR_PMPADDR9:
		csr_write(CSR_PMPADDR9, val);
		break;
	case CSR_PMPADDR10:
		csr_write(CSR_PMPADDR10, val);
		break;
	case CSR_PMPADDR11:
		csr_write(CSR_PMPADDR11, val);
		break;
	case CSR_PMPADDR12:
		csr_write(CSR_PMPADDR12, val);
		break;
	case CSR_PMPADDR13:
		csr_write(CSR_PMPADDR13, val);
		break;
	case CSR_PMPADDR14:
		csr_write(CSR_PMPADDR14, val);
		break;
	case CSR_PMPADDR15:
		csr_write(CSR_PMPADDR15, val);
		break;
	default:
		break;
	};
}

static unsigned long ctz(unsigned long x)
{
	unsigned long ret = 0;

	while (!(x & 1UL)) {
		ret++;
		x = x >> 1;
	}

	return ret;
}

int pmp_set(unsigned int n, unsigned long prot, unsigned long addr,
	    unsigned long log2len)
{
	int pmpcfg_csr, pmpcfg_shift, pmpaddr_csr;
	unsigned long cfgmask, pmpcfg;
	unsigned long addrmask, pmpaddr;

	/* check parameters */
	if (n >= PMP_COUNT || log2len > __riscv_xlen || log2len < PMP_SHIFT)
		return SBI_EINVAL;

	/* calculate PMP register and offset */
#if __riscv_xlen == 32
	pmpcfg_csr   = CSR_PMPCFG0 + (n >> 2);
	pmpcfg_shift = (n & 3) << 3;
#elif __riscv_xlen == 64
	pmpcfg_csr   = (CSR_PMPCFG0 + (n >> 2)) & ~1;
	pmpcfg_shift = (n & 7) << 3;
#else
	pmpcfg_csr   = -1;
	pmpcfg_shift = -1;
#endif
	pmpaddr_csr = CSR_PMPADDR0 + n;
	if (pmpcfg_csr < 0 || pmpcfg_shift < 0)
		return SBI_ENOTSUPP;

	/* encode PMP config */
	prot |= (log2len == PMP_SHIFT) ? PMP_A_NA4 : PMP_A_NAPOT;
	cfgmask = ~(0xff << pmpcfg_shift);
	pmpcfg	= (csr_read_num(pmpcfg_csr) & cfgmask);
	pmpcfg |= ((prot << pmpcfg_shift) & ~cfgmask);

	/* encode PMP address */
	if (log2len == PMP_SHIFT) {
		pmpaddr = (addr >> PMP_SHIFT);
	} else {
		if (log2len == __riscv_xlen) {
			pmpaddr = -1UL;
		} else {
			addrmask = (1UL << (log2len - PMP_SHIFT)) - 1;
			pmpaddr	 = ((addr >> PMP_SHIFT) & ~addrmask);
			pmpaddr |= (addrmask >> 1);
		}
	}

	/* write csrs */
	csr_write_num(pmpaddr_csr, pmpaddr);
	csr_write_num(pmpcfg_csr, pmpcfg);

	return 0;
}

int pmp_get(unsigned int n, unsigned long *prot_out, unsigned long *addr_out,
	    unsigned long *size)
{
	int pmpcfg_csr, pmpcfg_shift, pmpaddr_csr;
	unsigned long cfgmask, pmpcfg, prot;
	unsigned long t1, addr, log2len;

	/* check parameters */
	if (n >= PMP_COUNT || !prot_out || !addr_out || !size)
		return SBI_EINVAL;
	*prot_out = *addr_out = *size = 0;

	/* calculate PMP register and offset */
#if __riscv_xlen == 32
	pmpcfg_csr   = CSR_PMPCFG0 + (n >> 2);
	pmpcfg_shift = (n & 3) << 3;
#elif __riscv_xlen == 64
	pmpcfg_csr   = (CSR_PMPCFG0 + (n >> 2)) & ~1;
	pmpcfg_shift = (n & 7) << 3;
#else
	pmpcfg_csr   = -1;
	pmpcfg_shift = -1;
#endif
	pmpaddr_csr = CSR_PMPADDR0 + n;
	if (pmpcfg_csr < 0 || pmpcfg_shift < 0)
		return SBI_ENOTSUPP;

	/* decode PMP config */
	cfgmask = (0xff << pmpcfg_shift);
	pmpcfg	= csr_read_num(pmpcfg_csr) & cfgmask;
	prot	= pmpcfg >> pmpcfg_shift;

	/* decode PMP address */
	if ((prot & PMP_A) == PMP_A_NAPOT) {
		addr = csr_read_num(pmpaddr_csr);
		if (addr == -1UL) {
			addr	= 0;
			log2len = __riscv_xlen;
		} else {
			t1	= ctz(~addr);
			addr	= (addr & ~((1UL << t1) - 1)) << PMP_SHIFT;
			log2len = (t1 + PMP_SHIFT + 1);
		}
	} else {
		addr	= csr_read_num(pmpaddr_csr) << PMP_SHIFT;
		log2len = PMP_SHIFT;
	}

	/* return details */
	*prot_out    = prot;
	*addr_out    = addr;

	if (log2len < __riscv_xlen)
		*size = (1UL << log2len);

	return 0;
}
