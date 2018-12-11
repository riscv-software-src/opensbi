/*
 * Copyright (c) 2018 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef __RISCV_ATOMIC_H__
#define __RISCV_ATOMIC_H__

typedef struct {
	volatile long counter;
} atomic_t;

#define ATOMIC_INIT(_lptr, val)		\
	(_lptr)->counter = (val)

#define ATOMIC_INITIALIZER(val)		\
	{ .counter = (val), }

long atomic_read(atomic_t *atom);

void atomic_write(atomic_t *atom, long value);

long atomic_add_return(atomic_t *atom, long value);

long atomic_sub_return(atomic_t *atom, long value);

long arch_atomic_cmpxchg(atomic_t *atom, long oldval, long newval);

long arch_atomic_xchg(atomic_t *atom, long newval);

unsigned int atomic_raw_xchg_uint(volatile unsigned int *ptr,
				  unsigned int newval);

#endif
