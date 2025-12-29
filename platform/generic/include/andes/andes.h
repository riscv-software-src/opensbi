/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2024 Andes Technology Corporation
 */

#ifndef _RISCV_ANDES_H
#define _RISCV_ANDES_H

#include <sbi/sbi_bitops.h>
#include <sbi/sbi_scratch.h>

/* Memory and Miscellaneous Registers */
#define CSR_MPFT_CTL		0x7c5
#define CSR_MCACHE_CTL		0x7ca
#define CSR_MCCTLCOMMAND	0x7cc
#define CSR_MMISC_CTL		0x7d0

/* Configuration Control & Status Registers */
#define CSR_MICM_CFG		0xfc0
#define CSR_MDCM_CFG		0xfc1
#define CSR_MMSC_CFG		0xfc2

/* Trap Related Registers */
#define CSR_MXSTATUS		0x7c4
#define CSR_MSLIDELEG		0x7d5
#define CSR_SLIE		0x9c4
#define CSR_SLIP		0x9c5

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

#define MCACHE_CTL_IC_EN_MASK		BIT(0)
#define MCACHE_CTL_DC_EN_MASK		BIT(1)
#define MCACHE_CTL_CCTL_SUEN_OFFSET	8
#define MCACHE_CTL_CCTL_SUEN_MASK	(1 << MCACHE_CTL_CCTL_SUEN_OFFSET)
#define MCACHE_CTL_DC_COHEN_MASK	BIT(19)
#define MCACHE_CTL_DC_COHSTA_MASK	BIT(20)

/* Performance monitor */
#define MMSC_CFG_PMNDS_MASK		(1 << 15)
#define MIP_PMOVI			(1 << 18)

/* Cache control commands */
#define MCCTLCOMMAND_L1D_WBINVAL_ALL	6

/* AE350 platform specific sleep types */
#define SBI_SUSP_AE350_LIGHT_SLEEP	SBI_SUSP_PLATFORM_SLEEP_START

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

struct andes_hart_data {
	unsigned long mcache_ctl;
	unsigned long mmisc_ctl;
	unsigned long mpft_ctl;
	unsigned long mslideleg;
	unsigned long mxstatus;
	unsigned long slie;
	unsigned long slip;
	unsigned long pmacfg0;
	unsigned long pmacfg2;
	unsigned long pmaaddrX[16];
};

void ae350_non_ret_save(struct sbi_scratch *scratch);
void ae350_non_ret_restore(struct sbi_scratch *scratch);
void ae350_enable_coherency_warmboot(void);

/*
 * On Andes 4X-series CPUs, disabling the L1 data cache causes the CPU to fetch
 * data directly from RAM. However, L1 cache flushes write data back to the
 * Last Level Cache (LLC). This discrepancy can lead to return address
 * corruption on the stack. To prevent this, the following functions must
 * be inlined.
 */
static inline void ae350_disable_coherency(void)
{
	/*
	 * To disable cache coherency of a core in AE350 platform, follow below steps:
	 *
	 * 1) Disable I/D-Cache
	 * 2) Write back and invalidate D-Cache
	 * 3) Disable D-Cache coherency
	 * 4) Wait for D-Cache disengaged from the coherence management
	 */
	csr_clear(CSR_MCACHE_CTL, MCACHE_CTL_IC_EN_MASK | MCACHE_CTL_DC_EN_MASK);
	csr_write(CSR_MCCTLCOMMAND, MCCTLCOMMAND_L1D_WBINVAL_ALL);
	csr_clear(CSR_MCACHE_CTL, MCACHE_CTL_DC_COHEN_MASK);
	while (csr_read(CSR_MCACHE_CTL) & MCACHE_CTL_DC_COHSTA_MASK)
		;
}

static inline void ae350_enable_coherency(void)
{
	/*
	 * To enable cache coherency of a core in AE350 platform, follow below steps:
	 *
	 * 1) Enable D-Cache coherency
	 * 2) Wait for D-Cache engaging in the coherence management
	 * 3) Enable I/D-Cache
	 */
	csr_set(CSR_MCACHE_CTL, MCACHE_CTL_DC_COHEN_MASK);

	/*
	 * mcache_ctl.DC_COHEN is hardwired to 0 if there is no coherence
	 * manager. In such situation, just enable the I/D-Cache to prevent
	 * permanently being stuck in the while loop.
	 */
	if (csr_read(CSR_MCACHE_CTL) & MCACHE_CTL_DC_COHEN_MASK)
		while (!(csr_read(CSR_MCACHE_CTL) & MCACHE_CTL_DC_COHSTA_MASK))
			;

	csr_set(CSR_MCACHE_CTL, MCACHE_CTL_IC_EN_MASK | MCACHE_CTL_DC_EN_MASK);
}

#endif /* _RISCV_ANDES_H */
