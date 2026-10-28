/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2026 SpacemiT
 * Authors:
 *   Xianbin Zhu <xianbin.zhu@linux.spacemit.com>
 *   Troy Mitchell <troy.mitchell@linux.spacemit.com>
 */

#ifndef __RISCV_SPACEMIT_K3_H__
#define __RISCV_SPACEMIT_K3_H__

#include <spacemit/common.h>

/* PMA configuration CSR */
#define CSR_PMACFG0				0x7de

/*
 * PMACFG0 audio buffer region field: bits [55:48].
 * The Boot ROM maps this region as I/O. Restore it to a cacheable
 * attribute so later firmware can use it for data or instructions.
 */
#define PMACFG0_AUDIO_BUF_FIELD_MASK		_UL(0xff)
#define PMACFG0_AUDIO_BUF_FIELD_SHIFT		48
#define PMACFG0_AUDIO_BUF_ATTR_CACHEABLE	_UL(0x20)

/*
 * PMACFG0 XIP region field: bits [23:16].
 * Lower 12 bits are zero so GAS emits a single lui instruction.
 */
#define PMACFG0_XIP_FIELD_CLEAR			_UL(0x00ff0000)
#define PMACFG0_XIP_ATTR_IO			_UL(0x00220000)

#define CSR_ML2HINT				0x7f7
#define MISA_EXT_H				_BITUL(7)

/* Enable instruction prefetch in L2 cache on miss */
#define ML2SETUP_IPRF				_BITUL(16)
/* Enable TLB prefetch in L2 cache */
#define ML2SETUP_TPRF				_BITUL(18)
/* Enable top-of-clk of trace */
#define ML2HINT_TRACE_TOP_ICGEN			_BITUL(26)

#define MSETUP_BOOT_FLAGS			(MSETUP_DE | MSETUP_IE | \
						 MSETUP_BPE | MSETUP_PFE | \
						 MSETUP_MME | MSETUP_ECCE)

/* A100 cores: hart 8-11 belong to C2 cluster, hart 12-15 to C3 cluster */
#define PMU_CAP_CORE8_WAKEUP			(PMU_AP_BASE + 0x360)
#define PMU_CAP_CORE9_WAKEUP			(PMU_AP_BASE + 0x364)
#define PMU_CAP_CORE10_WAKEUP			(PMU_AP_BASE + 0x368)
#define PMU_CAP_CORE11_WAKEUP			(PMU_AP_BASE + 0x36c)
#define PMU_CAP_CORE12_WAKEUP			(PMU_AP_BASE + 0x22c)
#define PMU_CAP_CORE13_WAKEUP			(PMU_AP_BASE + 0x230)
#define PMU_CAP_CORE14_WAKEUP			(PMU_AP_BASE + 0x234)
#define PMU_CAP_CORE15_WAKEUP			(PMU_AP_BASE + 0x238)

/* A100 per-core idle config registers (harts 8-15) */
#define PMU_CAP_CORE8_IDLE_CFG			(PMU_AP_BASE + 0x340)
#define PMU_CAP_CORE9_IDLE_CFG			(PMU_AP_BASE + 0x344)
#define PMU_CAP_CORE10_IDLE_CFG			(PMU_AP_BASE + 0x348)
#define PMU_CAP_CORE11_IDLE_CFG			(PMU_AP_BASE + 0x34c)
#define PMU_CAP_CORE12_IDLE_CFG			(PMU_AP_BASE + 0x20c)
#define PMU_CAP_CORE13_IDLE_CFG			(PMU_AP_BASE + 0x210)
#define PMU_CAP_CORE14_IDLE_CFG			(PMU_AP_BASE + 0x214)
#define PMU_CAP_CORE15_IDLE_CFG			(PMU_AP_BASE + 0x218)

