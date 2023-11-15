/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <sbi/sbi_bitops.h>
#include <sbi/riscv_asm.h>
#include <sbi/riscv_atomic.h>
#include <sbi/riscv_barrier.h>

#ifndef __riscv_atomic
#error "opensbi strongly relies on the A extension of RISC-V"
#endif

long atomic_read(atomic_t *atom)
{
	long ret = atom->counter;
	rmb();
	return ret;
}

void atomic_write(atomic_t *atom, long value)
{
	atom->counter = value;
	wmb();
}

long atomic_add_return(atomic_t *atom, long value)
{
	long ret;
#if __SIZEOF_LONG__ == 4
	__asm__ __volatile__("	amoadd.w.aqrl  %1, %2, %0"
			     : "+A"(atom->counter), "=r"(ret)
			     : "r"(value)
			     : "memory");
#elif __SIZEOF_LONG__ == 8
	__asm__ __volatile__("	amoadd.d.aqrl  %1, %2, %0"
			     : "+A"(atom->counter), "=r"(ret)
			     : "r"(value)
			     : "memory");
#endif
	return ret + value;
}

long atomic_sub_return(atomic_t *atom, long value)
{
	return atomic_add_return(atom, -value);
}

#define __axchg(ptr, new, size)							\
	({									\
		__typeof__(ptr) __ptr = (ptr);					\
		__typeof__(new) __new = (new);					\
		__typeof__(*(ptr)) __ret;					\
		switch (size) {							\
		case 4:								\
			__asm__ __volatile__ (					\
				"	amoswap.w.aqrl %0, %2, %1\n"		\
				: "=r" (__ret), "+A" (*__ptr)			\
				: "r" (__new)					\
				: "memory");					\
			break;							\
		case 8:								\
			__asm__ __volatile__ (					\
				"	amoswap.d.aqrl %0, %2, %1\n"		\
				: "=r" (__ret), "+A" (*__ptr)			\
				: "r" (__new)					\
				: "memory");					\
			break;							\
		default:							\
			break;							\
		}								\
		__ret;								\
	})

#define axchg(ptr, x)								\
	({									\
		__typeof__(*(ptr)) _x_ = (x);					\
		(__typeof__(*(ptr))) __axchg((ptr), _x_, sizeof(*(ptr)));	\
	})

long atomic_cmpxchg(atomic_t *atom, long oldval, long newval)
{
	return __sync_val_compare_and_swap(&atom->counter, oldval, newval);
}

long atomic_xchg(atomic_t *atom, long newval)
{
	/* Atomically set new value and return old value. */
	return axchg(&atom->counter, newval);
}

unsigned int atomic_raw_xchg_uint(volatile unsigned int *ptr,
				  unsigned int newval)
{
	/* Atomically set new value and return old value. */
	return axchg(ptr, newval);
}

unsigned long atomic_raw_xchg_ulong(volatile unsigned long *ptr,
				    unsigned long newval)
{
	/* Atomically set new value and return old value. */
	return axchg(ptr, newval);
}

#if (__SIZEOF_POINTER__ == 8)
#define __AMO(op) "amo" #op ".d"
#elif (__SIZEOF_POINTER__ == 4)
#define __AMO(op) "amo" #op ".w"
#else
#error "Unexpected __SIZEOF_POINTER__"
#endif

#define __atomic_op_bit_ord(op, mod, nr, addr, ord)                          \
	({                                                                   \
		unsigned long __res, __mask;                                 \
		__mask = BIT_MASK(nr);                                       \
		__asm__ __volatile__(__AMO(op) #ord " %0, %2, %1"            \
				     : "=r"(__res), "+A"(addr[BIT_WORD(nr)]) \
				     : "r"(mod(__mask))                      \
				     : "memory");                            \
		__res & __mask ? 1 : 0;                                      \
	})

#define __atomic_op_bit(op, mod, nr, addr) \
	__atomic_op_bit_ord(op, mod, nr, addr, .aqrl)

/* Bitmask modifiers */
#define __NOP(x) (x)
#define __NOT(x) (~(x))

int atomic_raw_set_bit(int nr, volatile unsigned long *addr)
{
	return __atomic_op_bit(or, __NOP, nr, addr);
}

int atomic_raw_clear_bit(int nr, volatile unsigned long *addr)
{
	return __atomic_op_bit(and, __NOT, nr, addr);
}

int atomic_set_bit(int nr, atomic_t *atom)
{
	return atomic_raw_set_bit(nr, (unsigned long *)&atom->counter);
}

int atomic_clear_bit(int nr, atomic_t *atom)
{
	return atomic_raw_clear_bit(nr, (unsigned long *)&atom->counter);
}
