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
struct sbi_trap_regs;
struct sbi_ecall_return;

#define EXC_MODE_PP_SHIFT		0
#define EXC_MODE_PP			BIT(EXC_MODE_PP_SHIFT)
#define EXC_MODE_PV_SHIFT		1
#define EXC_MODE_PV			BIT(EXC_MODE_PV_SHIFT)
#define EXC_MODE_SSTATUS_SPIE_SHIFT	2
#define EXC_MODE_SSTATUS_SPIE		BIT(EXC_MODE_SSTATUS_SPIE_SHIFT)

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

	/**
	 * Called when the SBI_EXT_SSE_ENABLE is invoked on the event.
	 */
	void (*enable_cb)(uint32_t event_id);

	/**
	 * Called when the SBI_EXT_SSE_DISABLE is invoked on the event.
	 */
	void (*disable_cb)(uint32_t event_id);
};

/* Add a supported event with associated callback operations
 * @param event_id Event identifier (SBI_SSE_EVENT_* or a custom platform one)
 * @param cb_ops Callback operations (Can be NULL if any)
 * @return 0 on success, error otherwise
 */
int sbi_sse_add_event(uint32_t event_id, const struct sbi_sse_cb_ops *cb_ops);

/* Inject an event to the current hard
 * @param event_id Event identifier (SBI_SSE_EVENT_*)
 * @param regs Registers that were used on SBI entry
 * @return 0 on success, error otherwise
 */
int sbi_sse_inject_event(uint32_t event_id);

void sbi_sse_process_pending_events(struct sbi_trap_regs *regs);


int sbi_sse_init(struct sbi_scratch *scratch, bool cold_boot);
void sbi_sse_exit(struct sbi_scratch *scratch);

/* Interface called from sbi_ecall_sse.c */
int sbi_sse_register(uint32_t event_id, unsigned long handler_entry_pc,
		     unsigned long handler_entry_arg);
int sbi_sse_unregister(uint32_t event_id);
int sbi_sse_hart_mask(void);
int sbi_sse_hart_unmask(void);
int sbi_sse_enable(uint32_t event_id);
int sbi_sse_disable(uint32_t event_id);
int sbi_sse_complete(struct sbi_trap_regs *regs, struct sbi_ecall_return *out);
int sbi_sse_inject_from_ecall(uint32_t event_id, unsigned long hart_id,
			      struct sbi_ecall_return *out);
int sbi_sse_read_attrs(uint32_t event_id, uint32_t base_attr_id,
		       uint32_t attr_count, unsigned long output_phys_lo,
		       unsigned long output_phys_hi);
int sbi_sse_write_attrs(uint32_t event_id, uint32_t base_attr_id,
			uint32_t attr_count, unsigned long input_phys_lo,
			unsigned long input_phys_hi);

#endif
