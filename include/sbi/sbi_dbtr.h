/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Ventana Micro Systems, Inc.
 *
 * Authors:
 *   Himanshu Chauhan <hchauhan@ventanamicro.com>
 */

#ifndef __SBI_DBTR_H__
#define __SBI_DBTR_H__

#include <sbi/riscv_dbtr.h>
#include <sbi/sbi_types.h>

struct sbi_domain;

enum {
	RV_DBTR_DECLARE_BIT(TS, MAPPED, 0), /* trigger mapped to hw trigger */
	RV_DBTR_DECLARE_BIT(TS, U, 1),
	RV_DBTR_DECLARE_BIT(TS, S, 2),
	RV_DBTR_DECLARE_BIT(TS, VU, 3),
	RV_DBTR_DECLARE_BIT(TS, VS, 4),
	RV_DBTR_DECLARE_BIT(TS, HAVE_TRIG, 5), /* H/w dbtr details available */
	RV_DBTR_DECLARE_BIT(TS, HW_IDX, 8), /* Hardware index of trigger */
};

enum {
	RV_DBTR_DECLARE_BIT_MASK(TS, MAPPED, 1),
	RV_DBTR_DECLARE_BIT_MASK(TS, U, 1),
	RV_DBTR_DECLARE_BIT_MASK(TS, S, 1),
	RV_DBTR_DECLARE_BIT_MASK(TS, VU, 1),
	RV_DBTR_DECLARE_BIT_MASK(TS, VS, 1),
	RV_DBTR_DECLARE_BIT_MASK(TS, HAVE_TRIG, 1),
	RV_DBTR_DECLARE_BIT_MASK(TS, HW_IDX, (__riscv_xlen-9)),
};

#if __riscv_xlen == 64
#define SBI_DBTR_SHMEM_INVALID_ADDR	0xFFFFFFFFFFFFFFFFUL
#elif __riscv_xlen == 32
#define SBI_DBTR_SHMEM_INVALID_ADDR	0xFFFFFFFFUL
#else
#error "Unexpected __riscv_xlen"
#endif

struct sbi_dbtr_shmem {
	unsigned long phys_lo;
	unsigned long phys_hi;
};

struct sbi_dbtr_trigger {
	unsigned long index;
	unsigned long type_mask;
	unsigned long state;
	unsigned long tdata1;
	unsigned long tdata2;
	unsigned long tdata3;
};

struct sbi_dbtr_data_msg {
	unsigned long tstate;
	unsigned long tdata1;
	unsigned long tdata2;
	unsigned long tdata3;
};

struct sbi_dbtr_id_msg {
	unsigned long idx;
};

struct sbi_dbtr_hart_triggers_state {
	struct sbi_dbtr_trigger triggers[RV_MAX_TRIGGERS];
	struct sbi_dbtr_shmem shmem;
	u32 total_trigs;
	u32 available_trigs;
	u32 hartid;
	u32 probed;
};

#define TDATA1_GET_TYPE(_t1)					\
	EXTRACT_FIELD(_t1, RV_DBTR_BIT_MASK(TDATA1, TYPE))

/* Set the hardware index of trigger in logical trigger state */
#define SET_TRIG_HW_INDEX(_state, _idx)				\
	do {							\
		_state &= ~RV_DBTR_BIT_MASK(TS, HW_IDX);	\
		_state |= (((unsigned long)_idx			\
			    << RV_DBTR_BIT(TS, HW_IDX))		\
			   & RV_DBTR_BIT_MASK(TS, HW_IDX));	\
	}while (0);

/** SBI shared mem messages layout */
union sbi_dbtr_shmem_entry {
	struct sbi_dbtr_data_msg data;
	struct sbi_dbtr_id_msg id;
};

#define SBI_DBTR_SHMEM_ALIGN_MASK	((__riscv_xlen / 8) - 1)

/** Initialize debug triggers */
int sbi_dbtr_init(struct sbi_scratch *scratch, bool coldboot);

/** SBI DBTR extension functions */
int sbi_dbtr_supported(void);
int sbi_dbtr_setup_shmem(const struct sbi_domain *dom, unsigned long smode,
			 unsigned long shmem_phys_lo,
			 unsigned long shmem_phys_hi);
int sbi_dbtr_num_trig(unsigned long trig_tdata1, unsigned long *out);
int sbi_dbtr_read_trig(unsigned long smode,
		       unsigned long trig_idx_base, unsigned long trig_count);
int sbi_dbtr_install_trig(unsigned long smode,
			  unsigned long trig_count, unsigned long *out);
int sbi_dbtr_uninstall_trig(unsigned long trig_idx_base,
			    unsigned long trig_idx_mask);
int sbi_dbtr_enable_trig(unsigned long trig_idx_base,
			 unsigned long trig_idx_mask);
int sbi_dbtr_update_trig(unsigned long smode,
			 unsigned long trig_count);
int sbi_dbtr_disable_trig(unsigned long trig_idx_base,
			  unsigned long trig_idx_mask);

int sbi_dbtr_get_total_triggers(void);

#endif
