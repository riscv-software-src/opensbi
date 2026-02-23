/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 MIPS
 *
 */

#ifndef __P8700_H__
#define __P8700_H__

/** Coherence manager information
 *
 * @num_cm: Number of coherence manager
 * @gcr_base: Array of base address of the CM
 */
struct p8700_cm_info {
	unsigned int num_cm;
	unsigned long *gcr_base;
};

extern const struct p8700_cm_info *p8700_cm_info;

/* PMA */
#define CSR_MIPSPMACFG0		0x7e0
#define CSR_MIPSPMACFG1		0x7e1
#define CSR_MIPSPMACFG2		0x7e2
#define CSR_MIPSPMACFG3		0x7e3
#define CSR_MIPSPMACFG4		0x7e4
#define CSR_MIPSPMACFG5		0x7e5
#define CSR_MIPSPMACFG6		0x7e6
#define CSR_MIPSPMACFG7		0x7e7
#define CSR_MIPSPMACFG8		0x7e8
#define CSR_MIPSPMACFG9		0x7e9
#define CSR_MIPSPMACFG10	0x7ea
#define CSR_MIPSPMACFG11	0x7eb
#define CSR_MIPSPMACFG12	0x7ec
#define CSR_MIPSPMACFG13	0x7ed
#define CSR_MIPSPMACFG14	0x7ee
#define CSR_MIPSPMACFG15	0x7ef

/* MIPS CCA */
#define CCA_CACHE_ENABLE	0
#define CCA_CACHE_DISABLE	2
#define PMA_SPECULATION		(1 << 3)

/* MIPS CSR */
#define CSR_MIPSTVEC		0x7c0
#define CSR_MIPSCACHEERR	0x7c5
#define CSR_MIPSERRCTL		0x7c6
#define CSR_MIPSDIAGDATA	0x7c8
#define CSR_MIPSCONFIG1		0x7d1
#define CSR_MIPSCONFIG5		0x7d5
#define CSR_MIPSCONFIG6		0x7d6
#define CSR_MIPSCONFIG7		0x7d7
#define CSR_MIPSCONFIG8		0x7d8
#define CSR_MIPSCONFIG9		0x7d9
#define CSR_MIPSCONFIG10	0x7da
#define CSR_MIPSCONFIG11	0x7db

/* fields for CSR_MIPSCACHEERR */
#define MIPSCACHEERR_STATE	GENMASK(31,30)
#define MIPSCACHEERR_ARRAY	GENMASK(29,26)
#define MIPSCACHEERR_ERR_BIT	GENMASK(25,20)	/* for correctable */
#define MIPSCACHEERR_F2		BIT(23)		/* for uncorrectable */
#define MIPSCACHEERR_F		BIT(22)		/* for uncorrectable */
#define MIPSCACHEERR_P		BIT(21)		/* for uncorrectable */
#define MIPSCACHEERR_S		BIT(20)		/* for uncorrectable */
#define MIPSCACHEERR_WAY	GENMASK(19,17)
#define MIPSCACHEERR_INDEX	GENMASK(16,4)
#define MIPSCACHEERR_WORD	GENMASK(3,0)

/* fields for CSR_MIPSERRCTL */
#define MIPSERRCTL_PE		BIT(31)
#define MIPSERRCTL_BUS_TO	GENMASK(19,10)

/* fields for CSR_MIPSCONFIG1 */
#define MIPSCONFIG1_L2C		BIT(31)
#define MIPSCONFIG1_IS		GENMASK(24,22)
#define MIPSCONFIG1_IL		GENMASK(21,19)
#define MIPSCONFIG1_IA		GENMASK(18,16)
#define MIPSCONFIG1_DS		GENMASK(15,13)
#define MIPSCONFIG1_DL		GENMASK(12,10)
#define MIPSCONFIG1_DA		GENMASK(9,7)

#define MIPSCONFIG5_MTW		4

/* mhartID structure */
#define P8700_HARTID_CLUSTER	GENMASK(19, 16)
#define P8700_HARTID_CORE	GENMASK(11, 4)
#define P8700_HARTID_HART	GENMASK(3, 0)
#define cpu_cluster(i)		EXTRACT_FIELD(i, P8700_HARTID_CLUSTER)
#define cpu_core(i)		EXTRACT_FIELD(i, P8700_HARTID_CORE)
#define cpu_hart(i)		EXTRACT_FIELD(i, P8700_HARTID_HART)

#define CPC_OFFSET		(0x8000)

#define SIZE_FOR_CPC_MTIME	0x10000	/* The size must be 2^order */
#define AIA_OFFSET		(0x40000)
#define SIZE_FOR_AIA_M_MODE	0x20000	/* The size must be 2^order */
#define P8700_ALIGN		0x10000

#define CM_BASE_HART_SHIFT	3
#define CM_BASE_CORE_SHIFT	8
#define CM_BASE_CLUSTER_SHIFT	19

/* GCR Block offsets */
#define GCR_OFF_LOCAL		0x2000

#define GCR_GLOBAL_CONFIG	0x0000
#define GCR_GC_NUM_CORES	GENMASK(7, 0)
#define GCR_GC_NUM_IOCUS	GENMASK(11, 8)
#define GCR_GC_NUM_MMIOS	GENMASK(19, 16)
#define GCR_GC_NUM_AUX		GENMASK(22, 20)
#define GCR_GC_NUM_CLUSTERS	GENMASK(29, 23)
#define GCR_GC_HAS_ITU		BIT(31)
#define GCR_GC_CL_ID		GENMASK(39, 32)
#define GCR_GC_HAS_DBU		BIT(40)
#define GCR_GC_NOC		GENMASK(43, 41)

