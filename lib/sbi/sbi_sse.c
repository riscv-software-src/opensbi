/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Rivos Systems Inc.
 *
 */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_barrier.h>
#include <sbi/riscv_encoding.h>
#include <sbi/riscv_locks.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_ecall.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_fifo.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_heap.h>
#include <sbi/sbi_hsm.h>
#include <sbi/sbi_ipi.h>
#include <sbi/sbi_list.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_pmu.h>
#include <sbi/sbi_sse.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_string.h>
#include <sbi/sbi_trap.h>

#include <sbi/sbi_console.h>

#define sse_get_hart_state_ptr(__scratch) \
	sbi_scratch_read_type((__scratch), void *, shs_ptr_off)

#define sse_thishart_state_ptr() \
	sse_get_hart_state_ptr(sbi_scratch_thishart_ptr())

#define sse_set_hart_state_ptr(__scratch, __sse_state) \
	sbi_scratch_write_type((__scratch), void *, shs_ptr_off, (__sse_state))

#define EVENT_IS_GLOBAL(__event_id) ((__event_id) & SBI_SSE_EVENT_GLOBAL_BIT)

static const uint32_t supported_events[] = {
	SBI_SSE_EVENT_LOCAL_RAS,
	SBI_SSE_EVENT_LOCAL_DOUBLE_TRAP,
	SBI_SSE_EVENT_GLOBAL_RAS,
	SBI_SSE_EVENT_LOCAL_PMU,
	SBI_SSE_EVENT_LOCAL_SOFTWARE,
	SBI_SSE_EVENT_GLOBAL_SOFTWARE,
};

#define EVENT_COUNT array_size(supported_events)

