/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Rivos Systems.
 */

#ifndef __SBI_SSE_H__
#define __SBI_SSE_H__

#include <sbi/sbi_types.h>
#include <sbi/sbi_list.h>
#include <sbi/riscv_locks.h>

struct sbi_scratch;

#define EXC_MODE_PP_SHIFT		0
#define EXC_MODE_PP			BIT(EXC_MODE_PP_SHIFT)
#define EXC_MODE_PV_SHIFT		1
#define EXC_MODE_PV			BIT(EXC_MODE_PV_SHIFT)
#define EXC_MODE_SSTATUS_SPIE_SHIFT	2
#define EXC_MODE_SSTATUS_SPIE		BIT(EXC_MODE_SSTATUS_SPIE_SHIFT)

struct sse_entry_state {
	/** Entry program counter */
	unsigned long pc;
	/** ra register state */
	unsigned long ra;
	/** sp register state */
	unsigned long sp;
	/** gp register state */
	unsigned long gp;
	/** tp register state */
	unsigned long tp;
	/** t0 register state */
	unsigned long t0;
	/** t1 register state */
	unsigned long t1;
	/** t2 register state */
	unsigned long t2;
	/** s0 register state */
	unsigned long s0;
	/** s1 register state */
	unsigned long s1;
	/** a0 register state */
	unsigned long a0;
	/** a1 register state */
	unsigned long a1;
	/** a2 register state */
	unsigned long a2;
	/** a3 register state */
	unsigned long a3;
	/** a4 register state */
	unsigned long a4;
	/** a5 register state */
	unsigned long a5;
	/** a6 register state */
	unsigned long a6;
	/** a7 register state */
	unsigned long a7;
	/** s2 register state */
	unsigned long s2;
	/** s3 register state */
	unsigned long s3;
	/** s4 register state */
	unsigned long s4;
	/** s5 register state */
	unsigned long s5;
	/** s6 register state */
	unsigned long s6;
	/** s7 register state */
	unsigned long s7;
	/** s8 register state */
	unsigned long s8;
	/** s9 register state */
	unsigned long s9;
	/** s10 register state */
	unsigned long s10;
	/** s11 register state */
	unsigned long s11;
	/** t3 register state */
	unsigned long t3;
	/** t4 register state */
	unsigned long t4;
	/** t5 register state */
	unsigned long t5;
	/** t6 register state */
	unsigned long t6;
};

struct sse_interrupted_state {
	/** Interrupted program counter */
	unsigned long pc;
	/** ra register state */
	unsigned long ra;
	/** sp register state */
	unsigned long sp;
	/** gp register state */
	unsigned long gp;
	/** tp register state */
	unsigned long tp;
	/** t0 register state */
	unsigned long t0;
	/** t1 register state */
	unsigned long t1;
	/** t2 register state */
	unsigned long t2;
	/** s0 register state */
	unsigned long s0;
	/** s1 register state */
	unsigned long s1;
	/** a0 register state */
	unsigned long a0;
	/** a1 register state */
	unsigned long a1;
	/** a2 register state */
	unsigned long a2;
	/** a3 register state */
	unsigned long a3;
	/** a4 register state */
	unsigned long a4;
	/** a5 register state */
	unsigned long a5;
	/** a6 register state */
	unsigned long a6;
	/** a7 register state */
	unsigned long a7;
	/** s2 register state */
	unsigned long s2;
	/** s3 register state */
	unsigned long s3;
	/** s4 register state */
	unsigned long s4;
	/** s5 register state */
	unsigned long s5;
	/** s6 register state */
	unsigned long s6;
	/** s7 register state */
	unsigned long s7;
	/** s8 register state */
	unsigned long s8;
	/** s9 register state */
	unsigned long s9;
	/** s10 register state */
	unsigned long s10;
	/** s11 register state */
	unsigned long s11;
	/** t3 register state */
	unsigned long t3;
	/** t4 register state */
	unsigned long t4;
	/** t5 register state */
	unsigned long t5;
	/** t6 register state */
	unsigned long t6;
	/** Exception mode */
	unsigned long exc_mode;
};

struct sbi_sse_handler_ctx {
	struct sse_entry_state entry;
	struct sse_interrupted_state interrupted;
};

enum sbi_sse_state {
	SSE_STATE_UNUSED = 0,
	SSE_STATE_REGISTERED = 1,
	SSE_STATE_ENABLED = 2,
	SSE_STATE_RUNNING = 3,
};

struct sbi_sse_cb_ops {
	/**
	 * Called when hart_id is changed on the event.
	 */
	void (*set_hartid_cb)(uint32_t event_id, unsigned long hart_id);

	/**
	 * Called when the SBI_EXT_SSE_COMPLETE is invoked on the event.
	 */
	void (*complete_cb)(uint32_t event_id);

	/**
	 * Called when the SBI_EXT_SSE_REGISTER is invoked on the event.
	 */
	void (*register_cb)(uint32_t event_id);

	/**
	 * Called when the SBI_EXT_SSE_UNREGISTER is invoked on the event.
	 */
	void (*unregister_cb)(uint32_t event_id);
};

/* Set the callback operations for an event
 * @param event_id Event identifier (SBI_SSE_EVENT_*)
 * @param cb_ops Callback operations
 * @return 0 on success, error otherwise
 */
int sbi_sse_set_cb_ops(uint32_t event_id, const struct sbi_sse_cb_ops *cb_ops);

/* Inject an event to the current hard
 * @param event_id Event identifier (SBI_SSE_EVENT_*)
 * @param regs Registers that were used on SBI entry
 * @return 0 on success, error otherwise
 */
int sbi_sse_inject_event(uint32_t event_id, struct sbi_trap_regs *regs);

int sbi_sse_init(struct sbi_scratch *scratch, bool cold_boot);
void sbi_sse_exit(struct sbi_scratch *scratch);

/* Interface called from sbi_ecall_sse.c */
int sbi_sse_get_attr(uint32_t event_id, uint32_t attr_id, unsigned long *out_val);
int sbi_sse_set_attr(uint32_t event_id, uint32_t attr_id, unsigned long value);
int sbi_sse_register(uint32_t event_id, unsigned long phys_hi, unsigned long phys_lo);
int sbi_sse_unregister(uint32_t event_id);
int sbi_sse_enable(uint32_t event_id);
int sbi_sse_disable(uint32_t event_id);
int sbi_sse_complete(uint32_t event_id, uint32_t status, uint32_t flags,
		     struct sbi_trap_regs *regs);
int sbi_sse_inject_from_ecall(uint32_t event_id, unsigned long hart_id,
			      struct sbi_trap_regs *regs);

#endif
