/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2024 Andes Technology Corporation
 */

#ifndef _RISCV_ANDES_H
#define _RISCV_ANDES_H

/* Memory and Miscellaneous Registers */
#define CSR_MCACHE_CTL		0x7ca
#define CSR_MCCTLCOMMAND	0x7cc

/* Configuration Control & Status Registers */
#define CSR_MICM_CFG		0xfc0
#define CSR_MDCM_CFG		0xfc1
#define CSR_MMSC_CFG		0xfc2

/* Machine Trap Related Registers */
#define CSR_MSLIDELEG		0x7d5

/* Counter Related Registers */
#define CSR_MCOUNTERWEN		0x7ce
#define CSR_MCOUNTERINTEN	0x7cf
#define CSR_MCOUNTERMASK_M	0x7d1
#define CSR_MCOUNTERMASK_S	0x7d2
#define CSR_MCOUNTERMASK_U	0x7d3
#define CSR_MCOUNTEROVF		0x7d4

/* PMA Related Registers */
#define CSR_PMACFG0		0xbc0
#define CSR_PMAADDR0		0xbd0

#define MICM_CFG_ISZ_OFFSET		6
#define MICM_CFG_ISZ_MASK		(7 << MICM_CFG_ISZ_OFFSET)

#define MDCM_CFG_DSZ_OFFSET		6
#define MDCM_CFG_DSZ_MASK		(7 << MDCM_CFG_DSZ_OFFSET)

#define MMSC_CFG_CCTLCSR_OFFSET		16
#define MMSC_CFG_CCTLCSR_MASK		(1 << MMSC_CFG_CCTLCSR_OFFSET)
#define MMSC_CFG_PPMA_OFFSET		30
#define MMSC_CFG_PPMA_MASK		(1 << MMSC_CFG_PPMA_OFFSET)
#define MMSC_IOCP_OFFSET		47
#define MMSC_IOCP_MASK			(1ULL << MMSC_IOCP_OFFSET)

#define MCACHE_CTL_CCTL_SUEN_OFFSET	8
#define MCACHE_CTL_CCTL_SUEN_MASK	(1 << MCACHE_CTL_CCTL_SUEN_OFFSET)

/* Performance monitor */
#define MMSC_CFG_PMNDS_MASK		(1 << 15)
#define MIP_PMOVI			(1 << 18)

#ifndef __ASSEMBLER__

#define is_andes(series)				\
({							\
	char value = csr_read(CSR_MARCHID) & 0xff;	\
	(series) == (value >> 4) * 10 + (value & 0x0f);	\
})

#define has_andes_pmu()					\
({							\
	(((csr_read(CSR_MMSC_CFG) &			\
	   MMSC_CFG_PMNDS_MASK)				\
	  && misa_extension('S')) ? true : false);	\
})

#endif /* __ASSEMBLER__ */

#endif /* _RISCV_ANDES_H */
