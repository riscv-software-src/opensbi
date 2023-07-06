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
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_fifo.h>
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

#define sse_get_hart_state_ptr(__scratch)	\
	sbi_scratch_read_type((__scratch), void *, shs_ptr_off)

#define sse_thishart_state_ptr()		\
	sse_get_hart_state_ptr(sbi_scratch_thishart_ptr())

#define sse_set_hart_state_ptr(__scratch, __sse_state)	\
	sbi_scratch_write_type((__scratch), void *, shs_ptr_off, (__sse_state))

/*
 * Rather than using memcpy to copy the context (which does it byte-per-byte),
 * copy each field which generates ld/lw.
 */
#define regs_copy(dst, src) \
	dst->ra = src->ra; \
	dst->sp = src->sp; \
	dst->gp = src->gp; \
	dst->tp = src->tp; \
	dst->t0 = src->t0; \
	dst->t1 = src->t1; \
	dst->t2 = src->t2; \
	dst->s0 = src->s0; \
	dst->s1 = src->s1; \
	dst->a0 = src->a0; \
	dst->a1 = src->a1; \
	dst->a2 = src->a2; \
	dst->a3 = src->a3; \
	dst->a4 = src->a4; \
	dst->a5 = src->a5; \
	dst->a6 = src->a6; \
	dst->a7 = src->a7; \
	dst->s2 = src->s2; \
	dst->s3 = src->s3; \
	dst->s4 = src->s4; \
	dst->s5 = src->s5; \
	dst->s6 = src->s6; \
	dst->s7 = src->s7; \
	dst->s8 = src->s8; \
	dst->s9 = src->s9; \
	dst->s10 = src->s10; \
	dst->s11 = src->s11; \
	dst->t3 = src->t3; \
	dst->t4 = src->t4; \
	dst->t5 = src->t5; \
	dst->t6 = src->t6

#define EVENT_IS_GLOBAL(__event_id)	((__event_id) & SBI_SSE_EVENT_GLOBAL)

#define EVENT_COUNT	array_size(supported_events)

static const uint32_t supported_events[] =
{
	SBI_SSE_EVENT_LOCAL_RAS,
	SBI_SSE_EVENT_LOCAL_PMU,
	SBI_SSE_EVENT_LOCAL_ASYNC_PF,
	SBI_SSE_EVENT_LOCAL_DEBUG,
	SBI_SSE_EVENT_GLOBAL_RAS,
	SBI_SSE_EVENT_GLOBAL_DEBUG,
};

struct sse_ipi_inject_data {
	uint32_t event_id;
};

struct sbi_sse_event {
	enum sbi_sse_state state;
	bool pending;
	uint32_t event_id;
	struct sbi_sse_handler_ctx *ctx;
	uint32_t prio;
	unsigned int hartid;
	const struct sbi_sse_cb_ops *cb_ops;
	struct sbi_dlist node;
	spinlock_t lock;
};

struct sse_hart_state {
	struct sbi_dlist event_list;
	spinlock_t list_lock;
	struct sbi_sse_event *local_events;
};

static unsigned int local_event_count;
static unsigned int global_event_count;
static struct sbi_sse_event *global_events;

static unsigned long sse_inject_fifo_off;
static unsigned long sse_inject_fifo_mem_off;
/* Offset of pointer to SSE HART state in scratch space */
static unsigned long shs_ptr_off;

static u32 sse_ipi_inject_event = SBI_IPI_EVENT_MAX;

static int sse_ipi_inject_send(unsigned long hartid, uint32_t event_id);

static bool sse_event_is_global(struct sbi_sse_event *e)
{
	return EVENT_IS_GLOBAL(e->event_id);
}

static void sse_event_lock(struct sbi_sse_event *e)
{
	if (sse_event_is_global(e))
		spin_lock(&e->lock);
}

static void sse_event_unlock(struct sbi_sse_event *e)
{
	if (sse_event_is_global(e))
		spin_unlock(&e->lock);
}

static void sse_event_set_state(struct sbi_sse_event *e,
				enum sbi_sse_state new_state)
{
	enum sbi_sse_state prev_state = e->state;

