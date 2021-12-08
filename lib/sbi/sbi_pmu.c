/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2021 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Atish Patra <atish.patra@wdc.com>
 */

#include <sbi/riscv_asm.h>
#include <sbi/sbi_bitops.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_pmu.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_string.h>

/** Information about hardware counters */
struct sbi_pmu_hw_event {
	uint32_t counters;
	uint32_t start_idx;
	uint32_t end_idx;
	/* Event selector value used only for raw events. The event select value
	 * can be a even id or a selector value for set of events encoded in few
	 * bits. In case latter, the bits used for encoding of the events should
	 * be zeroed out in the select value.
	 */
	uint64_t select;
	 /**
	  * The select_mask indicates which bits are encoded for the event(s).
	  */
	uint64_t select_mask;
};

/** Representation of a firmware event */
struct sbi_pmu_fw_event {
	/* Event associated with the particular counter */
	uint32_t event_idx;

	/* Current value of the counter */
	unsigned long curr_count;

	/* A flag indicating pmu event monitoring is started */
	bool bStarted;
};

/* Information about PMU counters as per SBI specification */
union sbi_pmu_ctr_info {
	unsigned long value;
	struct {
		unsigned long csr:12;
		unsigned long width:6;
#if __riscv_xlen == 32
		unsigned long reserved:13;
#else
		unsigned long reserved:45;
#endif
		unsigned long type:1;
	};
};

/* Mapping between event range and possible counters  */
static struct sbi_pmu_hw_event hw_event_map[SBI_PMU_HW_EVENT_MAX] = {0};

/* counter to enabled event mapping */
static uint32_t active_events[SBI_HARTMASK_MAX_BITS][SBI_PMU_HW_CTR_MAX + SBI_PMU_FW_CTR_MAX];

/* Contains all the information about firmwares events */
static struct sbi_pmu_fw_event fw_event_map[SBI_HARTMASK_MAX_BITS][SBI_PMU_FW_EVENT_MAX] = {0};

/* Maximum number of hardware events available */
static uint32_t num_hw_events;
/* Maximum number of hardware counters available */
static uint32_t num_hw_ctrs;

/* Maximum number of counters available */
static uint32_t total_ctrs;

/* Helper macros to retrieve event idx and code type */
#define get_cidx_type(x) ((x & SBI_PMU_EVENT_IDX_TYPE_MASK) >> 16)
#define get_cidx_code(x) (x & SBI_PMU_EVENT_IDX_CODE_MASK)

/**
 * Perform a sanity check on event & counter mappings with event range overlap check
 * @param evtA Pointer to the existing hw event structure
 * @param evtB Pointer to the new hw event structure
 *
 * Return FALSE if the range doesn't overlap, TRUE otherwise
 */
static bool pmu_event_range_overlap(struct sbi_pmu_hw_event *evtA,
				    struct sbi_pmu_hw_event *evtB)
{
	/* check if the range of events overlap with a previous entry */
	if (((evtA->end_idx < evtB->start_idx) && (evtA->end_idx < evtB->end_idx)) ||
	   ((evtA->start_idx > evtB->start_idx) && (evtA->start_idx > evtB->end_idx)))
		return FALSE;
	return TRUE;
}

static bool pmu_event_select_overlap(struct sbi_pmu_hw_event *evt,
				     uint64_t select_val, uint64_t select_mask)
{
	if ((evt->select == select_val) && (evt->select_mask == select_mask))
		return TRUE;

	return FALSE;
}

static int pmu_ctr_validate(uint32_t cidx, uint32_t *event_idx_code)
{
	uint32_t event_idx_val;
	uint32_t event_idx_type;
	u32 hartid = current_hartid();

	event_idx_val = active_events[hartid][cidx];

	if (cidx >= total_ctrs || (event_idx_val == SBI_PMU_EVENT_IDX_INVALID))
		return SBI_EINVAL;

	event_idx_type = get_cidx_type(event_idx_val);
	if (event_idx_type >= SBI_PMU_EVENT_TYPE_MAX)
		return SBI_EINVAL;

	*event_idx_code = get_cidx_code(event_idx_val);

	return event_idx_type;
}

