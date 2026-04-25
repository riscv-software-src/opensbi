/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#ifndef __SBI_TIMER_H__
#define __SBI_TIMER_H__

#include <sbi/sbi_list.h>

/** Timer event re-start details */
struct sbi_timer_event_restart {
	/** Flag indicating whether event re-start is required */
	bool required;
	/** Next time stamp for event if re-start is required */
	u64 next_event;
};

/** Timer event abstraction */
struct sbi_timer_event {
	/** List head for per-HART event list (Internal) */
	struct sbi_dlist head;

	/** Hart on which the event is started / running (Internal) */
	int hart_index;

	/** Time stamp when the event expires (Internal) */
	u64 time_stamp;

	/**
	 * Event callback to be called upon expiry.
	 *
	 * If the callback wants to re-start the event then
	 * it must update the event re-start details.
	 *
	 * NOTE: This will be called with the per-HART timer
	 * event list lock held.
	 */
	void (*callback)(struct sbi_timer_event *ev,
			 struct sbi_timer_event_restart *restart);

	/**
	 * Event cleanup to be called upon sbi_timer_exit()
	 *
	 * NOTE: This will be called with per-HART timer
	 * event list lock held.
	 */
	void (*cleanup)(struct sbi_timer_event *ev);

	/** Event specific private data */
	void *priv;
};

#define SBI_INIT_TIMER_EVENT(__ptr, __callback, __cleanup, __priv)	\
do {									\
	SBI_INIT_LIST_HEAD(&(__ptr)->head);				\
	(__ptr)->hart_index = -1;					\
	(__ptr)->time_stamp = 0;					\
	(__ptr)->callback = (__callback); 				\
	(__ptr)->cleanup = (__cleanup); 				\
	(__ptr)->priv = (__priv); 					\
} while (0)

/** Timer hardware device */
struct sbi_timer_device {
	/** Name of the timer operations */
	char name[32];

	/** Frequency of timer in HZ */
	unsigned long timer_freq;

	/** Get free-running timer value */
	u64 (*timer_value)(void);

	/** Start timer event for current HART */
	void (*timer_event_start)(u64 next_event);

	/** Stop timer event for current HART */
	void (*timer_event_stop)(void);

	/** Initialize timer device for current HART */
	int (*warm_init)(void);
};

struct sbi_scratch;

/** Compute timer value delta based on arbitary units */
u64 sbi_timer_compute_delta(ulong units, u64 unit_freq);

/** Compute timer value delta from milliseconds */
static inline u64 sbi_timer_compute_mdelta(ulong msecs)
{
	return sbi_timer_compute_delta(msecs, 1000);
}

/** Compute timer value delta from microseconds */
static inline u64 sbi_timer_compute_udelta(ulong usecs)
{
	return sbi_timer_compute_delta(usecs, 1000000);
}

/** Generic delay loop of desired granularity */
void sbi_timer_delay_loop(ulong units, u64 unit_freq,
			  void (*delay_fn)(void *), void *opaque);

/** Provide delay in terms of milliseconds */
static inline void sbi_timer_mdelay(ulong msecs)
{
	sbi_timer_delay_loop(msecs, 1000, NULL, NULL);
}

/** Provide delay in terms of microseconds */
static inline void sbi_timer_udelay(ulong usecs)
{
	sbi_timer_delay_loop(usecs, 1000000, NULL, NULL);
}

/**
 * A blocking function that will wait until @p predicate returns true or
 * @p timeout_ms milliseconds elapsed. @p arg will be passed as argument to
 * @p predicate function.
 *
 * @param predicate Pointer to a function that returns true if certain
 * condition is met. It shouldn't block the code execution.
 * @param arg Argument to pass to @p predicate.
 * @param timeout_ms Timeout value in milliseconds. The function will return
 * false if @p timeout_ms time period elapsed but still @p predicate doesn't
 * return true.
 *
 * @return true if @p predicate returns true within @p timeout_ms, false
 * otherwise.
 */
bool sbi_timer_waitms_until(bool (*predicate)(void *), void *arg,
			    uint64_t timeout_ms);

/** Get timer value for current HART */
u64 sbi_timer_value(void);

/** Compute timer value after specified milliseconds */
static inline u64 sbi_timer_value_after_msecs(ulong msecs)
{
	return sbi_timer_value() + sbi_timer_compute_mdelta(msecs);
}

/** Compute timer value after specified microseconds */
static inline u64 sbi_timer_value_after_usecs(ulong usecs)
{
	return sbi_timer_value() + sbi_timer_compute_udelta(usecs);
}

/** Get virtualized timer value for current HART */
u64 sbi_timer_virt_value(void);

/** Get timer delta value for current HART */
u64 sbi_timer_get_delta(void);

/** Set timer delta value for current HART */
void sbi_timer_set_delta(ulong delta);

#if __riscv_xlen == 32
/** Set upper 32-bits of timer delta value for current HART */
void sbi_timer_set_delta_upper(ulong delta_upper);
#endif

/** Start timer event on current HART */
void sbi_timer_event_start(struct sbi_timer_event *ev, u64 next_event);

/** Stop timer event on current HART */
void sbi_timer_event_stop(struct sbi_timer_event *ev);

/** Start supervisor timer event on current HART */
void sbi_timer_smode_event_start(u64 next_event);

/** Process timer event for current HART */
void sbi_timer_process(void);

/** Get current timer device */
const struct sbi_timer_device *sbi_timer_get_device(void);

/** Register timer device */
void sbi_timer_set_device(const struct sbi_timer_device *dev);

/* Initialize timer */
int sbi_timer_init(struct sbi_scratch *scratch, bool cold_boot);

/* Exit timer */
void sbi_timer_exit(struct sbi_scratch *scratch);

#endif