	e->state = new_state;
	switch (new_state) {
		case SSE_STATE_UNUSED:
			if (prev_state == SSE_STATE_REGISTERED)
				return;
		break;
		case SSE_STATE_REGISTERED:
			if (prev_state == SSE_STATE_UNUSED ||
			    prev_state == SSE_STATE_ENABLED) {
				return;
			}
		break;
		case SSE_STATE_ENABLED:
			if (prev_state == SSE_STATE_REGISTERED ||
			    prev_state == SSE_STATE_RUNNING)
				return;
		break;
		case SSE_STATE_RUNNING:
			if (prev_state == SSE_STATE_ENABLED)
				return;
		break;
	}

	sbi_panic("Invalid SSE state transition: %d -> %d\n", prev_state,
		  new_state);
}

static struct sbi_sse_event *sse_event_get(uint32_t event)
{
	unsigned int i;
	struct sbi_sse_event *events, *e;
	unsigned int count;
	struct sse_hart_state *shs;

	if (EVENT_IS_GLOBAL(event)) {
		count = global_event_count;
		events = global_events;
	} else {
		count = local_event_count;
		shs = sse_thishart_state_ptr();
		events = shs->local_events;
	}

	for (i = 0; i < count; i++) {
		e = &events[i];
		if (e->event_id == event)
			return e;
	}

	return NULL;
}

static int sse_event_get_attr(struct sbi_sse_event *e, uint32_t attr_id,
			      unsigned long *out_val)
{
	int ret;

	switch (attr_id) {
	case SBI_SSE_ATTR_STATE:
		*out_val = e->state;
		ret = 0;
		break;
	case SBI_SSE_ATTR_PRIO:
		*out_val = e->prio;
		ret = 0;
		break;
	case SBI_SSE_ATTR_ALLOW_INJECT:
		*out_val = 1;
		ret = 0;
		break;
	case SBI_SSE_ATTR_HART_ID:
		*out_val = e->hartid;
		ret = 0;
		break;
	case SBI_SSE_ATTR_PENDING:
		*out_val = e->pending;
		ret = 0;
		break;
	default:
		ret = SBI_EINVAL;
		break;
	}

	return ret;
}

static void sse_event_remove_from_list(struct sbi_sse_event *e)
{
	struct sbi_scratch *scratch = sbi_hartid_to_scratch(e->hartid);
	struct sse_hart_state *state = sse_get_hart_state_ptr(scratch);

	spin_lock(&state->list_lock);
	sbi_list_del(&e->node);
	spin_unlock(&state->list_lock);
}

static void sse_event_add_to_list(struct sbi_sse_event *e)
{
	struct sbi_scratch *scratch = sbi_hartid_to_scratch(e->hartid);
	struct sse_hart_state *state = sse_get_hart_state_ptr(scratch);
	struct sbi_sse_event *tmp;

	spin_lock(&state->list_lock);
	sbi_list_for_each_entry(tmp, &state->event_list, node) {
		if (e->prio < tmp->prio)
			break;
		if (e->prio == tmp->prio && e->event_id < tmp->event_id)
			break;
	}
	sbi_list_add_tail(&e->node, &tmp->node);

	spin_unlock(&state->list_lock);
}

static int sse_event_disable(struct sbi_sse_event *e)
{
	if (e->state != SSE_STATE_ENABLED)
		return SBI_EINVALID_STATE;

	sse_event_remove_from_list(e);
	sse_event_set_state(e, SSE_STATE_REGISTERED);

	return SBI_OK;
}

static int sse_event_set_hart_id(struct sbi_sse_event *e, uint32_t event_id,
				 unsigned long new_hartid)
{
	int hstate;
	unsigned int hartid = (uint32_t) new_hartid;
	struct sbi_domain * hd = sbi_domain_thishart_ptr();

	if (!sse_event_is_global(e))
		return SBI_EDENIED;

	if (e->state == SSE_STATE_RUNNING)
		return SBI_EBUSY;

	if (!sbi_domain_is_assigned_hart(hd, new_hartid))
		return SBI_EINVAL;

	hstate = sbi_hsm_hart_get_state(hd, hartid);
	if (hstate != SBI_HSM_STATE_STARTED)
		return SBI_EINVAL;

	if (new_hartid == e->hartid)
		return SBI_OK;

	if (e->state >= SSE_STATE_ENABLED)
		sse_event_remove_from_list(e);

	e->hartid = hartid;

	if (e->cb_ops && e->cb_ops->set_hartid_cb)
		 e->cb_ops->set_hartid_cb(event_id, e->hartid);

	if (e->state >= SSE_STATE_ENABLED)
		sse_event_add_to_list(e);

	if (e->pending)
		sbi_ipi_send_many(BIT(e->hartid), 0, sse_ipi_inject_event, NULL);

	return 0;
}