static int pmu_ctr_read_fw(uint32_t cidx, unsigned long *cval,
			       uint32_t fw_evt_code)
{
	u32 hartid = current_hartid();
	struct sbi_pmu_fw_event fevent;

	fevent = fw_event_map[hartid][fw_evt_code];
	*cval = fevent.curr_count;

	return 0;
}

/* Add a hardware counter read for completeness for future purpose */
static int pmu_ctr_read_hw(uint32_t cidx, uint64_t *cval)
{
	/* Check for invalid hw counter read requests */
	if (unlikely(cidx == 1))
		return SBI_EINVAL;
#if __riscv_xlen == 32
	uint32_t temp, temph = 0;

	temp = csr_read_num(CSR_MCYCLE + cidx);
	temph = csr_read_num(CSR_MCYCLEH + cidx);
	*cval = ((uint64_t)temph << 32) | temp;
#else
	*cval = csr_read_num(CSR_MCYCLE + cidx);
#endif

	return 0;
}

int sbi_pmu_ctr_read(uint32_t cidx, unsigned long *cval)
{
	int event_idx_type;
	uint32_t event_code;
	uint64_t cval64;

	event_idx_type = pmu_ctr_validate(cidx, &event_code);
	if (event_idx_type < 0)
		return SBI_EINVAL;
	else if (event_idx_type == SBI_PMU_EVENT_TYPE_FW)
		pmu_ctr_read_fw(cidx, cval, event_code);
	else
		pmu_ctr_read_hw(cidx, &cval64);

	return 0;
}

static int pmu_add_hw_event_map(u32 eidx_start, u32 eidx_end, u32 cmap,
				uint64_t select, uint64_t select_mask)
{
	int i = 0;
	bool is_overlap;
	struct sbi_pmu_hw_event *event = &hw_event_map[num_hw_events];

	/* The first two counters are reserved by priv spec */
	if (eidx_start > SBI_PMU_HW_INSTRUCTIONS && (cmap & SBI_PMU_FIXED_CTR_MASK))
		return SBI_EDENIED;

	if (num_hw_events >= SBI_PMU_HW_EVENT_MAX - 1) {
		sbi_printf("Can not handle more than %d perf events\n",
			    SBI_PMU_HW_EVENT_MAX);
		return SBI_EFAIL;
	}

	event->start_idx = eidx_start;
	event->end_idx = eidx_end;

	/* Sanity check */
	for (i = 0; i < num_hw_events; i++) {
		if (eidx_start == SBI_PMU_EVENT_RAW_IDX)
		/* All raw events have same event idx. Just do sanity check on select */
			is_overlap = pmu_event_select_overlap(&hw_event_map[i],
							      select, select_mask);
		else
			is_overlap = pmu_event_range_overlap(&hw_event_map[i], event);
		if (is_overlap)
			goto reset_event;
	}

	event->select_mask = select_mask;
	event->counters = cmap;
	event->select = select;
	num_hw_events++;

	return 0;

reset_event:
	event->start_idx = 0;
	event->end_idx = 0;
	return SBI_EINVAL;
}

/**
 * Logical counter ids are assigned to hardware counters are assigned consecutively.
 * E.g. counter0 must count MCYCLE where counter2 must count minstret. Similarly,
 * counterX will mhpmcounterX.
 */
int sbi_pmu_add_hw_event_counter_map(u32 eidx_start, u32 eidx_end, u32 cmap)
{
	if ((eidx_start > eidx_end) || eidx_start == SBI_PMU_EVENT_RAW_IDX ||
	     eidx_end == SBI_PMU_EVENT_RAW_IDX)
		return SBI_EINVAL;

	return pmu_add_hw_event_map(eidx_start, eidx_end, cmap, 0, 0);
}

int sbi_pmu_add_raw_event_counter_map(uint64_t select, uint64_t select_mask, u32 cmap)
{
	return pmu_add_hw_event_map(SBI_PMU_EVENT_RAW_IDX,
				    SBI_PMU_EVENT_RAW_IDX, cmap, select, select_mask);
}

