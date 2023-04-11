#ifndef _RISCV_ANDES45_H
#define _RISCV_ANDES45_H

#define CSR_MARCHID_MICROID 0xfff

/* Memory and Miscellaneous Registers */
#define CSR_MCACHE_CTL		0x7ca
#define CSR_MCCTLCOMMAND	0x7cc

/* Configuration Control & Status Registers */
#define CSR_MICM_CFG		0xfc0
#define CSR_MDCM_CFG		0xfc1
#define CSR_MMSC_CFG		0xfc2

#define MICM_CFG_ISZ_OFFSET		6
#define MICM_CFG_ISZ_MASK		(0x7  << MICM_CFG_ISZ_OFFSET)

#define MDCM_CFG_DSZ_OFFSET		6
#define MDCM_CFG_DSZ_MASK		(0x7  << MDCM_CFG_DSZ_OFFSET)

#define MMSC_CFG_CCTLCSR_OFFSET		16
#define MMSC_CFG_CCTLCSR_MASK		(0x1 << MMSC_CFG_CCTLCSR_OFFSET)
#define MMSC_IOCP_OFFSET			47
#define MMSC_IOCP_MASK			(0x1ULL << MMSC_IOCP_OFFSET)

#define MCACHE_CTL_CCTL_SUEN_OFFSET	8
#define MCACHE_CTL_CCTL_SUEN_MASK	(0x1 << MCACHE_CTL_CCTL_SUEN_OFFSET)

#endif /* _RISCV_ANDES45_H */