static int sse_event_set_attr(struct sbi_sse_event *e, uint32_t event_id,
			      uint32_t attr_id, unsigned long val)
{
	int ret;

	switch (attr_id) {
	case SBI_SSE_ATTR_PENDING:
	case SBI_SSE_ATTR_STATE:
	case SBI_SSE_ATTR_ALLOW_INJECT:
		/* Read-only */
		ret = SBI_EDENIED;
		break;
	case SBI_SSE_ATTR_PRIO:
		if (e->state >= SSE_STATE_ENABLED) {
			ret = SBI_EINVALID_STATE;
		} else {
			e->prio = (uint32_t)val;
			ret = 0;
		}
		break;
	case SBI_SSE_ATTR_HART_ID:
		ret = sse_event_set_hart_id(e, event_id, val);
		break;
	default:
		ret = SBI_EINVAL;
		break;
	}

	return ret;
}

static int sse_event_register(struct sbi_sse_event *e,
			      struct sbi_sse_handler_ctx *ctx)
{
	if (sse_event_is_global(e) && e->hartid != current_hartid())
		return SBI_EINVAL;

	if (e->state != SSE_STATE_UNUSED)
		return SBI_EINVALID_STATE;

	e->ctx = ctx;
	sse_event_set_state(e, SSE_STATE_REGISTERED);

	return 0;
}

static int sse_event_unregister(struct sbi_sse_event *e)
{
	if (e->state != SSE_STATE_REGISTERED)
		return SBI_EINVALID_STATE;

	sse_event_set_state(e, SSE_STATE_UNUSED);
	e->ctx = NULL;

	return 0;
}

static void sse_event_inject(struct sbi_sse_event *e,
			     struct sbi_sse_event *prev_e,
			     struct sbi_trap_regs *regs)
{
	ulong prev_smode, prev_virt;
	struct sse_interrupted_state *i_ctx = &e->ctx->interrupted, *prev_i_ctx;
	struct sse_entry_state *e_ctx = &e->ctx->entry;

	sse_event_set_state(e, SSE_STATE_RUNNING);
	e->pending = false;

	if (prev_e) {
		/* We are injected right after another event, copy previous
		 * event context for correct restoration
		 */
		prev_i_ctx = &prev_e->ctx->interrupted;

		regs_copy(i_ctx, prev_i_ctx);
		i_ctx->exc_mode = prev_i_ctx->exc_mode;
		i_ctx->pc = prev_i_ctx->pc;
	} else {
		regs_copy(i_ctx, regs);

		prev_smode = (regs->mstatus & MSTATUS_MPP) >> MSTATUS_MPP_SHIFT;
	#if __riscv_xlen == 32
		prev_virt = (regs->mstatusH & MSTATUSH_MPV) ? 1 : 0;
	#else
		prev_virt = (regs->mstatus & MSTATUS_MPV) ? 1 : 0;
	#endif

		i_ctx->exc_mode = prev_smode << EXC_MODE_PP_SHIFT;
		i_ctx->exc_mode |= prev_virt << EXC_MODE_PV_SHIFT;
		if (regs->mstatus & MSTATUS_SPIE)
			i_ctx->exc_mode |= EXC_MODE_SSTATUS_SPIE;
		i_ctx->pc = regs->mepc;

		/* We only want to set SPIE for the first event injected after
		 * entering M-Mode. For the event injected right after another
		 * event (after calling sse_event_complete(), we will keep the
		 * saved SPIE).
		 */
		regs->mstatus &= ~MSTATUS_SPIE;
		if (regs->mstatus & MSTATUS_SIE)
			regs->mstatus |= MSTATUS_SPIE;
	}

	regs_copy(regs, e_ctx);
	regs->mepc = e_ctx->pc;

	regs->mstatus &= ~MSTATUS_MPP;
	regs->mstatus |= (PRV_S << MSTATUS_MPP_SHIFT);

	#if __riscv_xlen == 32
		regs->mstatusH &= ~MSTATUSH_MPV;
	#else
		regs->mstatus &= ~MSTATUS_MPV;
	#endif

	regs->mstatus &= ~MSTATUS_SIE;
}