static int pmu_ctr_enable_irq_hw(int ctr_idx)
{
	unsigned long mhpmevent_csr;
	unsigned long mhpmevent_curr;
	unsigned long mip_val;
	unsigned long of_mask;

	if (ctr_idx < 3 || ctr_idx >= SBI_PMU_HW_CTR_MAX)
		return SBI_EFAIL;

#if __riscv_xlen == 32
	mhpmevent_csr = CSR_MHPMEVENT3H  + ctr_idx - 3;
	of_mask = ~MHPMEVENTH_OF;
#else
	mhpmevent_csr = CSR_MHPMEVENT3 + ctr_idx - 3;
	of_mask = ~MHPMEVENT_OF;
#endif

	mhpmevent_curr = csr_read_num(mhpmevent_csr);
	mip_val = csr_read(CSR_MIP);
	/**
	 * Clear out the OF bit so that next interrupt can be enabled.
	 * This should be done only when the corresponding overflow interrupt
	 * bit is cleared. That indicates that software has already handled the
	 * previous interrupts or the hardware yet to set an overflow interrupt.
	 * Otherwise, there will be race conditions where we may clear the bit
	 * the software is yet to handle the interrupt.
	 */
	if (!(mip_val & MIP_LCOFIP)) {
		mhpmevent_curr &= of_mask;
		csr_write_num(mhpmevent_csr, mhpmevent_curr);
	}

	return 0;
}

static void pmu_ctr_write_hw(uint32_t cidx, uint64_t ival)
{
#if __riscv_xlen == 32
	csr_write_num(CSR_MCYCLE + cidx, 0);
	csr_write_num(CSR_MCYCLE + cidx, ival & 0xFFFFFFFF);
	csr_write_num(CSR_MCYCLEH + cidx, ival >> BITS_PER_LONG);
#else
	csr_write_num(CSR_MCYCLE + cidx, ival);
#endif
}

static int pmu_ctr_start_hw(uint32_t cidx, uint64_t ival, bool ival_update)
{
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();
	unsigned long mctr_inhbt;

	/* Make sure the counter index lies within the range and is not TM bit */
	if (cidx > num_hw_ctrs || cidx == 1)
		return SBI_EINVAL;

	if (!sbi_hart_has_feature(scratch, SBI_HART_HAS_MCOUNTINHIBIT))
		goto skip_inhibit_update;

	/*
	 * Some of the hardware may not support mcountinhibit but perf stat
	 * still can work if supervisor mode programs the initial value.
	 */
	mctr_inhbt = csr_read(CSR_MCOUNTINHIBIT);
	if (!__test_bit(cidx, &mctr_inhbt))
		return SBI_EALREADY_STARTED;

	__clear_bit(cidx, &mctr_inhbt);

	if (sbi_hart_has_feature(scratch, SBI_HART_HAS_SSCOFPMF))
		pmu_ctr_enable_irq_hw(cidx);
	csr_write(CSR_MCOUNTINHIBIT, mctr_inhbt);

skip_inhibit_update:
	if (ival_update)
		pmu_ctr_write_hw(cidx, ival);

	return 0;
}

static int pmu_ctr_start_fw(uint32_t cidx, uint32_t fw_evt_code,
			    uint64_t ival, bool ival_update)
{
	u32 hartid = current_hartid();
	struct sbi_pmu_fw_event *fevent;

	fevent = &fw_event_map[hartid][fw_evt_code];
	if (ival_update)
		fevent->curr_count = ival;
	fevent->bStarted = TRUE;

	return 0;
}

