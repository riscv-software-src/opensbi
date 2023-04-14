/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022 Andes Technology Corporation
 *
 * Authors:
 *   Yu Chien Peter Lin <peterlin@andestech.com>
 */

#include <platform_override.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/fdt/fdt_fixup.h>
#include <sbi_utils/sys/atcsmu.h>
#include <sbi/riscv_asm.h>
#include <sbi/sbi_bitops.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_hsm.h>
#include <sbi/sbi_ipi.h>
#include <sbi/sbi_init.h>
#include <andes/andes45.h>

static struct smu_data smu = { 0 };
extern void __ae350_enable_coherency_warmboot(void);
extern void __ae350_disable_coherency(void);

static __always_inline bool is_andes25(void)
{
	ulong marchid = csr_read(CSR_MARCHID);
	return !!(EXTRACT_FIELD(marchid, CSR_MARCHID_MICROID) == 0xa25);
}

static int ae350_hart_start(u32 hartid, ulong saddr)
{
	/* Don't send wakeup command at boot-time */
	if (!sbi_init_count(hartid) || (is_andes25() && hartid == 0))
		return sbi_ipi_raw_send(hartid);

	/* Write wakeup command to the sleep hart */
	smu_set_command(&smu, WAKEUP_CMD, hartid);

	return 0;
}

static int ae350_hart_stop(void)
{
	int rc;
	u32 hartid = current_hartid();

	/**
	 * For Andes AX25MP, the hart0 shares power domain with
	 * L2-cache, instead of turning it off, it should fall
	 * through and jump to warmboot_addr.
	 */
	if (is_andes25() && hartid == 0)
		return SBI_ENOTSUPP;

	if (!smu_support_sleep_mode(&smu, DEEPSLEEP_MODE, hartid))
		return SBI_ENOTSUPP;

	/**
	 * disable all events, the current hart will be
	 * woken up from reset vector when other hart
	 * writes its PCS (power control slot) control
	 * register
	 */
	smu_set_wakeup_events(&smu, 0x0, hartid);
	smu_set_command(&smu, DEEP_SLEEP_CMD, hartid);

	rc = smu_set_reset_vector(&smu, (ulong)__ae350_enable_coherency_warmboot,
			       hartid);
	if (rc)
		goto fail;

	__ae350_disable_coherency();

	wfi();

fail:
	/* It should never reach here */
	sbi_hart_hang();
	return 0;
}

static const struct sbi_hsm_device andes_smu = {
	.name	      = "andes_smu",
	.hart_start   = ae350_hart_start,
	.hart_stop    = ae350_hart_stop,
};

static void ae350_hsm_device_init(void)
{
	int rc;
	void *fdt;

	fdt = fdt_get_address();

	rc = fdt_parse_compat_addr(fdt, (uint64_t *)&smu.addr,
				   "andestech,atcsmu");

	if (!rc) {
		sbi_hsm_set_device(&andes_smu);
	}
}

static int ae350_final_init(bool cold_boot, const struct fdt_match *match)
{
	if (cold_boot)
		ae350_hsm_device_init();

	return 0;
}

static const struct fdt_match andes_ae350_match[] = {
	{ .compatible = "andestech,ae350" },
	{ },
};

const struct platform_override andes_ae350 = {
	.match_table = andes_ae350_match,
	.final_init  = ae350_final_init,
};