static int sse_event_resume(struct sbi_sse_event *e, struct sbi_trap_regs *regs)
{
	struct sse_interrupted_state *i_ctx = &e->ctx->interrupted;

	regs_copy(regs, i_ctx);

	/* Restore previous virtualization state */
#if __riscv_xlen == 32
	regs->mstatusH &= ~MSTATUSH_MPV;
	if (i_ctx->exc_mode & EXC_MODE_PV)
		regs->mstatusH |= MSTATUSH_MPV;
#else
	regs->mstatus &= ~MSTATUS_MPV;
	if (i_ctx->exc_mode & EXC_MODE_PV)
		regs->mstatus |= MSTATUS_MPV;
#endif

	regs->mstatus &= ~MSTATUS_MPP;
	if (i_ctx->exc_mode & EXC_MODE_PP)
		regs->mstatus |= (PRV_S << MSTATUS_MPP_SHIFT);

	regs->mstatus &= ~MSTATUS_SIE;
	if (regs->mstatus & MSTATUS_SPIE)
		regs->mstatus |= MSTATUS_SIE;

	regs->mstatus &= ~MSTATUS_SPIE;
	if (i_ctx->exc_mode & EXC_MODE_SSTATUS_SPIE)
		regs->mstatus |= MSTATUS_SPIE;

	regs->mepc = i_ctx->pc;

	return SBI_EJUMP;
}

static bool event_is_ready(struct sbi_sse_event *e)
{
	if (!e->pending || e->state == SSE_STATE_RUNNING ||
	    e->hartid != current_hartid()) {
		return false;
	}

	return true;
}

static int sse_process_pending_events(struct sbi_sse_event *prev_e,
				      struct sbi_trap_regs *regs)
{
	int ret = SBI_OK;
	struct sbi_sse_event *e, *to_run = NULL;
	struct sse_hart_state *state = sse_thishart_state_ptr();

retry:
	spin_lock(&state->list_lock);
	sbi_list_for_each_entry(e, &state->event_list, node) {
		/*
		 * List of event is ordered by priority, stop at first running
		 * event since all other events after this one are of lower
		 * priority.
		 */
		if (e->state == SSE_STATE_RUNNING)
			break;

		if (event_is_ready(e)) {
			to_run = e;
			break;
		}
	}
	spin_unlock(&state->list_lock);

	/*
	 * We can not lock the event while holding the list lock or we could
	 * deadlock due to different locking order than other functions.
	 */
	if (to_run) {
		sse_event_lock(to_run);
		/*
		 * Reevaluate readiness, for global events, the hart_id might
		 * have changed, in that case, try to find another ready event
		 */
		if (!event_is_ready(e)) {
			sse_event_unlock(to_run);
			to_run = NULL;
			goto retry;
		}

		sse_event_inject(to_run, prev_e, regs);
		sse_event_unlock(to_run);

		return SBI_EJUMP;
	}

	return ret;
}

static int sse_event_set_pending(struct sbi_sse_event *e)
{
	if (e->state != SSE_STATE_RUNNING && e->state != SSE_STATE_ENABLED)
		return SBI_ERR_INVALID_STATE;

	e->pending = true;

	return SBI_OK;
}

static void sse_ipi_inject_process(struct sbi_scratch *scratch,
				   struct sbi_trap_regs *regs)
{
	struct sbi_sse_event *e;
	struct sse_ipi_inject_data evt;
	struct sbi_fifo *sse_inject_fifo_r =
			sbi_scratch_offset_ptr(scratch, sse_inject_fifo_off);

	/* This can be the case when sbi_exit() is called */
	if (!regs)
		return;

	/* Mark all queued events as pending */
	while(!sbi_fifo_dequeue(sse_inject_fifo_r, &evt)) {
		e = sse_event_get(evt.event_id);
		if (!e)
			continue;

		sse_event_lock(e);
		sse_event_set_pending(e);
		sse_event_unlock(e);
	}

	sse_process_pending_events(NULL, regs);
}

static struct sbi_ipi_event_ops sse_ipi_inject_ops = {
	.name = "IPI_SSE_INJECT",
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
	sse_inject_fifo_r = sbi_scratch_offset_ptr(remote_scratch,
						   sse_inject_fifo_off);

	ret = sbi_fifo_enqueue(sse_inject_fifo_r, &evt);
	if (ret)
		return SBI_EFAIL;

	ret = sbi_ipi_send_many(BIT(hartid), 0, sse_ipi_inject_event, NULL);
	if (ret)
		return SBI_EFAIL;

	return SBI_OK;
}

