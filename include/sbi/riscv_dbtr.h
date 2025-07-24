/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Ventana Micro System, Inc.
 *
 * Authors:
 *   Himanshu Chauhan <hchauhan@ventanamicro.com>
 */

#ifndef __RISCV_DBTR_H__
#define __RISCV_DBTR_H__

#define RV_MAX_TRIGGERS	32

enum {
	RISCV_DBTR_TRIG_NONE = 0,
	RISCV_DBTR_TRIG_LEGACY,
	RISCV_DBTR_TRIG_MCONTROL,
	RISCV_DBTR_TRIG_ICOUNT,
	RISCV_DBTR_TRIG_ITRIGGER,
	RISCV_DBTR_TRIG_ETRIGGER,
	RISCV_DBTR_TRIG_MCONTROL6,
};

#define RV_DBTR_BIT(_prefix, _name)		\
	RV_DBTR_##_prefix##_##_name##_BIT

#define RV_DBTR_BIT_MASK(_prefix, _name)	\
	RV_DBTR_##_prefix##_name##_BIT_MASK

#define RV_DBTR_DECLARE_BIT(_prefix, _name, _val)	\
	RV_DBTR_BIT(_prefix, _name) = _val

#define RV_DBTR_DECLARE_BIT_MASK(_prefix, _name, _width)		\
	RV_DBTR_BIT_MASK(_prefix, _name) =				\
		(((1UL << _width) - 1) << RV_DBTR_BIT(_prefix, _name))

#define CLEAR_DBTR_BIT(_target, _prefix, _bit_name)		\
	__clear_bit(RV_DBTR_BIT(_prefix, _bit_name), &_target)

#define SET_DBTR_BIT(_target, _prefix, _bit_name)		\
	__set_bit(RV_DBTR_BIT(_prefix, _bit_name), &_target)

/* Trigger Data 1 */
enum {
	RV_DBTR_DECLARE_BIT(TDATA1, DATA,   0),
#if __riscv_xlen == 64
	RV_DBTR_DECLARE_BIT(TDATA1, DMODE,  59),
	RV_DBTR_DECLARE_BIT(TDATA1, TYPE,   60),
#elif __riscv_xlen == 32
	RV_DBTR_DECLARE_BIT(TDATA1, DMODE,  27),
	RV_DBTR_DECLARE_BIT(TDATA1, TYPE,   28),
#else
	#error "Unknown __riscv_xlen"
#endif
};

enum {
#if __riscv_xlen == 64
	RV_DBTR_DECLARE_BIT_MASK(TDATA1, DATA,  59),
#elif __riscv_xlen == 32
	RV_DBTR_DECLARE_BIT_MASK(TDATA1, DATA,  27),
#else
	#error "Unknown __riscv_xlen"
#endif
	RV_DBTR_DECLARE_BIT_MASK(TDATA1, DMODE, 1),
	RV_DBTR_DECLARE_BIT_MASK(TDATA1, TYPE,  4),
};

/* MC - Match Control Type Register */
enum {
	RV_DBTR_DECLARE_BIT(MC, LOAD,    0),
	RV_DBTR_DECLARE_BIT(MC, STORE,   1),
	RV_DBTR_DECLARE_BIT(MC, EXEC,    2),
	RV_DBTR_DECLARE_BIT(MC, U,       3),
	RV_DBTR_DECLARE_BIT(MC, S,       4),
	RV_DBTR_DECLARE_BIT(MC, RES2,    5),
	RV_DBTR_DECLARE_BIT(MC, M,       6),
	RV_DBTR_DECLARE_BIT(MC, MATCH,   7),
	RV_DBTR_DECLARE_BIT(MC, CHAIN,   11),
	RV_DBTR_DECLARE_BIT(MC, ACTION,  12),
	RV_DBTR_DECLARE_BIT(MC, SIZELO,  16),
	RV_DBTR_DECLARE_BIT(MC, TIMING,  18),
	RV_DBTR_DECLARE_BIT(MC, SELECT,  19),
	RV_DBTR_DECLARE_BIT(MC, HIT,     20),
#if __riscv_xlen >= 64
	RV_DBTR_DECLARE_BIT(MC, SIZEHI,  21),
#endif
#if __riscv_xlen == 64
	RV_DBTR_DECLARE_BIT(MC, MASKMAX, 53),
	RV_DBTR_DECLARE_BIT(MC, DMODE,   59),
	RV_DBTR_DECLARE_BIT(MC, TYPE,    60),
#elif __riscv_xlen == 32
	RV_DBTR_DECLARE_BIT(MC, MASKMAX, 21),
	RV_DBTR_DECLARE_BIT(MC, DMODE,   27),
	RV_DBTR_DECLARE_BIT(MC, TYPE,    28),
#else
	#error "Unknown __riscv_xlen"
#endif
};

