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

int sbi_hsm_hart_get_state(struct sbi_scratch *scratch, u32 hartid)
{
	struct sbi_hsm_data *hdata;
	u32 hstate;

	if (hartid != sbi_current_hartid())
		scratch = sbi_hart_id_to_scratch(scratch, hartid);

	hdata = sbi_scratch_offset_ptr(scratch, hart_data_offset);
	hstate = atomic_read(&hdata->state);

	return hstate;
}

bool sbi_hsm_hart_started(struct sbi_scratch *scratch, u32 hartid)
{
	if (sbi_hsm_hart_get_state(scratch, hartid) == SBI_HART_STARTED)
		return TRUE;
	else
		return FALSE;
}

void sbi_hsm_prepare_next_jump(struct sbi_scratch *scratch, u32 hartid)
{
	u32 oldstate;
	struct sbi_hsm_data *hdata = sbi_scratch_offset_ptr(scratch,
							hart_data_offset);

	oldstate = arch_atomic_cmpxchg(&hdata->state, SBI_HART_STARTING,
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
	struct sbi_scratch *rscratch;
	struct sbi_hsm_data *hdata;
	u32 hart_count, i;
	const struct sbi_platform *plat = sbi_platform_ptr(scratch);

	if (cold_boot) {
		hart_data_offset = sbi_scratch_alloc_offset(sizeof(*hdata),
								    "HART_DATA");
		if (!hart_data_offset)
			return SBI_ENOMEM;
		hart_count = sbi_platform_hart_count(plat);

		/* Initialize hart state data for every hart */
		for (i = 0; i < hart_count; i++) {
			rscratch = sbi_hart_id_to_scratch(scratch, i);
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

	hstate = arch_atomic_cmpxchg(&hdata->state, SBI_HART_STOPPING,
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
	sbi_printf("ERR: Failed stop hart [%u]\n", sbi_current_hartid());
	sbi_hart_hang();
}

int sbi_hsm_hart_start(struct sbi_scratch *scratch, u32 hartid,
		       ulong saddr, ulong priv)
{
	unsigned long init_count;
	unsigned int hstate;
	int rc;
	const struct sbi_platform *plat = sbi_platform_ptr(scratch);
	struct sbi_scratch *rscratch = sbi_hart_id_to_scratch(scratch, hartid);
	struct sbi_hsm_data *hdata = sbi_scratch_offset_ptr(rscratch,
							hart_data_offset);

	if (sbi_platform_hart_disabled(plat, hartid))
		return SBI_EINVAL;
	hstate = arch_atomic_cmpxchg(&hdata->state, SBI_HART_STOPPED,
				     SBI_HART_STARTING);
	if (hstate == SBI_HART_STARTED)
		return SBI_EALREADY_STARTED;

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
					     scratch->warmboot_addr, priv);
	} else {
		sbi_platform_ipi_send(plat, hartid);
	}

	return 0;
}

int sbi_hsm_hart_stop(struct sbi_scratch *scratch, bool exitnow)
{
	int oldstate;
	u32 hartid = sbi_current_hartid();
	const struct sbi_platform *plat = sbi_platform_ptr(scratch);
	struct sbi_hsm_data *hdata = sbi_scratch_offset_ptr(scratch,
							hart_data_offset);

	if (sbi_platform_hart_disabled(plat, hartid) ||
	    !sbi_hsm_hart_started(scratch, hartid))
		return SBI_EINVAL;

	oldstate = arch_atomic_cmpxchg(&hdata->state, SBI_HART_STARTED,
				     SBI_HART_STOPPING);
	if (oldstate != SBI_HART_STARTED) {
		sbi_printf("%s: ERR: The hart is in invalid state [%u]\n",
			   __func__, oldstate);
		return SBI_DENIED;
	}

	if (exitnow)
		sbi_exit(scratch);

	return 0;
}