int sbi_pmu_ctr_start(unsigned long cbase, unsigned long cmask,
		      unsigned long flags, uint64_t ival)
{
	int event_idx_type;
	uint32_t event_code;
	unsigned long ctr_mask = cmask << cbase;
	int ret = SBI_EINVAL;
	bool bUpdate = FALSE;

	if (__fls(ctr_mask) >= total_ctrs)
		return ret;

	if (flags & SBI_PMU_START_FLAG_SET_INIT_VALUE)
		bUpdate = TRUE;

	for_each_set_bit_from(cbase, &ctr_mask, total_ctrs) {
		event_idx_type = pmu_ctr_validate(cbase, &event_code);
		if (event_idx_type < 0)
			/* Continue the start operation for other counters */
			continue;
		else if (event_idx_type == SBI_PMU_EVENT_TYPE_FW)
			ret = pmu_ctr_start_fw(cbase, event_code, ival, bUpdate);
		else
			ret = pmu_ctr_start_hw(cbase, ival, bUpdate);
	}

	return ret;
}

static int pmu_ctr_stop_hw(uint32_t cidx)
{
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();
	unsigned long mctr_inhbt;

	if (!sbi_hart_has_feature(scratch, SBI_HART_HAS_MCOUNTINHIBIT))
		return 0;

	mctr_inhbt = csr_read(CSR_MCOUNTINHIBIT);

	/* Make sure the counter index lies within the range and is not TM bit */
	if (cidx > num_hw_ctrs || cidx == 1)
		return SBI_EINVAL;

	if (!__test_bit(cidx, &mctr_inhbt)) {
		__set_bit(cidx, &mctr_inhbt);
		csr_write(CSR_MCOUNTINHIBIT, mctr_inhbt);
		return 0;
	} else
		return SBI_EALREADY_STOPPED;
}

static int pmu_ctr_stop_fw(uint32_t cidx, uint32_t fw_evt_code)
{
	u32 hartid = current_hartid();

	fw_event_map[hartid][fw_evt_code].bStarted = FALSE;

	return 0;
}

static int pmu_reset_hw_mhpmevent(int ctr_idx)
{
	if (ctr_idx < 3 || ctr_idx >= SBI_PMU_HW_CTR_MAX)
		return SBI_EFAIL;
#if __riscv_xlen == 32
	csr_write_num(CSR_MHPMEVENT3 + ctr_idx - 3, 0);
	csr_write_num(CSR_MHPMEVENT3H + ctr_idx - 3, 0);
#else
	csr_write_num(CSR_MHPMEVENT3 + ctr_idx - 3, 0);
#endif

	return 0;
}

int sbi_pmu_ctr_stop(unsigned long cbase, unsigned long cmask,
		     unsigned long flag)
{
	u32 hartid = current_hartid();
	int ret = SBI_EINVAL;
	int event_idx_type;
	uint32_t event_code;
	unsigned long ctr_mask = cmask << cbase;

	if (__fls(ctr_mask) >= total_ctrs)
		return SBI_EINVAL;

	for_each_set_bit_from(cbase, &ctr_mask, total_ctrs) {
		event_idx_type = pmu_ctr_validate(cbase, &event_code);
		if (event_idx_type < 0)
			/* Continue the stop operation for other counters */
			continue;

		else if (event_idx_type == SBI_PMU_EVENT_TYPE_FW)
			ret = pmu_ctr_stop_fw(cbase, event_code);
		else
			ret = pmu_ctr_stop_hw(cbase);

		if (flag & SBI_PMU_STOP_FLAG_RESET) {
			active_events[hartid][cbase] = SBI_PMU_EVENT_IDX_INVALID;
			pmu_reset_hw_mhpmevent(cbase);
		}
	}

	return ret;
}

static void pmu_update_inhibit_flags(unsigned long flags, uint64_t *mhpmevent_val)
{
	if (flags & SBI_PMU_CFG_FLAG_SET_VUINH)
		*mhpmevent_val |= MHPMEVENT_VUINH;
	if (flags & SBI_PMU_CFG_FLAG_SET_VSINH)
		*mhpmevent_val |= MHPMEVENT_VSINH;
	if (flags & SBI_PMU_CFG_FLAG_SET_UINH)
		*mhpmevent_val |= MHPMEVENT_UINH;
	if (flags & SBI_PMU_CFG_FLAG_SET_SINH)
		*mhpmevent_val |= MHPMEVENT_SINH;
}

