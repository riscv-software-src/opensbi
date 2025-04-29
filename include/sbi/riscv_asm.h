/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#ifndef __RISCV_ASM_H__
#define __RISCV_ASM_H__

#include <sbi/riscv_encoding.h>

/* clang-format off */

#ifdef __ASSEMBLER__
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

#define PAGE_SHIFT	(12)
#define PAGE_SIZE	(_AC(1, UL) << PAGE_SHIFT)
#define PAGE_MASK	(~(PAGE_SIZE - 1))

#define REG_L		__REG_SEL(ld, lw)
#define REG_S		__REG_SEL(sd, sw)
#define SZREG		__REG_SEL(8, 4)
#define LGREG		__REG_SEL(3, 2)

#if __SIZEOF_POINTER__ == 8
#ifdef __ASSEMBLER__
#define RISCV_PTR		.dword
#define RISCV_SZPTR		8
#define RISCV_LGPTR		3
#else
#define RISCV_PTR		".dword"
#define RISCV_SZPTR		"8"
#define RISCV_LGPTR		"3"
#endif
#elif __SIZEOF_POINTER__ == 4
#ifdef __ASSEMBLER__
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

/* clang-format on */

#ifndef __ASSEMBLER__

#define csr_swap(csr, val)                                              \
	({                                                              \
		unsigned long __v = (unsigned long)(val);               \
		__asm__ __volatile__("csrrw %0, " __ASM_STR(csr) ", %1" \
				     : "=r"(__v)                        \
				     : "rK"(__v)                        \
				     : "memory");                       \
		__v;                                                    \
	})

#define csr_read(csr)                                           \
	({                                                      \
		register unsigned long __v;                     \
		__asm__ __volatile__("csrr %0, " __ASM_STR(csr) \
				     : "=r"(__v)                \
				     :                          \
				     : "memory");               \
		__v;                                            \
	})

/* Variant of csr_read() that allows the compiler to cache the value. */
#define csr_read_relaxed(csr)                                     \
	({                                                        \
		register unsigned long __v;                       \
		__asm__ ("csrr %0, " __ASM_STR(csr) : "=r"(__v)); \
		__v;                                              \
	})

#define csr_write(csr, val)                                        \
	({                                                         \
		unsigned long __v = (unsigned long)(val);          \
		__asm__ __volatile__("csrw " __ASM_STR(csr) ", %0" \
				     :                             \
				     : "rK"(__v)                   \
				     : "memory");                  \
	})

#define csr_read_set(csr, val)                                          \
	({                                                              \
		unsigned long __v = (unsigned long)(val);               \
		__asm__ __volatile__("csrrs %0, " __ASM_STR(csr) ", %1" \
				     : "=r"(__v)                        \
				     : "rK"(__v)                        \
				     : "memory");                       \
		__v;                                                    \
	})

#define csr_set(csr, val)                                          \
	({                                                         \
		unsigned long __v = (unsigned long)(val);          \
		__asm__ __volatile__("csrs " __ASM_STR(csr) ", %0" \
				     :                             \
				     : "rK"(__v)                   \
				     : "memory");                  \
	})

#define csr_read_clear(csr, val)                                        \
	({                                                              \
		unsigned long __v = (unsigned long)(val);               \
		__asm__ __volatile__("csrrc %0, " __ASM_STR(csr) ", %1" \
				     : "=r"(__v)                        \
				     : "rK"(__v)                        \
				     : "memory");                       \
		__v;                                                    \
	})

#define csr_clear(csr, val)                                        \
	({                                                         \
		unsigned long __v = (unsigned long)(val);          \
		__asm__ __volatile__("csrc " __ASM_STR(csr) ", %0" \
				     :                             \
				     : "rK"(__v)                   \
				     : "memory");                  \
	})

#if __riscv_xlen == 64
#define __csrrw64(op, csr, csrh, val) (true ? op(csr, val) : (uint64_t)csrh)
#define __csrr64( op, csr, csrh)      (true ? op(csr)      : (uint64_t)csrh)
#define __csrw64( op, csr, csrh, val) (true ? op(csr, val) : (uint64_t)csrh)
#elif __riscv_xlen == 32
#define __csrrw64(op, csr, csrh, val) (  op(csr, val) | (uint64_t)op(csrh, val >> 32) << 32)
#define __csrr64( op, csr, csrh)      (  op(csr)      | (uint64_t)op(csrh)            << 32)
#define __csrw64( op, csr, csrh, val) ({ op(csr, val);            op(csrh, val >> 32);    })
#endif

#define csr_swap64(        csr, val) __csrrw64(csr_swap,         csr, csr ## H, val)
#define csr_read64(        csr)      __csrr64 (csr_read,         csr, csr ## H)
#define csr_read_relaxed64(csr)      __csrr64 (csr_read_relaxed, csr, csr ## H)
#define csr_write64(       csr, val) __csrw64 (csr_write,        csr, csr ## H, val)
#define csr_read_set64(    csr, val) __csrrw64(csr_read_set,     csr, csr ## H, val)
#define csr_set64(         csr, val) __csrw64 (csr_set,          csr, csr ## H, val)
#define csr_clear64(       csr, val) __csrw64 (csr_clear,        csr, csr ## H, val)
#define csr_read_clear64(  csr, val) __csrrw64(csr_read_clear,   csr, csr ## H, val)
#define csr_clear64(       csr, val) __csrw64 (csr_clear,        csr, csr ## H, val)

unsigned long csr_read_num(int csr_num);

void csr_write_num(int csr_num, unsigned long val);

#define wfi()                                             \
	do {                                              \
		__asm__ __volatile__("wfi" ::: "memory"); \
	} while (0)

#define ebreak()                                             \
	do {                                              \
		__asm__ __volatile__("ebreak" ::: "memory"); \
	} while (0)

/* Get current HART id */
#define current_hartid()	((unsigned int)csr_read_relaxed(CSR_MHARTID))

/* determine CPU extension, return non-zero support */
int misa_extension_imp(char ext);

#define misa_extension(c)\
({\
	_Static_assert(((c >= 'A') && (c <= 'Z')),\
		"The parameter of misa_extension must be [A-Z]");\
	misa_extension_imp(c);\
})

/* Get MXL field of misa, return -1 on error */
int misa_xlen(void);

/* Get RISC-V ISA string representation */
void misa_string(int xlen, char *out, unsigned int out_sz);

/* Disable pmp entry at a given index */
int pmp_disable(unsigned int n);

/* Check if the matching field is set */
int is_pmp_entry_mapped(unsigned long entry);

int pmp_set(unsigned int n, unsigned long prot, unsigned long addr,
	    unsigned long log2len);

int pmp_get(unsigned int n, unsigned long *prot_out, unsigned long *addr_out,
	    unsigned long *log2len);

#endif /* !__ASSEMBLER__ */

#endif