#define sse_event_invoke_cb(_event, _cb, ...)                                 \
	{                                                                     \
		if (_event->cb_ops && _event->cb_ops->_cb)                    \
			_event->cb_ops->_cb(_event->event_id, ##__VA_ARGS__); \
	}

struct sse_entry_state {
	/** entry pc */
	unsigned long pc;
	/** a6 register state */
	unsigned long arg;
};

struct sse_interrupted_state {
	/** sepc register state */
	unsigned long sepc;
	/** flags register state */
	unsigned long flags;
	/** a6 register state */
	unsigned long a6;
	/** a7 register state */
	unsigned long a7;
};

struct sse_ipi_inject_data {
	uint32_t event_id;
};

struct sbi_sse_event_attrs {
	unsigned long status;
	unsigned long prio;
	unsigned long config;
	unsigned long hartid;
	struct sse_entry_state entry;
	struct sse_interrupted_state interrupted;
};

/* Make sure all attributes are packed for direct memcpy in ATTR_READ */
#define assert_field_offset(field, attr_offset)                  \
	_Static_assert(                                          \
		((offsetof(struct sbi_sse_event_attrs, field)) / \
		 sizeof(unsigned long)) == attr_offset,          \
		"field " #field                                  \
		" from struct sbi_sse_event_attrs invalid offset, expected " #attr_offset)

assert_field_offset(status, SBI_SSE_ATTR_STATUS);
assert_field_offset(prio, SBI_SSE_ATTR_PRIO);
assert_field_offset(config, SBI_SSE_ATTR_CONFIG);
assert_field_offset(hartid, SBI_SSE_ATTR_PREFERRED_HART);
assert_field_offset(entry.pc, SBI_SSE_ATTR_ENTRY_PC);
assert_field_offset(entry.arg, SBI_SSE_ATTR_ENTRY_ARG);
assert_field_offset(interrupted.sepc, SBI_SSE_ATTR_INTERRUPTED_SEPC);
assert_field_offset(interrupted.flags, SBI_SSE_ATTR_INTERRUPTED_FLAGS);
assert_field_offset(interrupted.a6, SBI_SSE_ATTR_INTERRUPTED_A6);
assert_field_offset(interrupted.a7, SBI_SSE_ATTR_INTERRUPTED_A7);

struct sbi_sse_event {
	struct sbi_sse_event_attrs attrs;
	uint32_t event_id;
	u32 hartindex;
	const struct sbi_sse_cb_ops *cb_ops;
	struct sbi_dlist node;
};

/** Per-hart state */
struct sse_hart_state {
	/* Priority sorted list of enabled events (global and local in >=
	 * ENABLED state). This list is protected by the enabled_event_lock.
	 *
	 * Global events can also be inserted in this list. Since these events
	 * can be accessed by all harts, we actually need to lock independently
	 * (see sse_global_event).
	 *
	 * Local events do not actually need to be locked since, we do
	 * not have preemption and there are solely accessed by the current
	 * hart. So when inserting a local event in this list, we just need to
	 * lock the list at insertion/removal time.
	 *
	 * When an event is in a state >= ENABLED, then it is inserted in the
	 * this enabled_event_list and thus can only be removed from this
	 * list upon disable ecall or on complete with ONESHOT flag.
	 */
	struct sbi_dlist enabled_event_list;

	/**
	 * Lock that protects enabled_event_list
	 */
	spinlock_t enabled_event_lock;

	/**
	 * List of local events allocated at boot time.
	 */
	struct sbi_sse_event *local_events;

	/**
	 * State to track if the hart is ready to take sse events.
	 * One hart cannot modify this state of another hart.
	 */
	bool masked;
};

/**
 * Global events are accessible by all harts
 */
struct sse_global_event {
	/**
	 * global event struct
	 */
	struct sbi_sse_event event;

	/**
	 * Global event lock protecting access from multiple harts from ecall to
	 * the event.
	 */
	spinlock_t lock;
};

static unsigned int local_event_count;
static unsigned int global_event_count;
static struct sse_global_event *global_events;

static unsigned long sse_inject_fifo_off;
static unsigned long sse_inject_fifo_mem_off;
/* Offset of pointer to SSE HART state in scratch space */
static unsigned long shs_ptr_off;

static u32 sse_ipi_inject_event = SBI_IPI_EVENT_MAX;

static int sse_ipi_inject_send(unsigned long hartid, uint32_t event_id);

static unsigned long sse_event_state(struct sbi_sse_event *e)
{
	return e->attrs.status & SBI_SSE_ATTR_STATUS_STATE_MASK;
}

static unsigned long sse_event_pending(struct sbi_sse_event *e)
{
	return !!(e->attrs.status & BIT(SBI_SSE_ATTR_STATUS_PENDING_OFFSET));
}

static bool sse_event_is_global(struct sbi_sse_event *e)
{
	return EVENT_IS_GLOBAL(e->event_id);
}

static bool sse_event_is_local(struct sbi_sse_event *e)
{
	return !sse_event_is_global(e);
}

/**
 * If event is global, must be called under global event lock
 */
static struct sse_hart_state *sse_get_hart_state(struct sbi_sse_event *e)
{
	struct sbi_scratch *s = sbi_hartindex_to_scratch(e->hartindex);

	return sse_get_hart_state_ptr(s);
}

static struct sse_global_event *sse_get_global_event(struct sbi_sse_event *e)
{
	return container_of(e, struct sse_global_event, event);
}

/**
 * If event is global, must be called under enabled event lock
 */
static void sse_enabled_event_lock(struct sbi_sse_event *e)
{
	struct sse_hart_state *shs;

	shs = sse_get_hart_state(e);
	spin_lock(&shs->enabled_event_lock);
}

/**
 * If event is global, must be called under enabled event lock
 */
static void sse_enabled_event_unlock(struct sbi_sse_event *e)
{
	struct sse_hart_state *shs;

	shs = sse_get_hart_state(e);
	spin_unlock(&shs->enabled_event_lock);
}

static void sse_event_set_state(struct sbi_sse_event *e,
				unsigned long new_state)
{
	e->attrs.status &= ~SBI_SSE_ATTR_STATUS_STATE_MASK;
	e->attrs.status |= new_state;
}

static int sse_event_get(uint32_t event_id, struct sbi_sse_event **eret)
{
	unsigned int i;
	struct sbi_sse_event *e;
	struct sse_hart_state *shs;

	if (!eret)
		return SBI_EINVAL;

	if (EVENT_IS_GLOBAL(event_id)) {
		for (i = 0; i < global_event_count; i++) {
			e = &global_events[i].event;
			if (e->event_id == event_id) {
				spin_lock(&global_events[i].lock);
				*eret = e;
				return SBI_SUCCESS;
			}
		}
	} else {
		shs = sse_thishart_state_ptr();
		for (i = 0; i < local_event_count; i++) {
			e = &shs->local_events[i];
			if (e->event_id == event_id) {
				*eret = e;
				return SBI_SUCCESS;
			}
		}
	}

	return SBI_EINVAL;
}

static void sse_event_put(struct sbi_sse_event *e)
{
	struct sse_global_event *ge;

	if (sse_event_is_local(e))
		return;

	ge = sse_get_global_event(e);
	spin_unlock(&ge->lock);
}

static void sse_event_remove_from_list(struct sbi_sse_event *e)
{
	sbi_list_del(&e->node);
}

/**
 * Must be called under owner hart lock
 */
static void sse_event_add_to_list(struct sbi_sse_event *e)
{
	struct sse_hart_state *state = sse_get_hart_state(e);
	struct sbi_sse_event *tmp;

	sbi_list_for_each_entry(tmp, &state->enabled_event_list, node) {
		if (e->attrs.prio < tmp->attrs.prio)
			break;
		if (e->attrs.prio == tmp->attrs.prio &&
		    e->event_id < tmp->event_id)
			break;
	}
	sbi_list_add_tail(&e->node, &tmp->node);
}

/**
 * Must be called under owner hart lock
 */
static int sse_event_disable(struct sbi_sse_event *e)
{
	if (sse_event_state(e) != SBI_SSE_STATE_ENABLED)
		return SBI_EINVALID_STATE;

	sse_event_invoke_cb(e, disable_cb);

	sse_event_remove_from_list(e);
	sse_event_set_state(e, SBI_SSE_STATE_REGISTERED);

	return SBI_OK;
}

static int sse_event_set_hart_id_check(struct sbi_sse_event *e,
				       unsigned long new_hartid)
{
	int hstate;
	unsigned int hartid = (uint32_t)new_hartid;
	struct sbi_domain *hd = sbi_domain_thishart_ptr();

	if (!sse_event_is_global(e))
		return SBI_EBAD_RANGE;

	if (!sbi_domain_is_assigned_hart(hd, sbi_hartid_to_hartindex(hartid)))
		return SBI_EINVAL;

	hstate = sbi_hsm_hart_get_state(hd, hartid);
	if (hstate != SBI_HSM_STATE_STARTED)
		return SBI_EINVAL;

	return SBI_OK;
}

static int sse_event_set_attr_check(struct sbi_sse_event *e, uint32_t attr_id,
				    unsigned long val)
{
	switch (attr_id) {
	case SBI_SSE_ATTR_CONFIG:
		if (sse_event_state(e) >= SBI_SSE_STATE_ENABLED)
			return SBI_EINVALID_STATE;

		if (val & ~SBI_SSE_ATTR_CONFIG_ONESHOT)
			return SBI_EINVAL;

		return SBI_OK;
	case SBI_SSE_ATTR_PRIO:
		if (sse_event_state(e) >= SBI_SSE_STATE_ENABLED)
			return SBI_EINVALID_STATE;

#if __riscv_xlen > 32
		if (val != (uint32_t)val)
			return SBI_EINVAL;
#endif
		return SBI_OK;
	case SBI_SSE_ATTR_PREFERRED_HART:
		if (sse_event_state(e) >= SBI_SSE_STATE_ENABLED)
			return SBI_EINVALID_STATE;

		return sse_event_set_hart_id_check(e, val);
	case SBI_SSE_ATTR_INTERRUPTED_FLAGS:
		if (val & ~(SBI_SSE_ATTR_INTERRUPTED_FLAGS_STATUS_SPP |
			    SBI_SSE_ATTR_INTERRUPTED_FLAGS_STATUS_SPIE |
			    SBI_SSE_ATTR_INTERRUPTED_FLAGS_HSTATUS_SPV |
			    SBI_SSE_ATTR_INTERRUPTED_FLAGS_HSTATUS_SPVP))
			return SBI_EINVAL;
		__attribute__((__fallthrough__));
	case SBI_SSE_ATTR_INTERRUPTED_SEPC:
	case SBI_SSE_ATTR_INTERRUPTED_A6:
	case SBI_SSE_ATTR_INTERRUPTED_A7:
		if (sse_event_state(e) != SBI_SSE_STATE_RUNNING)
			return SBI_EINVALID_STATE;

		if (current_hartid() != e->attrs.hartid)
			return SBI_EINVAL;

		return SBI_OK;
	default:
		return SBI_EBAD_RANGE;
	}
}

static void sse_event_set_attr(struct sbi_sse_event *e, uint32_t attr_id,
			       unsigned long val)
{
	switch (attr_id) {
	case SBI_SSE_ATTR_CONFIG:
		e->attrs.config = val;
		break;
	case SBI_SSE_ATTR_PRIO:
		e->attrs.prio = (uint32_t)val;
		break;
	case SBI_SSE_ATTR_PREFERRED_HART:
		e->attrs.hartid = val;
		e->hartindex = sbi_hartid_to_hartindex(val);
		sse_event_invoke_cb(e, set_hartid_cb, val);
		break;

	case SBI_SSE_ATTR_INTERRUPTED_SEPC:
		e->attrs.interrupted.sepc = val;
		break;
	case SBI_SSE_ATTR_INTERRUPTED_FLAGS:
		e->attrs.interrupted.flags = val;
		break;
	case SBI_SSE_ATTR_INTERRUPTED_A6:
		e->attrs.interrupted.a6 = val;
		break;
	case SBI_SSE_ATTR_INTERRUPTED_A7:
		e->attrs.interrupted.a7 = val;
		break;
	}
}

static int sse_event_register(struct sbi_sse_event *e,
			      unsigned long handler_entry_pc,
			      unsigned long handler_entry_arg)
{
	if (sse_event_state(e) != SBI_SSE_STATE_UNUSED)
		return SBI_EINVALID_STATE;

	e->attrs.entry.pc = handler_entry_pc;
	e->attrs.entry.arg = handler_entry_arg;

	sse_event_set_state(e, SBI_SSE_STATE_REGISTERED);

	sse_event_invoke_cb(e, register_cb);

	return 0;
}

static int sse_event_unregister(struct sbi_sse_event *e)
{
	if (sse_event_state(e) != SBI_SSE_STATE_REGISTERED)
		return SBI_EINVALID_STATE;

	sse_event_invoke_cb(e, unregister_cb);

	sse_event_set_state(e, SBI_SSE_STATE_UNUSED);

	return 0;
}

static unsigned long sse_interrupted_flags(unsigned long mstatus)
{
	unsigned long hstatus, flags = 0;

	if (mstatus & (MSTATUS_SPIE))
		flags |= SBI_SSE_ATTR_INTERRUPTED_FLAGS_STATUS_SPIE;
	if (mstatus & (MSTATUS_SPP))
		flags |= SBI_SSE_ATTR_INTERRUPTED_FLAGS_STATUS_SPP;

	if (misa_extension('H')) {
		hstatus = csr_read(CSR_HSTATUS);
		if (hstatus & HSTATUS_SPV)
			flags |= SBI_SSE_ATTR_INTERRUPTED_FLAGS_HSTATUS_SPV;
		if (hstatus & HSTATUS_SPVP)
			flags |= SBI_SSE_ATTR_INTERRUPTED_FLAGS_HSTATUS_SPVP;
	}

	return flags;
}

static void sse_event_inject(struct sbi_sse_event *e,
			     struct sbi_trap_regs *regs)
{
	struct sse_interrupted_state *i_ctx = &e->attrs.interrupted;

	sse_event_set_state(e, SBI_SSE_STATE_RUNNING);

	e->attrs.status = ~BIT(SBI_SSE_ATTR_STATUS_PENDING_OFFSET);

	i_ctx->a6 = regs->a6;
	i_ctx->a7 = regs->a7;
	i_ctx->flags = sse_interrupted_flags(regs->mstatus);
	i_ctx->sepc = csr_read(CSR_SEPC);

	regs->mstatus &= ~(MSTATUS_SPP | SSTATUS_SPIE);
	if (regs->mstatus & MSTATUS_MPP)
		regs->mstatus |= MSTATUS_SPP;
	if (regs->mstatus & MSTATUS_SIE)
		regs->mstatus |= MSTATUS_SPIE;

	if (misa_extension('H')) {
		unsigned long hstatus = csr_read(CSR_HSTATUS);

#if __riscv_xlen == 64
		if (regs->mstatus & MSTATUS_MPV)
#elif __riscv_xlen == 32
		if (regs->mstatusH & MSTATUSH_MPV)
#else
#error "Unexpected __riscv_xlen"
#endif
			hstatus |= HSTATUS_SPV;

		hstatus &= ~HSTATUS_SPVP;
		if (hstatus & HSTATUS_SPV && regs->mstatus & SSTATUS_SPP)
				hstatus |= HSTATUS_SPVP;

		csr_write(CSR_HSTATUS, hstatus);
	}
	csr_write(CSR_SEPC, regs->mepc);

	/* Setup entry context */
	regs->a6 = current_hartid();
	regs->a7 = e->attrs.entry.arg;
	regs->mepc = e->attrs.entry.pc;

	/* Return to S-mode with virtualization disabled */
	regs->mstatus &= ~(MSTATUS_MPP | MSTATUS_SIE);
	regs->mstatus |= (PRV_S << MSTATUS_MPP_SHIFT);

#if __riscv_xlen == 64
	regs->mstatus &= ~MSTATUS_MPV;
#elif __riscv_xlen == 32
	regs->mstatusH &= ~MSTATUSH_MPV;
#else
#error "Unexpected __riscv_xlen"
#endif

}

static void sse_event_resume(struct sbi_sse_event *e,
			     struct sbi_trap_regs *regs)
{
	struct sse_interrupted_state *i_ctx = &e->attrs.interrupted;

	regs->mepc = csr_read(CSR_SEPC);

	regs->mstatus &= ~MSTATUS_MPP;
	if (regs->mstatus & MSTATUS_SPP)
		regs->mstatus |= (PRV_S << MSTATUS_MPP_SHIFT);

	if (misa_extension('H')) {
		unsigned long hstatus = csr_read(CSR_HSTATUS);
#if __riscv_xlen == 64
		regs->mstatus &= ~MSTATUS_MPV;
		if (hstatus & HSTATUS_SPV)
			regs->mstatus |= MSTATUS_MPV;
#elif __riscv_xlen == 32
		regs->mstatusH &= ~MSTATUSH_MPV;
		if (hstatus & HSTATUS_SPV)
			regs->mstatusH |= MSTATUSH_MPV;
#else
#error "Unexpected __riscv_xlen"
#endif
		hstatus &= ~(HSTATUS_SPV | HSTATUS_SPVP);
		if (i_ctx->flags & SBI_SSE_ATTR_INTERRUPTED_FLAGS_HSTATUS_SPV)
			hstatus |= HSTATUS_SPV;

		if (i_ctx->flags & SBI_SSE_ATTR_INTERRUPTED_FLAGS_HSTATUS_SPVP)
			hstatus |= HSTATUS_SPVP;

		csr_write(CSR_HSTATUS, hstatus);
	}

	regs->mstatus &= ~MSTATUS_SIE;
	if (regs->mstatus & MSTATUS_SPIE)
		regs->mstatus |= MSTATUS_SIE;

	regs->mstatus &= ~MSTATUS_SPIE;
	if (i_ctx->flags & SBI_SSE_ATTR_INTERRUPTED_FLAGS_STATUS_SPIE)
		regs->mstatus |= MSTATUS_SPIE;

	regs->mstatus &= ~MSTATUS_SPP;
	if (i_ctx->flags & SBI_SSE_ATTR_INTERRUPTED_FLAGS_STATUS_SPP)
		regs->mstatus |= MSTATUS_SPP;

	regs->a7 = i_ctx->a7;
	regs->a6 = i_ctx->a6;
	csr_write(CSR_SEPC, i_ctx->sepc);
}

static bool sse_event_is_ready(struct sbi_sse_event *e)
{
	if (!sse_event_pending(e) ||
	    sse_event_state(e) != SBI_SSE_STATE_ENABLED ||
	    e->attrs.hartid != current_hartid()) {
		return false;
	}

	return true;
}

static bool sse_event_check_inject(struct sbi_sse_event *e,
				   struct sbi_trap_regs *regs)
{
	/*
	 * List of event is ordered by priority, stop at first running
	 * event since all other events after this one are of lower
	 * priority. This means an event of higher priority is already
	 * running.
	 */
	if (sse_event_state(e) == SBI_SSE_STATE_RUNNING) {
		return true;
	}

	if (sse_event_is_ready(e)) {
		sse_event_inject(e, regs);
		return true;
	}

	return false;
}

/* Return true if an event has been injected, false otherwise */
void sbi_sse_process_pending_events(struct sbi_trap_regs *regs)
{
	bool ret;
	struct sbi_sse_event *e;
	struct sse_hart_state *state = sse_thishart_state_ptr();

	/* if sse is masked on this hart, do nothing */
	if (state->masked)
		return;

	spin_lock(&state->enabled_event_lock);

	sbi_list_for_each_entry(e, &state->enabled_event_list, node) {
		ret = sse_event_check_inject(e, regs);
		if (ret)
			goto out;
	}

out:
	spin_unlock(&state->enabled_event_lock);
}

static int sse_event_set_pending(struct sbi_sse_event *e)
{
	if (sse_event_state(e) != SBI_SSE_STATE_RUNNING &&
	    sse_event_state(e) != SBI_SSE_STATE_ENABLED)
		return SBI_EINVALID_STATE;

	e->attrs.status |= BIT(SBI_SSE_ATTR_STATUS_PENDING_OFFSET);

	return SBI_OK;
}

static void sse_ipi_inject_process(struct sbi_scratch *scratch)
{
	struct sbi_sse_event *e;
	struct sse_ipi_inject_data evt;
	struct sbi_fifo *sse_inject_fifo_r =
		sbi_scratch_offset_ptr(scratch, sse_inject_fifo_off);

	/* Mark all queued events as pending */
	while (!sbi_fifo_dequeue(sse_inject_fifo_r, &evt)) {
		if (sse_event_get(evt.event_id, &e))
			continue;

		sse_event_set_pending(e);
		sse_event_put(e);
	}
}

static struct sbi_ipi_event_ops sse_ipi_inject_ops = {
	.name	 = "IPI_SSE_INJECT",
	.process = sse_ipi_inject_process,
};

static int sse_ipi_inject_send(unsigned long hartid, uint32_t event_id)
{
	int ret;
	struct sbi_scratch *remote_scratch = NULL;
	struct sse_ipi_inject_data evt = {event_id};
	struct sbi_fifo *sse_inject_fifo_r;

	remote_scratch = sbi_hartid_to_scratch(hartid);
	if (!remote_scratch)
		return SBI_EINVAL;
	sse_inject_fifo_r =
		sbi_scratch_offset_ptr(remote_scratch, sse_inject_fifo_off);

	ret = sbi_fifo_enqueue(sse_inject_fifo_r, &evt, false);
	if (ret)
		return SBI_EFAIL;

	ret = sbi_ipi_send_many(1, hartid, sse_ipi_inject_event, NULL);
	if (ret)
		return SBI_EFAIL;

	return SBI_OK;
}

static int sse_inject_event(uint32_t event_id, unsigned long hartid)
{
	int ret;
	struct sbi_sse_event *e;

	ret = sse_event_get(event_id, &e);
	if (ret)
		return ret;

	/* In case of global event, provided hart_id is ignored */
	if (sse_event_is_global(e))
		hartid = e->attrs.hartid;

	/* Event is for another hart, send it through IPI */
	if (hartid != current_hartid()) {
		sse_event_put(e);
		return sse_ipi_inject_send(hartid, event_id);
	}

	ret = sse_event_set_pending(e);
	sse_event_put(e);
	if (ret)
		return ret;

	return SBI_OK;
}

/**
 * Must be called under owner hart lock
 */
static int sse_event_enable(struct sbi_sse_event *e)
{
	if (sse_event_state(e) != SBI_SSE_STATE_REGISTERED)
		return SBI_EINVALID_STATE;

	sse_event_set_state(e, SBI_SSE_STATE_ENABLED);
	sse_event_add_to_list(e);

	sse_event_invoke_cb(e, enable_cb);

	if (sse_event_is_global(e) && sse_event_pending(e))
		sbi_ipi_send_many(1, e->attrs.hartid, sse_ipi_inject_event,
				  NULL);

	return SBI_OK;
}

static int sse_event_complete(struct sbi_sse_event *e,
			      struct sbi_trap_regs *regs,
			      struct sbi_ecall_return *out)
{
	if (sse_event_state(e) != SBI_SSE_STATE_RUNNING)
		return SBI_EINVALID_STATE;

	if (e->attrs.hartid != current_hartid())
		return SBI_EINVAL;

	sse_event_set_state(e, SBI_SSE_STATE_ENABLED);
	if (e->attrs.config & SBI_SSE_ATTR_CONFIG_ONESHOT)
		sse_event_disable(e);

	sse_event_invoke_cb(e, complete_cb);

	sse_event_resume(e, regs);
	out->skip_regs_update = true;

	return SBI_OK;
}

int sbi_sse_complete(struct sbi_trap_regs *regs, struct sbi_ecall_return *out)
{
	int ret = SBI_OK;
	struct sbi_sse_event *tmp;
	struct sse_hart_state *state = sse_thishart_state_ptr();

	spin_lock(&state->enabled_event_lock);
	sbi_list_for_each_entry(tmp, &state->enabled_event_list, node) {
		/*
		 * List of event is ordered by priority, first one running is
		 * the one that needs to be completed
		 */
		if (sse_event_state(tmp) == SBI_SSE_STATE_RUNNING) {
			ret = sse_event_complete(tmp, regs, out);
			break;
		}
	}
	spin_unlock(&state->enabled_event_lock);

	return ret;
}

int sbi_sse_enable(uint32_t event_id)
{
	int ret;
	struct sbi_sse_event *e;

	ret = sse_event_get(event_id, &e);
	if (ret)
		return ret;

	sse_enabled_event_lock(e);
	ret = sse_event_enable(e);
	sse_enabled_event_unlock(e);
	sse_event_put(e);

	return ret;
}

int sbi_sse_disable(uint32_t event_id)
{
	int ret;
	struct sbi_sse_event *e;

	ret = sse_event_get(event_id, &e);
	if (ret)
		return ret;

	sse_enabled_event_lock(e);
	ret = sse_event_disable(e);
	sse_enabled_event_unlock(e);

	sse_event_put(e);

	return ret;
}

int sbi_sse_hart_mask(void)
{
	struct sse_hart_state *state = sse_thishart_state_ptr();

	if (!state)
		return SBI_EFAIL;

	if (state->masked)
		return SBI_EALREADY_STARTED;

	state->masked = true;

	return SBI_SUCCESS;
}

int sbi_sse_hart_unmask(void)
{
	struct sse_hart_state *state = sse_thishart_state_ptr();

	if (!state)
		return SBI_EFAIL;

	if (!state->masked)
		return SBI_EALREADY_STOPPED;

	state->masked = false;

	return SBI_SUCCESS;
}

int sbi_sse_inject_from_ecall(uint32_t event_id, unsigned long hartid,
			      struct sbi_ecall_return *out)
{
	if (!sbi_domain_is_assigned_hart(sbi_domain_thishart_ptr(),
					 sbi_hartid_to_hartindex(hartid)))
		return SBI_EINVAL;

	return sse_inject_event(event_id, hartid);
}

int sbi_sse_inject_event(uint32_t event_id)
{
	return sse_inject_event(event_id, current_hartid());
}

int sbi_sse_set_cb_ops(uint32_t event_id, const struct sbi_sse_cb_ops *cb_ops)
{
	int ret;
	struct sbi_sse_event *e;

	if (cb_ops->set_hartid_cb && !EVENT_IS_GLOBAL(event_id))
		return SBI_EINVAL;

	ret = sse_event_get(event_id, &e);
	if (ret)
		return ret;

	e->cb_ops = cb_ops;
	sse_event_put(e);

	return SBI_OK;
}

int sbi_sse_attr_check(uint32_t base_attr_id, uint32_t attr_count,
		       unsigned long phys_lo, unsigned long phys_hi,
		       unsigned long access)
{
	const unsigned align = __riscv_xlen >> 3;
	uint64_t end_id = (uint64_t)base_attr_id + (attr_count - 1);

	if (attr_count == 0)
		return SBI_ERR_INVALID_PARAM;

	if (end_id >= SBI_SSE_ATTR_MAX)
		return SBI_EBAD_RANGE;

	if (phys_lo & (align - 1))
		return SBI_EINVALID_ADDR;

	/*
	 * On RV32, the M-mode can only access the first 4GB of
	 * the physical address space because M-mode does not have
	 * MMU to access full 34-bit physical address space.
	 *
	 * Based on above, we simply fail if the upper 32bits of
	 * the physical address (i.e. a2 register) is non-zero on
	 * RV32.
	 */
	if (phys_hi)
		return SBI_EINVALID_ADDR;

	if (!sbi_domain_check_addr_range(sbi_domain_thishart_ptr(), phys_lo,
					 sizeof(unsigned long) * attr_count,
					 PRV_S, access))
		return SBI_EINVALID_ADDR;

	return SBI_OK;
}

static void copy_attrs(unsigned long *out, const unsigned long *in,
		       unsigned int long_count)
{
	int i = 0;

	/*
	 * sbi_memcpy() does byte-per-byte copy, using this yields long-per-long
	 * copy
	 */
	for (i = 0; i < long_count; i++)
		out[i] = in[i];
}

int sbi_sse_read_attrs(uint32_t event_id, uint32_t base_attr_id,
		       uint32_t attr_count, unsigned long output_phys_lo,
		       unsigned long output_phys_hi)
{
	int ret;
	unsigned long *e_attrs;
	struct sbi_sse_event *e;
	unsigned long *attrs;

	ret = sbi_sse_attr_check(base_attr_id, attr_count, output_phys_lo,
				 output_phys_hi, SBI_DOMAIN_WRITE);
	if (ret)
		return ret;

	ret = sse_event_get(event_id, &e);
	if (ret)
		return ret;

	sbi_hart_map_saddr(output_phys_lo, sizeof(unsigned long) * attr_count);

	/*
	 * Copy all attributes at once since struct sse_event_attrs is matching
	 * the SBI_SSE_ATTR_* attributes. READ_ATTR is used in SSE handling path
	 * to retrieve the value of registers when interrupted. Rather than
	 * doing multiple SBI calls a single one is done allowing to retrieve
	 * them all at once.
	 */
	e_attrs = (unsigned long *)&e->attrs;
	attrs = (unsigned long *)output_phys_lo;
	copy_attrs(attrs, &e_attrs[base_attr_id], attr_count);

	sbi_hart_unmap_saddr();

	sse_event_put(e);

	return SBI_OK;
}

static int sse_write_attrs(struct sbi_sse_event *e, uint32_t base_attr_id,
			   uint32_t attr_count, unsigned long input_phys)
{
	int ret = 0;
	unsigned long attr = 0, val;
	uint32_t id, end_id = base_attr_id + attr_count;
	unsigned long *attrs = (unsigned long *)input_phys;

	sbi_hart_map_saddr(input_phys, sizeof(unsigned long) * attr_count);

	for (id = base_attr_id; id < end_id; id++) {
		val = attrs[attr++];
		ret = sse_event_set_attr_check(e, id, val);
		if (ret)
			goto out;
	}

	attr = 0;
	for (id = base_attr_id; id < end_id; id++) {
		val = attrs[attr++];
		sse_event_set_attr(e, id, val);
	}

out:
	sbi_hart_unmap_saddr();

	return ret;
}

int sbi_sse_write_attrs(uint32_t event_id, uint32_t base_attr_id,
			uint32_t attr_count, unsigned long input_phys_lo,
			unsigned long input_phys_hi)
{
	int ret = 0;
	struct sbi_sse_event *e;

	ret = sbi_sse_attr_check(base_attr_id, attr_count, input_phys_lo,
				 input_phys_hi, SBI_DOMAIN_READ);
	if (ret)
		return ret;

	ret = sse_event_get(event_id, &e);
	if (ret)
		return ret;

	ret = sse_write_attrs(e, base_attr_id, attr_count, input_phys_lo);
	sse_event_put(e);

	return ret;
}

int sbi_sse_register(uint32_t event_id, unsigned long handler_entry_pc,
		     unsigned long handler_entry_arg)
{
	int ret;
	struct sbi_sse_event *e;

	if (handler_entry_pc & 0x1)
		return SBI_EINVAL;

	if (!sbi_domain_check_addr_range(sbi_domain_thishart_ptr(),
					 handler_entry_pc,
					 sizeof(unsigned long), PRV_S,
					 SBI_DOMAIN_EXECUTE))
		return SBI_EINVALID_ADDR;

	ret = sse_event_get(event_id, &e);
	if (ret)
		return ret;

	ret = sse_event_register(e, handler_entry_pc, handler_entry_arg);
	sse_event_put(e);

	return ret;
}

int sbi_sse_unregister(uint32_t event_id)
{
	int ret;
	struct sbi_sse_event *e;

	ret = sse_event_get(event_id, &e);
	if (ret)
		return ret;

	ret = sse_event_unregister(e);
	sse_event_put(e);

	return ret;
}

static void sse_event_init(struct sbi_sse_event *e, uint32_t event_id)
{
	e->event_id = event_id;
	e->hartindex = current_hartindex();
	e->attrs.hartid = current_hartid();
	/* Declare all events as injectable */
	e->attrs.status |= BIT(SBI_SSE_ATTR_STATUS_INJECT_OFFSET);
}

static void sse_event_count_init()
{
	unsigned int i;

	for (i = 0; i < EVENT_COUNT; i++) {
		if (EVENT_IS_GLOBAL(supported_events[i]))
			global_event_count++;
		else
			local_event_count++;
	}
}

static int sse_global_init()
{
	struct sbi_sse_event *e;
	unsigned int i, ev = 0;

	global_events = sbi_zalloc(sizeof(*global_events) * global_event_count);
	if (!global_events)
		return SBI_ENOMEM;

	for (i = 0; i < EVENT_COUNT; i++) {
		if (!EVENT_IS_GLOBAL(supported_events[i]))
			continue;

		e = &global_events[ev].event;
		sse_event_init(e, supported_events[i]);
		SPIN_LOCK_INIT(global_events[ev].lock);

		ev++;
	}

	return 0;
}

static void sse_local_init(struct sse_hart_state *shs)
{
	unsigned int i, ev = 0;

	SBI_INIT_LIST_HEAD(&shs->enabled_event_list);
	SPIN_LOCK_INIT(shs->enabled_event_lock);

	for (i = 0; i < EVENT_COUNT; i++) {
		if (EVENT_IS_GLOBAL(supported_events[i]))
			continue;

		sse_event_init(&shs->local_events[ev++], supported_events[i]);
	}
}

int sbi_sse_init(struct sbi_scratch *scratch, bool cold_boot)
{
	int ret;
	void *sse_inject_mem;
	struct sse_hart_state *shs;
	struct sbi_fifo *sse_inject_q;

	if (cold_boot) {
		sse_event_count_init();

		ret = sse_global_init();
		if (ret)
			return ret;

		shs_ptr_off = sbi_scratch_alloc_offset(sizeof(void *));
		if (!shs_ptr_off)
			return SBI_ENOMEM;

		sse_inject_fifo_off =
			sbi_scratch_alloc_offset(sizeof(*sse_inject_q));
		if (!sse_inject_fifo_off) {
			sbi_scratch_free_offset(shs_ptr_off);
			return SBI_ENOMEM;
		}

		sse_inject_fifo_mem_off = sbi_scratch_alloc_offset(
			EVENT_COUNT * sizeof(struct sse_ipi_inject_data));
		if (!sse_inject_fifo_mem_off) {
			sbi_scratch_free_offset(sse_inject_fifo_off);
			sbi_scratch_free_offset(shs_ptr_off);
			return SBI_ENOMEM;
		}

		ret = sbi_ipi_event_create(&sse_ipi_inject_ops);
		if (ret < 0) {
			sbi_scratch_free_offset(shs_ptr_off);
			return ret;
		}
		sse_ipi_inject_event = ret;
	}

	shs = sse_get_hart_state_ptr(scratch);
	if (!shs) {
		/* Allocate per hart state and local events at once */
		shs = sbi_zalloc(sizeof(*shs) + sizeof(struct sbi_sse_event) *
							local_event_count);
		if (!shs)
			return SBI_ENOMEM;

		shs->local_events = (struct sbi_sse_event *)(shs + 1);

		/* SSE events are masked until hart unmasks them */
		shs->masked = true;

		sse_set_hart_state_ptr(scratch, shs);
	}

	sse_local_init(shs);

	sse_inject_q = sbi_scratch_offset_ptr(scratch, sse_inject_fifo_off);
	sse_inject_mem =
		sbi_scratch_offset_ptr(scratch, sse_inject_fifo_mem_off);

	sbi_fifo_init(sse_inject_q, sse_inject_mem, EVENT_COUNT,
		      sizeof(struct sse_ipi_inject_data));

	return 0;
}

void sbi_sse_exit(struct sbi_scratch *scratch)
{
	int i;
	struct sbi_sse_event *e;

	for (i = 0; i < EVENT_COUNT; i++) {
		if (sse_event_get(supported_events[i], &e))
			continue;

		if (e->attrs.hartid != current_hartid())
			goto skip;

		if (sse_event_state(e) > SBI_SSE_STATE_REGISTERED) {
			sbi_printf("Event %d in invalid state at exit", i);
			sse_event_set_state(e, SBI_SSE_STATE_UNUSED);
		}

skip:
		sse_event_put(e);
	}
}