enum {
	RV_DBTR_DECLARE_BIT_MASK(MC, LOAD,    1),
	RV_DBTR_DECLARE_BIT_MASK(MC, STORE,   1),
	RV_DBTR_DECLARE_BIT_MASK(MC, EXEC,    1),
	RV_DBTR_DECLARE_BIT_MASK(MC, U,       1),
	RV_DBTR_DECLARE_BIT_MASK(MC, S,       1),
	RV_DBTR_DECLARE_BIT_MASK(MC, RES2,    1),
	RV_DBTR_DECLARE_BIT_MASK(MC, M,       1),
	RV_DBTR_DECLARE_BIT_MASK(MC, MATCH,   4),
	RV_DBTR_DECLARE_BIT_MASK(MC, CHAIN,   1),
	RV_DBTR_DECLARE_BIT_MASK(MC, ACTION,  4),
	RV_DBTR_DECLARE_BIT_MASK(MC, SIZELO,  2),
	RV_DBTR_DECLARE_BIT_MASK(MC, TIMING,  1),
	RV_DBTR_DECLARE_BIT_MASK(MC, SELECT,  1),
	RV_DBTR_DECLARE_BIT_MASK(MC, HIT,     1),
#if __riscv_xlen >= 64
	RV_DBTR_DECLARE_BIT_MASK(MC, SIZEHI,  2),
#endif
	RV_DBTR_DECLARE_BIT_MASK(MC, MASKMAX, 6),
	RV_DBTR_DECLARE_BIT_MASK(MC, DMODE,   1),
	RV_DBTR_DECLARE_BIT_MASK(MC, TYPE,    4),
};


/* ICOUNT - Match Control Type Register */
enum {
	RV_DBTR_DECLARE_BIT(ICOUNT, ACTION,   0),
	RV_DBTR_DECLARE_BIT(ICOUNT, U,        6),
	RV_DBTR_DECLARE_BIT(ICOUNT, S,        7),
	RV_DBTR_DECLARE_BIT(ICOUNT, PENDING,  8),
	RV_DBTR_DECLARE_BIT(ICOUNT, M,        9),
	RV_DBTR_DECLARE_BIT(ICOUNT, COUNT,   10),
	RV_DBTR_DECLARE_BIT(ICOUNT, HIT,     24),
	RV_DBTR_DECLARE_BIT(ICOUNT, VU,      25),
	RV_DBTR_DECLARE_BIT(ICOUNT, VS,      26),
#if __riscv_xlen == 64
	RV_DBTR_DECLARE_BIT(ICOUNT, DMODE,   59),
	RV_DBTR_DECLARE_BIT(ICOUNT, TYPE,    60),
#elif __riscv_xlen == 32
	RV_DBTR_DECLARE_BIT(ICOUNT, DMODE,   27),
	RV_DBTR_DECLARE_BIT(ICOUNT, TYPE,    28),
#else
	#error "Unknown __riscv_xlen"
#endif
};