static int sse_inject_event(uint32_t event_id, unsigned long hartid,
			    struct sbi_trap_regs *regs, bool from_ecall)
{
	int ret;
	struct sbi_sse_event *e;

	e = sse_event_get(event_id);
	if (!e)
		return SBI_EINVAL;

	sse_event_lock(e);

	/* In case of global event, provided hart_id is ignored */
	if (sse_event_is_global(e))
		hartid = e->hartid;

	/*
	 * If coming from an ecall, always use an IPI to send the event, this
	 * simplifies handling as we don't have to modify epc/a0 for ecall
	 * return value.
	 */
	if (from_ecall || hartid != current_hartid()) {
		sse_event_unlock(e);
		return sse_ipi_inject_send(hartid, event_id);
	}

	/*
	 * In other cases, directly handle the event on this hart for faster
	 * handling
	 */
	ret = sse_event_set_pending(e);
	sse_event_unlock(e);
	if (ret)
		return ret;

	return sse_process_pending_events(NULL, regs);
}

static int sse_event_enable(struct sbi_sse_event *e)
{
	if (e->state != SSE_STATE_REGISTERED)
		return SBI_EINVALID_STATE;

	sse_event_set_state(e, SSE_STATE_ENABLED);
	sse_event_add_to_list(e);

	return SBI_OK;
}

static int sse_event_complete(struct sbi_sse_event *e, uint32_t status,
			      uint32_t flags, struct sbi_trap_regs *regs)
{
	if (e->state != SSE_STATE_RUNNING)
		return SBI_EINVALID_STATE;

	if (e->hartid != current_hartid())
		return SBI_EDENIED;

	if (flags & SBI_SSE_COMPLETE_FLAG_EVENT_DISABLE)
		sse_event_disable(e);
	else
		sse_event_set_state(e, SSE_STATE_ENABLED);

	if (e->cb_ops && e->cb_ops->complete_cb)
		e->cb_ops->complete_cb(e->event_id);

	if (sse_process_pending_events(e, regs) == SBI_EJUMP)
		return SBI_EJUMP;

	return sse_event_resume(e, regs);
}

int sbi_sse_complete(uint32_t event_id, uint32_t status, uint32_t flags,
		     struct sbi_trap_regs *regs)
{
	int ret;
	struct sbi_sse_event *e;

	e = sse_event_get(event_id);
	if (!e)
		return SBI_EINVAL;

	sse_event_lock(e);
	ret = sse_event_complete(e, status, flags, regs);
	sse_event_unlock(e);

	return ret;
}

int sbi_sse_enable(uint32_t event_id)
{
	int ret;
	struct sbi_sse_event *e;

	e = sse_event_get(event_id);
	if (!e)
		return SBI_EINVAL;

	sse_event_lock(e);
	ret = sse_event_enable(e);
	sse_event_unlock(e);

	return ret;
}

int sbi_sse_disable(uint32_t event_id)
{
	int ret;
	struct sbi_sse_event *e;

	e = sse_event_get(event_id);
	if (!e)
		return SBI_EINVAL;

	sse_event_lock(e);
	ret = sse_event_disable(e);
	sse_event_unlock(e);

	return ret;
}

int sbi_sse_inject_from_ecall(uint32_t event_id, unsigned long hartid,
			      struct sbi_trap_regs *regs)
{
	if (!sbi_domain_is_assigned_hart(sbi_domain_thishart_ptr(), hartid))
		return SBI_EINVAL;

	return sse_inject_event(event_id, hartid, regs, true);
}

int sbi_sse_inject_event(uint32_t event_id, struct sbi_trap_regs *regs)
{
	return sse_inject_event(event_id, current_hartid(), regs, false);
}

int sbi_sse_set_cb_ops(uint32_t event_id, const struct sbi_sse_cb_ops *cb_ops)
{
	struct sbi_sse_event *e;

	e = sse_event_get(event_id);
	if (!e)
		return SBI_EINVAL;

	if (cb_ops->set_hartid_cb && !sse_event_is_global(e))
		return SBI_EINVAL;

	sse_event_lock(e);
	e->cb_ops = cb_ops;
	sse_event_unlock(e);

	return SBI_OK;
}


int sbi_sse_get_attr(uint32_t event_id, uint32_t attr_id, unsigned long *out_val)
{
	struct sbi_sse_event *e;

	e = sse_event_get(event_id);
	if (!e)
		return SBI_EINVAL;

	return sse_event_get_attr(e, attr_id, out_val);
}

