/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Atish Patra <atish.patra@wdc.com>
 */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_barrier.h>
#include <sbi/riscv_encoding.h>
#include <sbi/riscv_atomic.h>
#include <sbi/sbi_bitops.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_hartmask.h>
#include <sbi/sbi_hsm.h>
#include <sbi/sbi_init.h>
#include <sbi/sbi_ipi.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_system.h>
#include <sbi/sbi_timer.h>
#include <sbi/sbi_console.h>

static unsigned long hart_data_offset;

/** Per hart specific data to manage state transition **/
struct sbi_hsm_data {
	atomic_t state;
};

int sbi_hsm_hart_state_to_status(int state)
{
	int ret;

	switch (state) {
	case SBI_HART_STOPPED:
		ret = SBI_HSM_HART_STATUS_STOPPED;
		break;
	case SBI_HART_STOPPING:
		ret = SBI_HSM_HART_STATUS_STOP_PENDING;
		break;
	case SBI_HART_STARTING:
		ret = SBI_HSM_HART_STATUS_START_PENDING;
		break;
	case SBI_HART_STARTED:
		ret = SBI_HSM_HART_STATUS_STARTED;
		break;
	default:
		ret = SBI_EINVAL;
	}

	return ret;
}

int sbi_hsm_hart_get_state(u32 hartid)
{
	struct sbi_hsm_data *hdata;
	struct sbi_scratch *scratch;

	scratch = sbi_hartid_to_scratch(hartid);
	if (!scratch)
		return SBI_HART_UNKNOWN;

	hdata = sbi_scratch_offset_ptr(scratch, hart_data_offset);

	return atomic_read(&hdata->state);
}

bool sbi_hsm_hart_started(u32 hartid)
{
	if (sbi_hsm_hart_get_state(hartid) == SBI_HART_STARTED)
		return TRUE;
	else
		return FALSE;
}

/**
 * Get ulong HART mask for given HART base ID
 * @param hbase the HART base ID
 * @param out_hmask the output ulong HART mask
 * @return 0 on success and SBI_Exxx (< 0) on failure
 * Note: the output HART mask will be set to zero on failure as well.
 */
int sbi_hsm_hart_started_mask(ulong hbase, ulong *out_hmask)
{
	ulong i;
	ulong hcount = sbi_scratch_last_hartid() + 1;

	*out_hmask = 0;
	if (hcount <= hbase)
		return SBI_EINVAL;
	if (BITS_PER_LONG < (hcount - hbase))
		hcount = BITS_PER_LONG;

	for (i = hbase; i < hcount; i++) {
		if (sbi_hsm_hart_get_state(i) == SBI_HART_STARTED)
			*out_hmask |= 1UL << (i - hbase);
	}

	return 0;
}

void sbi_hsm_prepare_next_jump(struct sbi_scratch *scratch, u32 hartid)
{
	u32 oldstate;
	struct sbi_hsm_data *hdata = sbi_scratch_offset_ptr(scratch,
							    hart_data_offset);

	oldstate = atomic_cmpxchg(&hdata->state, SBI_HART_STARTING,
				  SBI_HART_STARTED);
	if (oldstate != SBI_HART_STARTING)
		sbi_hart_hang();
}

static void sbi_hsm_hart_wait(struct sbi_scratch *scratch, u32 hartid)
{
	unsigned long saved_mie;
	const struct sbi_platform *plat = sbi_platform_ptr(scratch);
	struct sbi_hsm_data *hdata = sbi_scratch_offset_ptr(scratch,
							    hart_data_offset);
	/* Save MIE CSR */
	saved_mie = csr_read(CSR_MIE);

	/* Set MSIE bit to receive IPI */
	csr_set(CSR_MIE, MIP_MSIP);

	/* Wait for hart_add call*/
	while (atomic_read(&hdata->state) != SBI_HART_STARTING) {
		wfi();
	};

	/* Restore MIE CSR */
	csr_write(CSR_MIE, saved_mie);

	/* Clear current HART IPI */
	sbi_platform_ipi_clear(plat, hartid);
}