enum {
	RV_DBTR_DECLARE_BIT_MASK(ICOUNT, ACTION,   6),
	RV_DBTR_DECLARE_BIT_MASK(ICOUNT, U,        1),
	RV_DBTR_DECLARE_BIT_MASK(ICOUNT, S,        1),
	RV_DBTR_DECLARE_BIT_MASK(ICOUNT, PENDING,  1),
	RV_DBTR_DECLARE_BIT_MASK(ICOUNT, M,        1),
	RV_DBTR_DECLARE_BIT_MASK(ICOUNT, COUNT,   14),
	RV_DBTR_DECLARE_BIT_MASK(ICOUNT, HIT,      1),
	RV_DBTR_DECLARE_BIT_MASK(ICOUNT, VU,       1),
	RV_DBTR_DECLARE_BIT_MASK(ICOUNT, VS,       1),
#if __riscv_xlen == 64
	RV_DBTR_DECLARE_BIT_MASK(ICOUNT, DMODE,    1),
	RV_DBTR_DECLARE_BIT_MASK(ICOUNT, TYPE,     4),
#elif __riscv_xlen == 32
	RV_DBTR_DECLARE_BIT_MASK(ICOUNT, DMODE,    1),
	RV_DBTR_DECLARE_BIT_MASK(ICOUNT, TYPE,     4),
#else
	#error "Unknown __riscv_xlen"
#endif
};

/* MC6 - Match Control 6 Type Register */
enum {
	RV_DBTR_DECLARE_BIT(MC6, LOAD,   0),
	RV_DBTR_DECLARE_BIT(MC6, STORE,  1),
	RV_DBTR_DECLARE_BIT(MC6, EXEC,   2),
	RV_DBTR_DECLARE_BIT(MC6, U,      3),
	RV_DBTR_DECLARE_BIT(MC6, S,      4),
	RV_DBTR_DECLARE_BIT(MC6, RES2,   5),
	RV_DBTR_DECLARE_BIT(MC6, M,      6),
	RV_DBTR_DECLARE_BIT(MC6, MATCH,  7),
	RV_DBTR_DECLARE_BIT(MC6, CHAIN,  11),
	RV_DBTR_DECLARE_BIT(MC6, ACTION, 12),
	RV_DBTR_DECLARE_BIT(MC6, SIZE,   16),
	RV_DBTR_DECLARE_BIT(MC6, TIMING, 20),
	RV_DBTR_DECLARE_BIT(MC6, SELECT, 21),
	RV_DBTR_DECLARE_BIT(MC6, HIT,    22),
	RV_DBTR_DECLARE_BIT(MC6, VU,     23),
	RV_DBTR_DECLARE_BIT(MC6, VS,     24),
#if __riscv_xlen == 64
	RV_DBTR_DECLARE_BIT(MC6, DMODE,  59),
	RV_DBTR_DECLARE_BIT(MC6, TYPE,   60),
#elif __riscv_xlen == 32
	RV_DBTR_DECLARE_BIT(MC6, DMODE,  27),
	RV_DBTR_DECLARE_BIT(MC6, TYPE,   28),
#else
	#error "Unknown __riscv_xlen"
#endif
};

enum {
	RV_DBTR_DECLARE_BIT_MASK(MC6, LOAD,   1),
	RV_DBTR_DECLARE_BIT_MASK(MC6, STORE,  1),
	RV_DBTR_DECLARE_BIT_MASK(MC6, EXEC,   1),
	RV_DBTR_DECLARE_BIT_MASK(MC6, U,      1),
	RV_DBTR_DECLARE_BIT_MASK(MC6, S,      1),
	RV_DBTR_DECLARE_BIT_MASK(MC6, RES2,   1),
	RV_DBTR_DECLARE_BIT_MASK(MC6, M,      1),
	RV_DBTR_DECLARE_BIT_MASK(MC6, MATCH,  4),
	RV_DBTR_DECLARE_BIT_MASK(MC6, CHAIN,  1),
	RV_DBTR_DECLARE_BIT_MASK(MC6, ACTION, 4),
	RV_DBTR_DECLARE_BIT_MASK(MC6, SIZE,   4),
	RV_DBTR_DECLARE_BIT_MASK(MC6, TIMING, 1),
	RV_DBTR_DECLARE_BIT_MASK(MC6, SELECT, 1),
	RV_DBTR_DECLARE_BIT_MASK(MC6, HIT,    1),
	RV_DBTR_DECLARE_BIT_MASK(MC6, VU,     1),
	RV_DBTR_DECLARE_BIT_MASK(MC6, VS,     1),
#if __riscv_xlen == 64
	RV_DBTR_DECLARE_BIT_MASK(MC6, DMODE,  1),
	RV_DBTR_DECLARE_BIT_MASK(MC6, TYPE,   4),
#elif __riscv_xlen == 32
	RV_DBTR_DECLARE_BIT_MASK(MC6, DMODE,  1),
	RV_DBTR_DECLARE_BIT_MASK(MC6, TYPE,   4),
#else
	#error "Unknown __riscv_xlen"
#endif
};