static int pmu_update_hw_mhpmevent(struct sbi_pmu_hw_event *hw_evt, int ctr_idx,
				   unsigned long flags, unsigned long eindex,
				   uint64_t data)
{
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();
	const struct sbi_platform *plat = sbi_platform_ptr(scratch);
	uint64_t mhpmevent_val;

	/* Get the final mhpmevent value to be written from platform */
	mhpmevent_val = sbi_platform_pmu_xlate_to_mhpmevent(plat, eindex, data);

	if (!mhpmevent_val || ctr_idx < 3 || ctr_idx >= SBI_PMU_HW_CTR_MAX)
		return SBI_EFAIL;

	/* Always clear the OVF bit and inhibit countin of events in M-mode */
	if (sbi_hart_has_feature(scratch, SBI_HART_HAS_SSCOFPMF))
		mhpmevent_val = (mhpmevent_val & ~MHPMEVENT_SSCOF_MASK) | MHPMEVENT_MINH;

	/* Update the inhibit flags based on inhibit flags received from supervisor */
	pmu_update_inhibit_flags(flags, &mhpmevent_val);

#if __riscv_xlen == 32
	csr_write_num(CSR_MHPMEVENT3 + ctr_idx - 3, mhpmevent_val & 0xFFFFFFFF);
	csr_write_num(CSR_MHPMEVENT3H + ctr_idx - 3, mhpmevent_val >> BITS_PER_LONG);
#else
	csr_write_num(CSR_MHPMEVENT3 + ctr_idx - 3, mhpmevent_val);
#endif

	return 0;
}

static int pmu_ctr_find_fixed_fw(unsigned long evt_idx_code)
{
	/* Non-programmables counters are enabled always. No need to do lookup */
	if (evt_idx_code == SBI_PMU_HW_CPU_CYCLES)
		return 0;
	else if (evt_idx_code == SBI_PMU_HW_INSTRUCTIONS)
		return 2;
	else
		return SBI_EINVAL;
}

static int pmu_ctr_find_hw(unsigned long cbase, unsigned long cmask, unsigned long flags,
			   unsigned long event_idx, uint64_t data)
{
	unsigned long ctr_mask;
	int i, ret = 0, fixed_ctr, ctr_idx = SBI_ENOTSUPP;
	struct sbi_pmu_hw_event *temp;
	unsigned long mctr_inhbt = 0;
	u32 hartid = current_hartid();
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();

	if (cbase > num_hw_ctrs)
		return SBI_EINVAL;

	/**
	 * If Sscof is present try to find the programmable counter for
	 * cycle/instret as well.
	 */
	fixed_ctr = pmu_ctr_find_fixed_fw(event_idx);
	if (fixed_ctr >= 0 &&
	    !sbi_hart_has_feature(scratch, SBI_HART_HAS_SSCOFPMF))
		return fixed_ctr;

	if (sbi_hart_has_feature(scratch, SBI_HART_HAS_MCOUNTINHIBIT))
		mctr_inhbt = csr_read(CSR_MCOUNTINHIBIT);
	for (i = 0; i < num_hw_events; i++) {
		temp = &hw_event_map[i];
		if ((temp->start_idx > event_idx && event_idx < temp->end_idx) ||
		    (temp->start_idx < event_idx && event_idx > temp->end_idx))
			continue;

		/* For raw events, event data is used as the select value */
		if (event_idx == SBI_PMU_EVENT_RAW_IDX) {
			uint64_t select_mask = temp->select_mask;

			/* The non-event map bits of data should match the selector */
			if (temp->select != (data & select_mask))
				continue;
		}
		/* Fixed counters should not be part of the search */
		ctr_mask = temp->counters & (cmask << cbase) &
			   (~SBI_PMU_FIXED_CTR_MASK);
		for_each_set_bit_from(cbase, &ctr_mask, SBI_PMU_HW_CTR_MAX) {
			/**
			 * Some of the platform may not support mcountinhibit.
			 * Checking the active_events is enough for them
			 */
			if (active_events[hartid][cbase] != SBI_PMU_EVENT_IDX_INVALID)
				continue;
			/* If mcountinhibit is supported, the bit must be enabled */
			if ((sbi_hart_has_feature(scratch, SBI_HART_HAS_MCOUNTINHIBIT)) &&
			    !__test_bit(cbase, &mctr_inhbt))
				continue;
			/* We found a valid counter that is not started yet */
			ctr_idx = cbase;
		}
	}

	if (ctr_idx == SBI_ENOTSUPP) {
		/**
		 * We can't find any programmable counters for cycle/instret.
		 * Return the fixed counter as they are mandatory anyways.
		 */
		if (fixed_ctr >= 0)
			return fixed_ctr;
		else
			return SBI_EFAIL;
	}
	ret = pmu_update_hw_mhpmevent(temp, ctr_idx, flags, event_idx, data);

	if (!ret)
		ret = ctr_idx;

	return ret;
}