/* CX CAPMP cluster idle config registers */
#define PMU_CX_CAPMP_IDLE_CFG0		(PMU_AP_BASE + 0x120)
#define PMU_CX_CAPMP_IDLE_CFG1		(PMU_AP_BASE + 0x0e4)
#define PMU_CX_CAPMP_IDLE_CFG2		(PMU_AP_BASE + 0x150)
#define PMU_CX_CAPMP_IDLE_CFG3		(PMU_AP_BASE + 0x154)
#define PMU_CX_CAPMP_IDLE_CFG4		(PMU_AP_BASE + 0x314)
#define PMU_CX_CAPMP_IDLE_CFG5		(PMU_AP_BASE + 0x318)
#define PMU_CX_CAPMP_IDLE_CFG6		(PMU_AP_BASE + 0x31c)
#define PMU_CX_CAPMP_IDLE_CFG7		(PMU_AP_BASE + 0x320)
#define PMU_CX_CAPMP_IDLE_CFG8		(PMU_AP_BASE + 0x350)
#define PMU_CX_CAPMP_IDLE_CFG9		(PMU_AP_BASE + 0x354)
#define PMU_CX_CAPMP_IDLE_CFG10		(PMU_AP_BASE + 0x358)
#define PMU_CX_CAPMP_IDLE_CFG11		(PMU_AP_BASE + 0x35c)
#define PMU_CX_CAPMP_IDLE_CFG12		(PMU_AP_BASE + 0x21c)
#define PMU_CX_CAPMP_IDLE_CFG13		(PMU_AP_BASE + 0x220)
#define PMU_CX_CAPMP_IDLE_CFG14		(PMU_AP_BASE + 0x224)
#define PMU_CX_CAPMP_IDLE_CFG15		(PMU_AP_BASE + 0x228)

#define PMU_AP_IDLE_PWRDOWN_K3_MASK		(PMU_AP_IDLE_CORE_IDLE |		\
						 PMU_AP_IDLE_CORE_PWRDWN |		\
						 PMU_AP_IDLE_CORE_L1_SRAM_PWRDWN |	\
						 PMU_AP_IDLE_MASK_GIC_NIRQ |		\
						 PMU_AP_IDLE_MASK_GIC_NFIQ)

/*
 * PMU CX per-cluster idle control bits (PMU_CX_CAPMPn_IDLE_CFG).
 */
/* bit 0: MP Idle - gate cluster clocks externally on WFI */
#define PMU_CX_IDLE_MP_IDLE			BIT(0)
/* bit 1: MP Power Down - power off cluster logic when idle */
#define PMU_CX_IDLE_MP_PWRDWN			BIT(1)
/* bit 2: L2 Cache SRAM Power Down */
#define PMU_CX_IDLE_L2_SRAM_PWRDWN		BIT(2)
/* bit 3: SCU SRAM Power Down - not used, SCU SRAM does not support retention */
#define PMU_CX_IDLE_MP_SCU_SRAM_PWRDWN		BIT(3)
/* bit 7: ACNACTM Hardware Control - HW controls ACNACTM port on MP low power entry */
#define PMU_CX_IDLE_ACNACTM_HW_CTRL		BIT(7)

#define CLUSTER_PWR_DOWN_VALUE			(PMU_CX_IDLE_MP_IDLE |		\
						 PMU_CX_IDLE_MP_PWRDWN |	\
						 PMU_CX_IDLE_L2_SRAM_PWRDWN |	\
						 PMU_CX_IDLE_MP_SCU_SRAM_PWRDWN | \
						 PMU_CX_IDLE_ACNACTM_HW_CTRL)

/* boot entry for A100 clusters */
#define C2_RVBADDR_LO_ADDR			(0xd4282c00 + 0x3e8)
#define C2_RVBADDR_HI_ADDR			(0xd4282c00 + 0x3ec)
#define C3_RVBADDR_LO_ADDR			(0xd4282c00 + 0x260)
#define C3_RVBADDR_HI_ADDR			(0xd4282c00 + 0x264)

#define PLATFORM_A100_FIRST_ID			8

#define PLAT_CCI_K3_AIDMA_IFACE_IX		0
#define PLAT_CCI_K3_CLUSTER0_IFACE_IX		1
#define PLAT_CCI_K3_CLUSTER1_IFACE_IX		2
#define PLAT_CCI_K3_CLUSTER2_0_IFACE_IX	3
#define PLAT_CCI_K3_CLUSTER2_1_IFACE_IX	4
#define PLAT_CCI_K3_CLUSTER3_0_IFACE_IX	5
#define PLAT_CCI_K3_CLUSTER3_1_IFACE_IX	6

#endif /* __RISCV_SPACEMIT_K3_H__ */