#define RV_DBTR_SET_TDATA1_TYPE(_t1, _type)				\
	do {								\
		_t1 &= ~RV_DBTR_BIT_MASK(TDATA1, TYPE);			\
		_t1 |= (((unsigned long)_type				\
			 << RV_DBTR_BIT(TDATA1, TYPE))			\
			& RV_DBTR_BIT_MASK(TDATA1, TYPE));		\
	}while (0);

#define RV_DBTR_SET_MC_TYPE(_t1, _type)				\
	do {							\
		_t1 &= ~RV_DBTR_BIT_MASK(MC, TYPE);		\
		_t1 |= (((unsigned long)_type			\
			 << RV_DBTR_BIT(MC, TYPE))		\
			& RV_DBTR_BIT_MASK(MC, TYPE));		\
	}while (0);

#define RV_DBTR_SET_MC6_TYPE(_t1, _type)			\
	do {							\
		_t1 &= ~RV_DBTR_BIT_MASK(MC6, TYPE);		\
		_t1 |= (((unsigned long)_type			\
			 << RV_DBTR_BIT(MC6, TYPE))		\
			& RV_DBTR_BIT_MASK(MC6, TYPE));		\
	}while (0);

#define RV_DBTR_SET_MC_EXEC(_t1)		\
	SET_DBTR_BIT(_t1, MC, EXEC)

#define RV_DBTR_SET_MC_LOAD(_t1)		\
	SET_DBTR_BIT(_t1, MC, LOAD)

#define RV_DBTR_SET_MC_STORE(_t1)		\
	SET_DBTR_BIT(_t1, MC, STORE)

#define RV_DBTR_SET_MC_SIZELO(_t1, _val)			\
	do {							\
		_t1 &= ~RV_DBTR_BIT_MASK(MC, SIZELO);		\
		_t1 |= ((_val << RV_DBTR_BIT(MC, SIZELO))	\
			& RV_DBTR_BIT_MASK(MC, SIZELO));	\
	} while(0);

#define RV_DBTR_SET_MC_SIZEHI(_t1, _val)			\
	do {							\
		_t1 &= ~RV_DBTR_BIT_MASK(MC, SIZEHI);		\
		_t1 |= ((_val << RV_DBTR_BIT(MC, SIZEHI))	\
			& RV_DBTR_BIT_MASK(MC, SIZEHI));	\
	} while(0);

#define RV_DBTR_SET_MC6_EXEC(_t1)		\
	SET_DBTR_BIT(_t1, MC6, EXEC)

#define RV_DBTR_SET_MC6_LOAD(_t1)		\
	SET_DBTR_BIT(_t1, MC6, LOAD)

#define RV_DBTR_SET_MC6_STORE(_t1)		\
	SET_DBTR_BIT(_t1, MC6, STORE)

#define RV_DBTR_SET_MC6_SIZE(_t1, _val)				\
	do {							\
		_t1 &= ~RV_DBTR_BIT_MASK(MC6, SIZE);		\
		_t1 |= ((_val << RV_DBTR_BIT(MC6, SIZE))	\
			& RV_DBTR_BIT_MASK(MC6, SIZE));		\
	} while(0);

typedef unsigned long riscv_dbtr_tdata1_mcontrol_t;
typedef unsigned long riscv_dbtr_tdata1_mcontrol6_t;
typedef unsigned long riscv_dbtr_tdata1_t;

#endif /* __RISCV_DBTR_H__ */