int sbi_sse_set_attr(uint32_t event_id, uint32_t attr_id, unsigned long val)
{
	int ret;
	struct sbi_sse_event *e;

	e = sse_event_get(event_id);
	if (!e)
		return SBI_EINVAL;

	sse_event_lock(e);
	ret = sse_event_set_attr(e, event_id, attr_id, val);
	sse_event_unlock(e);

	return ret;
}

int sbi_sse_register(uint32_t event_id,
			unsigned long phys_lo,
			unsigned long phys_hi)
{
	int ret;
	struct sbi_sse_event *e;
	const unsigned align = __riscv_xlen >> 3;
	ulong smode = (csr_read(CSR_MSTATUS) & MSTATUS_MPP) >>
			MSTATUS_MPP_SHIFT;

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
					 sizeof(struct sbi_sse_handler_ctx),
					 smode,
					 SBI_DOMAIN_READ|SBI_DOMAIN_WRITE))
		return SBI_EINVALID_ADDR;

	e = sse_event_get(event_id);
	if (!e)
		return SBI_EINVAL;

	sse_event_lock(e);
	ret = sse_event_register(e, (struct sbi_sse_handler_ctx *)phys_lo);
	sse_event_unlock(e);

	if (!ret) {
		if (e->cb_ops && e->cb_ops->register_cb) {
			e->cb_ops->register_cb(e->event_id);
		}
	}

	return ret;
}

int sbi_sse_unregister(uint32_t event_id)
{
	int ret;
	struct sbi_sse_event *e;

	e = sse_event_get(event_id);
	if (!e)
		return SBI_EINVAL;

	sse_event_lock(e);
	ret = sse_event_unregister(e);
	sse_event_unlock(e);

	if (!ret) {
		if (e->cb_ops && e->cb_ops->unregister_cb) {
			e->cb_ops->unregister_cb(e->event_id);
		}
	}

	return ret;
}

static void sse_event_init(struct sbi_sse_event *event, uint32_t event_id)
{
	event->event_id = event_id;
	event->hartid = current_hartid();
	SPIN_LOCK_INIT(event->lock);
}

static int sse_global_init()
{
	unsigned int i, ev = 0;

	for (i = 0; i < EVENT_COUNT; i++) {
		if (EVENT_IS_GLOBAL(supported_events[i]))
			global_event_count++;
		else
			local_event_count++;
	}

	global_events = sbi_zalloc(sizeof(*global_events) * global_event_count);
	if (!global_events)
		return SBI_ENOMEM;

	for (i = 0; i < EVENT_COUNT; i++) {
		if (!EVENT_IS_GLOBAL(supported_events[i]))
			continue;

		sse_event_init(&global_events[ev++], supported_events[i]);
	}

	return 0;
}

static void sse_local_init(struct sse_hart_state *shs)
{
	unsigned int i, ev = 0;

	SBI_INIT_LIST_HEAD(&shs->event_list);
	SPIN_LOCK_INIT(shs->list_lock);

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
		ret = sse_global_init();
		if (ret)
			return ret;

		shs_ptr_off = sbi_scratch_alloc_offset(sizeof(void *));
		if (!shs_ptr_off)
			return SBI_ENOMEM;

		sse_inject_fifo_off = sbi_scratch_alloc_offset(
							sizeof(*sse_inject_q));
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
		shs = sbi_zalloc(sizeof(*shs) +
			sizeof(struct sbi_sse_event) * local_event_count);
		if (!shs)
			return SBI_ENOMEM;

		shs->local_events = (struct sbi_sse_event *)(shs + 1);

		sse_set_hart_state_ptr(scratch, shs);
	}

	sse_local_init(shs);

	sse_inject_q = sbi_scratch_offset_ptr(scratch, sse_inject_fifo_off);
	sse_inject_mem = sbi_scratch_offset_ptr(scratch,
						sse_inject_fifo_mem_off);

	sbi_fifo_init(sse_inject_q, sse_inject_mem, EVENT_COUNT,
		      sizeof(struct sse_ipi_inject_data));

	return 0;
}

void sbi_sse_exit(struct sbi_scratch *scratch)
{
	int i;
	struct sbi_sse_event *e;

	for (i = 0; i < EVENT_COUNT; i++) {
		e = sse_event_get(supported_events[i]);

		if (e->hartid != current_hartid())
			continue;

		if (e->state > SSE_STATE_REGISTERED)
			sbi_printf("Event %d in invalid state at exit", i);
	}
}
