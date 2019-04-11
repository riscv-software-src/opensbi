/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <sbi/sbi_types.h>
#include <sbi/riscv_asm.h>
#include <sbi/riscv_atomic.h>
#include <sbi/riscv_barrier.h>
#include <sbi/sbi_bits.h>

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

	__asm__ __volatile__("	amoadd.w.aqrl  %1, %2, %0"
			     : "+A"(atom->counter), "=r"(ret)
			     : "r"(value)
			     : "memory");

	return ret + value;
}

long atomic_sub_return(atomic_t *atom, long value)
{
	long ret;

	__asm__ __volatile__("	amoadd.w.aqrl  %1, %2, %0"
			     : "+A"(atom->counter), "=r"(ret)
			     : "r"(-value)
			     : "memory");

	return ret - value;
}

#define __xchg(ptr, new, size)                                            \
	({                                                                \
		__typeof__(ptr) __ptr	 = (ptr);                         \
		__typeof__(*(ptr)) __new = (new);                         \
		__typeof__(*(ptr)) __ret;                                 \
		register unsigned int __rc;                               \
		switch (size) {                                           \
		case 4:                                                   \
			__asm__ __volatile__("0:	lr.w %0, %2\n"           \
					     "	sc.w.rl %1, %z3, %2\n"     \
					     "	bnez %1, 0b\n"             \
					     "	fence rw, rw\n"            \
					     : "=&r"(__ret), "=&r"(__rc), \
					       "+A"(*__ptr)               \
					     : "rJ"(__new)                \
					     : "memory");                 \
			break;                                            \
		case 8:                                                   \
			__asm__ __volatile__("0:	lr.d %0, %2\n"           \
					     "	sc.d.rl %1, %z3, %2\n"     \
					     "	bnez %1, 0b\n"             \
					     "	fence rw, rw\n"            \
					     : "=&r"(__ret), "=&r"(__rc), \
					       "+A"(*__ptr)               \
					     : "rJ"(__new)                \
					     : "memory");                 \
			break;                                            \
		default:                                                  \
			break;                                            \
		}                                                         \
		__ret;                                                    \
	})

#define xchg(ptr, n)                                                     \
	({                                                               \
		__typeof__(*(ptr)) _n_ = (n);                            \
		(__typeof__(*(ptr))) __xchg((ptr), _n_, sizeof(*(ptr))); \
	})

#define __cmpxchg(ptr, old, new, size)                                    \
	({                                                                \
		__typeof__(ptr) __ptr	 = (ptr);                         \
		__typeof__(*(ptr)) __old = (old);                         \
		__typeof__(*(ptr)) __new = (new);                         \
		__typeof__(*(ptr)) __ret;                                 \
		register unsigned int __rc;                               \
		switch (size) {                                           \
		case 4:                                                   \
			__asm__ __volatile__("0:	lr.w %0, %2\n"           \
					     "	bne  %0, %z3, 1f\n"        \
					     "	sc.w.rl %1, %z4, %2\n"     \
					     "	bnez %1, 0b\n"             \
					     "	fence rw, rw\n"            \
					     "1:\n"                       \
					     : "=&r"(__ret), "=&r"(__rc), \
					       "+A"(*__ptr)               \
					     : "rJ"(__old), "rJ"(__new)   \
					     : "memory");                 \
			break;                                            \
		case 8:                                                   \
			__asm__ __volatile__("0:	lr.d %0, %2\n"           \
					     "	bne %0, %z3, 1f\n"         \
					     "	sc.d.rl %1, %z4, %2\n"     \
					     "	bnez %1, 0b\n"             \
					     "	fence rw, rw\n"            \
					     "1:\n"                       \
					     : "=&r"(__ret), "=&r"(__rc), \
					       "+A"(*__ptr)               \
					     : "rJ"(__old), "rJ"(__new)   \
					     : "memory");                 \
			break;                                            \
		default:                                                  \
			break;                                            \
		}                                                         \
		__ret;                                                    \
	})

#define cmpxchg(ptr, o, n)                                          \
	({                                                          \
		__typeof__(*(ptr)) _o_ = (o);                       \
		__typeof__(*(ptr)) _n_ = (n);                       \
		(__typeof__(*(ptr)))                                \
			__cmpxchg((ptr), _o_, _n_, sizeof(*(ptr))); \
	})

long arch_atomic_cmpxchg(atomic_t *atom, long oldval, long newval)
{
#ifdef __riscv_atomic
	return __sync_val_compare_and_swap(&atom->counter, oldval, newval);
#else
	return cmpxchg(&atom->counter, oldval, newval);
#endif
}

long arch_atomic_xchg(atomic_t *atom, long newval)
{
	/* Atomically set new value and return old value. */
#ifdef __riscv_atomic
	/*
	 * The name of GCC built-in macro __sync_lock_test_and_set()
	 * is misleading. A more appropriate name for GCC built-in
	 * macro would be __sync_val_exchange().
	 */
	return __sync_lock_test_and_set(&atom->counter, newval);
#else
	return xchg(&atom->counter, newval);
#endif
}

unsigned int atomic_raw_xchg_uint(volatile unsigned int *ptr,
				  unsigned int newval)
{
	/* Atomically set new value and return old value. */
#ifdef __riscv_atomic
	/*
	 * The name of GCC built-in macro __sync_lock_test_and_set()
	 * is misleading. A more appropriate name for GCC built-in
	 * macro would be __sync_val_exchange().
	 */
	return __sync_lock_test_and_set(ptr, newval);
#else
	return xchg(ptr, newval);
#endif
}

#if (BITS_PER_LONG == 64)
#define __AMO(op) "amo" #op ".d"
#elif (BITS_PER_LONG == 32)
#define __AMO(op) "amo" #op ".w"
#else
#error "Unexpected BITS_PER_LONG"
#endif

#define __atomic_op_bit_ord(op, mod, nr, addr, ord)                          \
	({                                                                   \
		unsigned long __res, __mask;                                 \
		__mask = BIT_MASK(nr);                                       \
		__asm__ __volatile__(__AMO(op) #ord " %0, %2, %1"            \
				     : "=r"(__res), "+A"(addr[BIT_WORD(nr)]) \
				     : "r"(mod(__mask))                      \
				     : "memory");                            \
		__res;                                                       \
	})

#define __atomic_op_bit(op, mod, nr, addr) \
	__atomic_op_bit_ord(op, mod, nr, addr, .aqrl)

/* Bitmask modifiers */
#define __NOP(x) (x)
#define __NOT(x) (~(x))

inline int atomic_raw_set_bit(int nr, volatile unsigned long *addr)
{
	return __atomic_op_bit(or, __NOP, nr, addr);
}

inline int atomic_raw_clear_bit(int nr, volatile unsigned long *addr)
{
	return __atomic_op_bit(and, __NOT, nr, addr);
}

inline int atomic_set_bit(int nr, atomic_t *atom)
{
	return atomic_raw_set_bit(nr, (unsigned long *)&atom->counter);
}

inline int atomic_clear_bit(int nr, atomic_t *atom)
{
	return atomic_raw_clear_bit(nr, (unsigned long *)&atom->counter);
}