#define GCR_BASE_OFFSET		0x0008
#define GCR_CORE_COH_EN		0x00f8
#define GCR_CORE_COH_EN_EN	(0x1 << 0)

#define L2_PFT_CONTROL_OFFSET	0x0300
#define L2_PFT_CONTROL_B_OFFSET	0x0308

#define GCR_L2_CONFIG		0x0130
#define GCR_L2_ASSOC		GENMASK(7, 0)
#define GCR_L2_LINE_SIZE	GENMASK(11, 8)
#define GCR_L2_SET_SIZE		GENMASK(15, 12)
#define GCR_L2_BYPASS		BIT(20)
#define GCR_L2_COP_DATA_ECC_WE	BIT(24)
#define GCR_L2_COP_TAG_ECC_WE	BIT(25)
#define GCR_L2_COP_LRU_WE	BIT(26)
#define GCR_L2_REG_EXISTS	BIT(31)

#define GCR_L2_TAG_ADDR		0x0600
#define GCR_L2_TAG_STATE	0x0608
#define GCR_L2_DATA		0x0610
#define GCR_L2_ECC		0x0618

#define GCR_L2SM_COP		0x0620
#define GCR_L2SM_COP_CMD	GENMASK(1, 0)
#define L2SM_COP_CMD_NOP	0
#define L2SM_COP_CMD_START	1
#define L2SM_COP_CMD_ABORT	3
#define GCR_L2SM_COP_TYPE	GENMASK(4, 2)
#define L2SM_COP_TYPE_IDX_WBINV		0
#define L2SM_COP_TYPE_IDX_STORETAG	1
#define L2SM_COP_TYPE_IDX_STORETAGDATA	2
#define L2SM_COP_TYPE_HIT_INV		4
#define L2SM_COP_TYPE_HIT_WBINV		5
#define L2SM_COP_TYPE_HIT_WB		6
#define L2SM_COP_TYPE_FETCHLOCK		7
#define GCR_L2SM_COP_RUNNING		BIT(5)
#define GCR_L2SM_COP_RESULT		GENMASK(8, 6)
#define L2SM_COP_RESULT_DONTCARE	0
#define L2SM_COP_RESULT_DONE_OK		1
#define L2SM_COP_RESULT_DONE_ERROR	2
#define L2SM_COP_RESULT_ABORT_OK	3
#define L2SM_COP_RESULT_ABORT_ERROR	4
#define GCR_L2SM_COP_PRESENT		BIT(31)
/* MMIO regions. Actual count in GCR_GLOBAL_CONFIG.GCR_GC_NUM_MMIOS */
#define GCR_MMIO_BOTTOM(n)		(0x700 + (n) * 0x10) /* n = 0..7 */
#define GCR_MMIO_TOP(n)			(0x708 + (n) * 0x10) /* n = 0..7 */
#define GCR_MMIO_ADDR			GENMASK(47, 16) /* both top and bottom */
#define GCR_MMIO_BOTTOM_CCA		GENMASK(9, 8)
#define GCR_MMIO_BOTTOM_FORCE_NC	BIT(6)
/*
 * 15:12 - reserved
 * 11 - AUX3
 * 10 - AUX2
 * 9  - AUX1
 * 8  - AUX0
 * 7:1 - reserved
 * 0  - Main memory port; MEM
 */
#define GCR_MMIO_BOTTOM_PORT		GENMASK(5, 2)
#define GCR_MMIO_BOTTOM_DIS_RQ_LIM	BIT(1)
#define GCR_MMIO_BOTTOM_EN		BIT(0)

/* CPC Block offsets */
#define CPC_PWRUP_CTL		0x0030
#define CPC_TIMECTL		0x0058
#define TIMECTL_HARMED		BIT(3)
#define TIMECTL_HSTOP		BIT(2)
#define TIMECTL_MARMED		BIT(1)
#define TIMECTL_MSTOP		BIT(0)
#define CPC_HRTIME		0x0090
#define CPC_CM_STAT_CONF	0x1008

#define CPC_OFF_LOCAL		0x2000

#define CPC_Cx_VP_STOP		0x0020
#define CPC_Cx_VP_RUN		0x0028
#define CPC_Cx_CMD		0x0000

#define CPC_Cx_CMD_PWRUP	0x3
#define CPC_Cx_CMD_RESET	0x4

#define CPC_Cx_STAT_CONF	0x0008
#define CPC_Cx_STAT_CONF_SEQ_STATE	GENMASK(22, 19)
#define CPC_Cx_STAT_CONF_SEQ_STATE_U5	6
#define CPC_Cx_STAT_CONF_SEQ_STATE_U6	7

extern const struct p8700_cm_info *p8700_cm_info;
void mips_p8700_dump_mmio(void);
void mips_p8700_pmp_set(unsigned int n, unsigned long flags,
			unsigned long prot, unsigned long addr,
			unsigned long log2len);
void mips_p8700_power_up_other_cluster(u32 hartid);
int mips_p8700_hart_start(u32 hartid, ulong saddr);
int mips_p8700_hart_stop(void);

struct p8700_cache_info {
	u32 line;
	u32 assoc_ways;
	u32 sets;
};

void mips_p8700_cache_info(struct p8700_cache_info *l1d, struct p8700_cache_info *l1i,
			   struct p8700_cache_info *l2);
int mips_p8700_add_memranges(void);
struct fdt_match;
int mips_p8700_platform_init(const void *fdt, int nodeoff, const struct fdt_match *match);

#endif
