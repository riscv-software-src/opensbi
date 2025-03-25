// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (C) 2023 Renesas Electronics Corp.
 *
 */
#include <andes/andes.h>
#include <andes/andes_sbi.h>
#include <andes/andes_pma.h>
#include <sbi/riscv_asm.h>
#include <sbi/sbi_error.h>

enum sbi_ext_andes_fid {
	SBI_EXT_ANDES_FID0 = 0, /* Reserved for future use */
	SBI_EXT_ANDES_IOCP_SW_WORKAROUND,
	SBI_EXT_ANDES_PMA_PROBE,
	SBI_EXT_ANDES_PMA_SET,
	SBI_EXT_ANDES_PMA_FREE,
};

static bool andes_cache_controllable(void)
{
	return (((csr_read(CSR_MICM_CFG) & MICM_CFG_ISZ_MASK) ||
		 (csr_read(CSR_MDCM_CFG) & MDCM_CFG_DSZ_MASK)) &&
		(csr_read(CSR_MMSC_CFG) & MMSC_CFG_CCTLCSR_MASK) &&
		(csr_read(CSR_MCACHE_CTL) & MCACHE_CTL_CCTL_SUEN_MASK) &&
		misa_extension('U'));
}

static bool andes_iocp_disabled(void)
{
	return (csr_read(CSR_MMSC_CFG) & MMSC_IOCP_MASK) ? false : true;
}

static bool andes_apply_iocp_sw_workaround(void)
{
	return andes_cache_controllable() && andes_iocp_disabled();
}

int andes_sbi_vendor_ext_provider(long funcid,
				  struct sbi_trap_regs *regs,
				  struct sbi_ecall_return *out)
{
	int ret = 0;

	switch (funcid) {
	case SBI_EXT_ANDES_IOCP_SW_WORKAROUND:
		out->value = andes_apply_iocp_sw_workaround();
		break;
	case SBI_EXT_ANDES_PMA_PROBE:
		out->value = andes_sbi_probe_pma();
		break;
	case SBI_EXT_ANDES_PMA_SET:
		ret = andes_sbi_set_pma(regs->a0, regs->a1, regs->a2);
		break;
	case SBI_EXT_ANDES_PMA_FREE:
		ret = andes_sbi_free_pma(regs->a0);
		break;
	default:
		ret = SBI_ENOTSUPP;
	}

	return ret;
}