int sbi_hsm_init(struct sbi_scratch *scratch, u32 hartid, bool cold_boot)
{
	u32 i;
	struct sbi_scratch *rscratch;
	struct sbi_hsm_data *hdata;

	if (cold_boot) {
		hart_data_offset = sbi_scratch_alloc_offset(sizeof(*hdata),
							    "HART_DATA");
		if (!hart_data_offset)
			return SBI_ENOMEM;

		/* Initialize hart state data for every hart */
		for (i = 0; i <= sbi_scratch_last_hartid(); i++) {
			rscratch = sbi_hartid_to_scratch(i);
			if (!rscratch)
				continue;

			hdata = sbi_scratch_offset_ptr(rscratch,
						       hart_data_offset);
			ATOMIC_INIT(&hdata->state,
			(i == hartid) ? SBI_HART_STARTING : SBI_HART_STOPPED);
		}
	} else {
		sbi_hsm_hart_wait(scratch, hartid);
	}

	return 0;
}

void __noreturn sbi_hsm_exit(struct sbi_scratch *scratch)
{
	u32 hstate;
	const struct sbi_platform *plat = sbi_platform_ptr(scratch);
	struct sbi_hsm_data *hdata = sbi_scratch_offset_ptr(scratch,
							    hart_data_offset);
	void (*jump_warmboot)(void) = (void (*)(void))scratch->warmboot_addr;

	hstate = atomic_cmpxchg(&hdata->state, SBI_HART_STOPPING,
				SBI_HART_STOPPED);
	if (hstate != SBI_HART_STOPPING)
		goto fail_exit;

	if (sbi_platform_has_hart_hotplug(plat)) {
		sbi_platform_hart_stop(plat);
		/* It should never reach here */
		goto fail_exit;
	}

	/**
	 * As platform is lacking support for hotplug, directly jump to warmboot
	 * and wait for interrupts in warmboot. We do it preemptively in order
	 * preserve the hart states and reuse the code path for hotplug.
	 */
	jump_warmboot();

fail_exit:
	/* It should never reach here */
	sbi_printf("ERR: Failed stop hart [%u]\n", current_hartid());
	sbi_hart_hang();
}

int sbi_hsm_hart_start(struct sbi_scratch *scratch, u32 hartid,
		       ulong saddr, ulong priv)
{
	int rc;
	unsigned long init_count;
	unsigned int hstate;
	struct sbi_scratch *rscratch;
	struct sbi_hsm_data *hdata;
	const struct sbi_platform *plat = sbi_platform_ptr(scratch);

	rscratch = sbi_hartid_to_scratch(hartid);
	if (!rscratch)
		return SBI_EINVAL;
	hdata = sbi_scratch_offset_ptr(rscratch, hart_data_offset);
	hstate = atomic_cmpxchg(&hdata->state, SBI_HART_STOPPED,
				SBI_HART_STARTING);
	if (hstate == SBI_HART_STARTED)
		return SBI_EALREADY;

	/**
	 * if a hart is already transition to start or stop, another start call
	 * is considered as invalid request.
	 */
	if (hstate != SBI_HART_STOPPED)
		return SBI_EINVAL;

	rc = sbi_hart_pmp_check_addr(scratch, saddr, PMP_X);
	if (rc)
		return rc;
	//TODO: We also need to check saddr for valid physical address as well.

	init_count = sbi_init_count(hartid);
	rscratch->next_arg1 = priv;
	rscratch->next_addr = saddr;

	if (sbi_platform_has_hart_hotplug(plat) ||
	   (sbi_platform_has_hart_secondary_boot(plat) && !init_count)) {
		return sbi_platform_hart_start(plat, hartid,
					       scratch->warmboot_addr);
	} else {
		sbi_platform_ipi_send(plat, hartid);
	}

	return 0;
}

int sbi_hsm_hart_stop(struct sbi_scratch *scratch, bool exitnow)
{
	int oldstate;
	u32 hartid = current_hartid();
	struct sbi_hsm_data *hdata = sbi_scratch_offset_ptr(scratch,
							    hart_data_offset);

	if (!sbi_hsm_hart_started(hartid))
		return SBI_EINVAL;

	oldstate = atomic_cmpxchg(&hdata->state, SBI_HART_STARTED,
				  SBI_HART_STOPPING);
	if (oldstate != SBI_HART_STARTED) {
		sbi_printf("%s: ERR: The hart is in invalid state [%u]\n",
			   __func__, oldstate);
		return SBI_EDENIED;
	}

	if (exitnow)
		sbi_exit(scratch);

	return 0;
}
