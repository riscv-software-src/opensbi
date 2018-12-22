/*
 * Copyright (c) 2018 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef __RISCV_ASM_H__
#define __RISCV_ASM_H__

#ifdef __ASSEMBLY__
#define __ASM_STR(x)	x
#else
#define __ASM_STR(x)	#x
#endif

#if __riscv_xlen == 64
#define __REG_SEL(a, b)	__ASM_STR(a)
#elif __riscv_xlen == 32
#define __REG_SEL(a, b)	__ASM_STR(b)
#else
#error "Unexpected __riscv_xlen"
#endif

#define REG_L		__REG_SEL(ld, lw)
#define REG_S		__REG_SEL(sd, sw)
#define SZREG		__REG_SEL(8, 4)
#define LGREG		__REG_SEL(3, 2)

#if __SIZEOF_POINTER__ == 8
#ifdef __ASSEMBLY__
#define RISCV_PTR		.dword
#define RISCV_SZPTR		8
#define RISCV_LGPTR		3
#else
#define RISCV_PTR		".dword"
#define RISCV_SZPTR		"8"
#define RISCV_LGPTR		"3"
#endif
#elif __SIZEOF_POINTER__ == 4
#ifdef __ASSEMBLY__
#define RISCV_PTR		.word
#define RISCV_SZPTR		4
#define RISCV_LGPTR		2
#else
#define RISCV_PTR		".word"
#define RISCV_SZPTR		"4"
#define RISCV_LGPTR		"2"
#endif
#else
#error "Unexpected __SIZEOF_POINTER__"
#endif

#if (__SIZEOF_INT__ == 4)
#define RISCV_INT		__ASM_STR(.word)
#define RISCV_SZINT		__ASM_STR(4)
#define RISCV_LGINT		__ASM_STR(2)
#else
#error "Unexpected __SIZEOF_INT__"
#endif

#if (__SIZEOF_SHORT__ == 2)
#define RISCV_SHORT		__ASM_STR(.half)
#define RISCV_SZSHORT		__ASM_STR(2)
#define RISCV_LGSHORT		__ASM_STR(1)
#else
#error "Unexpected __SIZEOF_SHORT__"
#endif

#define RISCV_SCRATCH_TMP0_OFFSET		(0 * __SIZEOF_POINTER__)
#define RISCV_SCRATCH_FW_START_OFFSET		(1 * __SIZEOF_POINTER__)
#define RISCV_SCRATCH_FW_SIZE_OFFSET		(2 * __SIZEOF_POINTER__)
#define RISCV_SCRATCH_NEXT_ARG1_OFFSET		(3 * __SIZEOF_POINTER__)
#define RISCV_SCRATCH_NEXT_ADDR_OFFSET		(4 * __SIZEOF_POINTER__)
#define RISCV_SCRATCH_NEXT_MODE_OFFSET		(5 * __SIZEOF_POINTER__)
#define RISCV_SCRATCH_WARMBOOT_ADDR_OFFSET	(6 * __SIZEOF_POINTER__)
#define RISCV_SCRATCH_PLATFORM_ADDR_OFFSET	(7 * __SIZEOF_POINTER__)
#define RISCV_SCRATCH_HARTID_TO_SCRATCH_OFFSET	(8 * __SIZEOF_POINTER__)
#define RISCV_SCRATCH_IPI_TYPE_OFFSET		(9 * __SIZEOF_POINTER__)
#define RISCV_SCRATCH_SIZE			256

#define RISCV_PLATFORM_NAME_OFFSET		(0x0)
#define RISCV_PLATFORM_FEATURES_OFFSET		(0x40)
#define RISCV_PLATFORM_HART_COUNT_OFFSET	(0x48)
#define RISCV_PLATFORM_HART_STACK_SIZE_OFFSET	(0x4c)

#define RISCV_TRAP_REGS_zero			0
#define RISCV_TRAP_REGS_ra			1
#define RISCV_TRAP_REGS_sp			2
#define RISCV_TRAP_REGS_gp			3
#define RISCV_TRAP_REGS_tp			4
#define RISCV_TRAP_REGS_t0			5
#define RISCV_TRAP_REGS_t1			6
#define RISCV_TRAP_REGS_t2			7
#define RISCV_TRAP_REGS_s0			8
#define RISCV_TRAP_REGS_s1			9
#define RISCV_TRAP_REGS_a0			10
#define RISCV_TRAP_REGS_a1			11
#define RISCV_TRAP_REGS_a2			12
#define RISCV_TRAP_REGS_a3			13
#define RISCV_TRAP_REGS_a4			14
#define RISCV_TRAP_REGS_a5			15
#define RISCV_TRAP_REGS_a6			16
#define RISCV_TRAP_REGS_a7			17
#define RISCV_TRAP_REGS_s2			18
#define RISCV_TRAP_REGS_s3			19
#define RISCV_TRAP_REGS_s4			20
#define RISCV_TRAP_REGS_s5			21
#define RISCV_TRAP_REGS_s6			22
#define RISCV_TRAP_REGS_s7			23
#define RISCV_TRAP_REGS_s8			24
#define RISCV_TRAP_REGS_s9			25
#define RISCV_TRAP_REGS_s10			26
#define RISCV_TRAP_REGS_s11			27
#define RISCV_TRAP_REGS_t3			28
#define RISCV_TRAP_REGS_t4			29
#define RISCV_TRAP_REGS_t5			30
#define RISCV_TRAP_REGS_t6			31
#define RISCV_TRAP_REGS_mepc			32
#define RISCV_TRAP_REGS_mstatus			33
#define RISCV_TRAP_REGS_last			34

#define RISCV_TRAP_REGS_OFFSET(x)		\
				((RISCV_TRAP_REGS_##x) * __SIZEOF_POINTER__)
#define RISCV_TRAP_REGS_SIZE			RISCV_TRAP_REGS_OFFSET(last)

#ifndef __ASSEMBLY__

#define csr_swap(csr, val)					\
({								\
	unsigned long __v = (unsigned long)(val);		\
	__asm__ __volatile__ ("csrrw %0, " #csr ", %1"		\
			      : "=r" (__v) : "rK" (__v)		\
			      : "memory");			\
	__v;							\
})

#define csr_read(csr)						\
({								\
	register unsigned long __v;				\
	__asm__ __volatile__ ("csrr %0, " #csr			\
			      : "=r" (__v) :			\
			      : "memory");			\
	__v;							\
})

#define csr_read_n(csr_num)					\
({								\
	register unsigned long __v;				\
	__asm__ __volatile__ ("csrr %0, " __ASM_STR(csr_num)	\
			      : "=r" (__v) :			\
			      : "memory");			\
	__v;							\
})

#define csr_write(csr, val)					\
({								\
	unsigned long __v = (unsigned long)(val);		\
	__asm__ __volatile__ ("csrw " #csr ", %0"		\
			      : : "rK" (__v)			\
			      : "memory");			\
})

#define csr_write_n(csr_num, val)				\
({								\
	unsigned long __v = (unsigned long)(val);		\
	__asm__ __volatile__ ("csrw " __ASM_STR(csr_num) ", %0"	\
			      : : "rK" (__v)			\
			      : "memory");			\
})

#define csr_read_set(csr, val)					\
({								\
	unsigned long __v = (unsigned long)(val);		\
	__asm__ __volatile__ ("csrrs %0, " #csr ", %1"		\
			      : "=r" (__v) : "rK" (__v)		\
			      : "memory");			\
	__v;							\
})

#define csr_set(csr, val)					\
({								\
	unsigned long __v = (unsigned long)(val);		\
	__asm__ __volatile__ ("csrs " #csr ", %0"		\
			      : : "rK" (__v)			\
			      : "memory");			\
})

#define csr_read_clear(csr, val)				\
({								\
	unsigned long __v = (unsigned long)(val);		\
	__asm__ __volatile__ ("csrrc %0, " #csr ", %1"		\
			      : "=r" (__v) : "rK" (__v)		\
			      : "memory");			\
	__v;							\
})

#define csr_clear(csr, val)					\
({								\
	unsigned long __v = (unsigned long)(val);		\
	__asm__ __volatile__ ("csrc " #csr ", %0"		\
			      : : "rK" (__v)			\
			      : "memory");			\
})

unsigned long csr_read_num(int csr_num);

void csr_write_num(int csr_num, unsigned long val);

#define wfi()							\
do {								\
	__asm__ __volatile__ ("wfi" ::: "memory");		\
} while (0)

static inline int misa_extension(char ext)
{
	return csr_read(misa) & (1 << (ext - 'A'));
}

static inline int misa_xlen(void)
{
	return ((long)csr_read(misa) < 0) ? 64 : 32;
}

static inline void misa_string(char *out, unsigned int out_sz)
{
	unsigned long i, val = csr_read(misa);

	for (i = 0; i < 26; i++) {
		if (val & (1 << i)) {
			*out = 'A' + i;
			out++;
		}
	}
	*out = '\0';
	out++;
}

int pmp_set(unsigned int n, unsigned long prot,
	    unsigned long addr, unsigned long log2len);

int pmp_get(unsigned int n, unsigned long *prot_out,
	    unsigned long *addr_out, unsigned long *log2len_out);

#endif /* !__ASSEMBLY__ */

#endif