/**
 * Any firmware counter can map to any firmware event.
 * Thus, select the first available fw counter after sanity
 * check.
 */
static int pmu_ctr_find_fw(unsigned long cbase, unsigned long cmask, u32 hartid)
{
	int i = 0;
	int fw_base;
	unsigned long ctr_mask = cmask << cbase;

	if (cbase <= num_hw_ctrs)
		fw_base = num_hw_ctrs + 1;
	else
		fw_base = cbase;

	for (i = fw_base; i < total_ctrs; i++)
		if ((active_events[hartid][i] == SBI_PMU_EVENT_IDX_INVALID) &&
		    ((1UL << i) & ctr_mask))
			return i;

	return SBI_ENOTSUPP;
}

int sbi_pmu_ctr_cfg_match(unsigned long cidx_base, unsigned long cidx_mask,
			  unsigned long flags, unsigned long event_idx,
			  uint64_t event_data)
{
	int ctr_idx = SBI_ENOTSUPP;
	u32 hartid = current_hartid();
	int event_type = get_cidx_type(event_idx);
	struct sbi_pmu_fw_event *fevent;
	uint32_t fw_evt_code;
	unsigned long tmp = cidx_mask << cidx_base;

	/* Do a basic sanity check of counter base & mask */
	if (__fls(tmp) >= total_ctrs || event_type >= SBI_PMU_EVENT_TYPE_MAX)
		return SBI_EINVAL;

	if (flags & SBI_PMU_CFG_FLAG_SKIP_MATCH) {
		/* The caller wants to skip the match because it already knows the
		 * counter idx for the given event. Verify that the counter idx
		 * is still valid.
		 */
		if (active_events[hartid][cidx_base] == SBI_PMU_EVENT_IDX_INVALID)
			return SBI_EINVAL;
		ctr_idx = cidx_base;
		goto skip_match;
	}

	if (event_type == SBI_PMU_EVENT_TYPE_FW) {
		/* Any firmware counter can be used track any firmware event */
		ctr_idx = pmu_ctr_find_fw(cidx_base, cidx_mask, hartid);
	} else {
		ctr_idx = pmu_ctr_find_hw(cidx_base, cidx_mask, flags, event_idx,
					  event_data);
	}

	if (ctr_idx < 0)
		return SBI_ENOTSUPP;

	active_events[hartid][ctr_idx] = event_idx;
skip_match:
	if (event_type == SBI_PMU_EVENT_TYPE_HW) {
		if (flags & SBI_PMU_CFG_FLAG_CLEAR_VALUE)
			pmu_ctr_write_hw(ctr_idx, 0);
		if (flags & SBI_PMU_CFG_FLAG_AUTO_START)
			pmu_ctr_start_hw(ctr_idx, 0, false);
	} else if (event_type == SBI_PMU_EVENT_TYPE_FW) {
		fw_evt_code = get_cidx_code(event_idx);
		fevent = &fw_event_map[hartid][fw_evt_code];
		if (flags & SBI_PMU_CFG_FLAG_CLEAR_VALUE)
			fevent->curr_count = 0;
		if (flags & SBI_PMU_CFG_FLAG_AUTO_START)
			fevent->bStarted = TRUE;
	}

	return ctr_idx;
}

