/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 Andes Technology Corporation
 */

#include <andes/andes.h>
#include <andes/andes_pmu.h>
#include <andes/andes_sbi.h>
#include <platform_override.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/riscv_asm.h>
#include <sbi/sbi_init.h>
#include <sbi/sbi_scratch.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/hsm/fdt_hsm_andes_atcsmu.h>

static unsigned long andes_hart_data_offset;
extern void _start_warm(void);

void ae350_non_ret_save(struct sbi_scratch *scratch)
{
	struct andes_hart_data *andes_hdata = sbi_scratch_offset_ptr(scratch,
								     andes_hart_data_offset);

	andes_hdata->mcache_ctl = csr_read(CSR_MCACHE_CTL);
	andes_hdata->mmisc_ctl = csr_read(CSR_MMISC_CTL);
	andes_hdata->mpft_ctl = csr_read(CSR_MPFT_CTL);
	andes_hdata->mslideleg = csr_read(CSR_MSLIDELEG);
	andes_hdata->mxstatus = csr_read(CSR_MXSTATUS);
	andes_hdata->slie = csr_read(CSR_SLIE);
	andes_hdata->slip = csr_read(CSR_SLIP);
	andes_hdata->pmacfg0 = csr_read(CSR_PMACFG0);
	andes_hdata->pmacfg2 = csr_read_num(CSR_PMACFG0 + 2);
	for (int i = 0; i < 16; i++)
		andes_hdata->pmaaddrX[i] = csr_read_num(CSR_PMAADDR0 + i);
}

void ae350_non_ret_restore(struct sbi_scratch *scratch)
{
	struct andes_hart_data *andes_hdata = sbi_scratch_offset_ptr(scratch,
								     andes_hart_data_offset);

	csr_write(CSR_MCACHE_CTL, andes_hdata->mcache_ctl);
	csr_write(CSR_MMISC_CTL, andes_hdata->mmisc_ctl);
	csr_write(CSR_MPFT_CTL, andes_hdata->mpft_ctl);
	csr_write(CSR_MSLIDELEG, andes_hdata->mslideleg);
	csr_write(CSR_MXSTATUS, andes_hdata->mxstatus);
	csr_write(CSR_SLIE, andes_hdata->slie);
	csr_write(CSR_SLIP, andes_hdata->slip);
	csr_write(CSR_PMACFG0, andes_hdata->pmacfg0);
	csr_write_num(CSR_PMACFG0 + 2, andes_hdata->pmacfg2);
	for (int i = 0; i < 16; i++)
		csr_write_num(CSR_PMAADDR0 + i, andes_hdata->pmaaddrX[i]);
}

void ae350_enable_coherency_warmboot(void)
{
	ae350_enable_coherency();
	_start_warm();
}

static int ae350_early_init(bool cold_boot)
{
	u32 hartid = current_hartid();
	u32 sleep_type = atcsmu_get_sleep_type(hartid);

	if (cold_boot) {
		andes_hart_data_offset = sbi_scratch_alloc_offset(sizeof(struct andes_hart_data));
		if (!andes_hart_data_offset)
			return SBI_ENOMEM;
	}

	/* Don't restore Andes CSRs during boot or wake up from light sleep */
	if (sbi_init_count(current_hartindex()) && sleep_type == SBI_SUSP_SLEEP_TYPE_SUSPEND)
		ae350_non_ret_restore(sbi_scratch_thishart_ptr());

	return generic_early_init(cold_boot);
}

static int ae350_platform_init(const void *fdt, int nodeoff, const struct fdt_match *match)
{
	generic_platform_ops.early_init = ae350_early_init;
	generic_platform_ops.extensions_init = andes_pmu_extensions_init;
	generic_platform_ops.pmu_init = andes_pmu_init;
	generic_platform_ops.vendor_ext_provider = andes_sbi_vendor_ext_provider;

	return 0;
}

static const struct fdt_match andes_ae350_match[] = {
	{ .compatible = "andestech,ae350" },
	{ },
};

const struct fdt_driver andes_ae350 = {
	.match_table = andes_ae350_match,
	.init = ae350_platform_init,
};
