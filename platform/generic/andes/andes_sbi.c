// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (C) 2023 Renesas Electronics Corp.
 *
 */
#include <andes/andes45.h>
#include <andes/andes_sbi.h>
#include <sbi/riscv_asm.h>
#include <sbi/sbi_error.h>

enum sbi_ext_andes_fid {
	SBI_EXT_ANDES_FID0 = 0, /* Reserved for future use */
	SBI_EXT_ANDES_IOCP_SW_WORKAROUND,
};

static bool andes45_cache_controllable(void)
{
	return (((csr_read(CSR_MICM_CFG) & MICM_CFG_ISZ_MASK) ||
		 (csr_read(CSR_MDCM_CFG) & MDCM_CFG_DSZ_MASK)) &&
		(csr_read(CSR_MMSC_CFG) & MMSC_CFG_CCTLCSR_MASK) &&
		(csr_read(CSR_MCACHE_CTL) & MCACHE_CTL_CCTL_SUEN_MASK) &&
		misa_extension('U'));
}

static bool andes45_iocp_disabled(void)
{
	return (csr_read(CSR_MMSC_CFG) & MMSC_IOCP_MASK) ? false : true;
}

static bool andes45_apply_iocp_sw_workaround(void)
{
	return andes45_cache_controllable() & andes45_iocp_disabled();
}

int andes_sbi_vendor_ext_provider(long funcid,
				  const struct sbi_trap_regs *regs,
				  unsigned long *out_value,
				  struct sbi_trap_info *out_trap,
				  const struct fdt_match *match)
{
	switch (funcid) {
	case SBI_EXT_ANDES_IOCP_SW_WORKAROUND:
		*out_value = andes45_apply_iocp_sw_workaround();
		break;

	default:
		return SBI_EINVAL;
	}

	return 0;
}