inline int sbi_pmu_ctr_incr_fw(enum sbi_pmu_fw_event_code_id fw_id)
{
	u32 hartid = current_hartid();
	struct sbi_pmu_fw_event *fevent;

	if (unlikely(fw_id >= SBI_PMU_FW_MAX))
		return SBI_EINVAL;

	fevent = &fw_event_map[hartid][fw_id];

	/* PMU counters will be only enabled during performance debugging */
	if (unlikely(fevent->bStarted))
		fevent->curr_count++;

	return 0;
}

unsigned long sbi_pmu_num_ctr(void)
{
	return (num_hw_ctrs + SBI_PMU_FW_CTR_MAX);
}

int sbi_pmu_ctr_get_info(uint32_t cidx, unsigned long *ctr_info)
{
	union sbi_pmu_ctr_info cinfo = {0};
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();

	/* Sanity check. Counter1 is not mapped at all */
	if (cidx >= total_ctrs || cidx == 1)
		return SBI_EINVAL;

	/* We have 31 HW counters with 31 being the last index(MHPMCOUNTER31) */
	if (cidx <= num_hw_ctrs) {
		cinfo.type = SBI_PMU_CTR_TYPE_HW;
		cinfo.csr = CSR_CYCLE + cidx;
		/* mcycle & minstret are always 64 bit */
		if (cidx == 0 || cidx == 2)
			cinfo.width = 63;
		else
			cinfo.width = sbi_hart_mhpm_bits(scratch) - 1;
	} else {
		/* it's a firmware counter */
		cinfo.type = SBI_PMU_CTR_TYPE_FW;
		/* Firmware counters are XLEN bits wide */
		cinfo.width = BITS_PER_LONG - 1;
	}

	*ctr_info = cinfo.value;

	return 0;
}

static void pmu_reset_event_map(u32 hartid)
{
	int j;

	/* Initialize the counter to event mapping table */
	for (j = 3; j < total_ctrs; j++)
		active_events[hartid][j] = SBI_PMU_EVENT_IDX_INVALID;
	for (j = 0; j < SBI_PMU_FW_CTR_MAX; j++)
		sbi_memset(&fw_event_map[hartid][j], 0,
			   sizeof(struct sbi_pmu_fw_event));
}

void sbi_pmu_exit(struct sbi_scratch *scratch)
{
	u32 hartid = current_hartid();

	if (sbi_hart_has_feature(scratch, SBI_HART_HAS_MCOUNTINHIBIT))
		csr_write(CSR_MCOUNTINHIBIT, 0xFFFFFFF8);

	csr_write(CSR_MCOUNTEREN, -1);
	pmu_reset_event_map(hartid);
}

int sbi_pmu_init(struct sbi_scratch *scratch, bool cold_boot)
{
	const struct sbi_platform *plat;
	u32 hartid = current_hartid();

	if (cold_boot) {
		plat = sbi_platform_ptr(scratch);
		/* Initialize hw pmu events */
		sbi_platform_pmu_init(plat);

		/* mcycle & minstret is available always */
		num_hw_ctrs = sbi_hart_mhpm_count(scratch) + 2;
		total_ctrs = num_hw_ctrs + SBI_PMU_FW_CTR_MAX;
	}

	pmu_reset_event_map(hartid);

	/* First three counters are fixed by the priv spec and we enable it by default */
	active_events[hartid][0] = SBI_PMU_EVENT_TYPE_HW << SBI_PMU_EVENT_IDX_OFFSET |
				   SBI_PMU_HW_CPU_CYCLES;
	active_events[hartid][1] = SBI_PMU_EVENT_IDX_INVALID;
	active_events[hartid][2] = SBI_PMU_EVENT_TYPE_HW << SBI_PMU_EVENT_IDX_OFFSET |
				   SBI_PMU_HW_INSTRUCTIONS;

	return 0;
}
