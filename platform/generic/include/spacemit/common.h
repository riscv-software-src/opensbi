/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2025 SpacemiT
 * Authors:
 *   Xianbin Zhu <xianbin.zhu@linux.spacemit.com>
 *   Troy Mitchell <troy.mitchell@linux.spacemit.com>
 */

#ifndef __RISCV_SPACEMIT_COMMON_H__
#define __RISCV_SPACEMIT_COMMON_H__

#include <sbi/sbi_const.h>

#ifndef __ASSEMBLY__
#include <sbi/sbi_bitops.h>
#endif

#define CSR_MSETUP				0x7c0
#define CSR_ML2SETUP				0x7f0

/* dcache enable */
#define MSETUP_DE				_BITUL(0)
/* icache enable */
#define MSETUP_IE				_BITUL(1)
/* branch prediction enable */
#define MSETUP_BPE				_BITUL(4)
/* prefetch functionality enable */
#define MSETUP_PFE				_BITUL(5)
/* misaligned memory access enable */
#define MSETUP_MME				_BITUL(6)
/* ECC enable */
#define MSETUP_ECCE				_BITUL(16)

#define PMU_AP_BASE				0xd4282800

#define PMU_AP_CORE0_WAKEUP			(PMU_AP_BASE + 0x12c)
#define PMU_AP_CORE1_WAKEUP			(PMU_AP_BASE + 0x130)
#define PMU_AP_CORE2_WAKEUP			(PMU_AP_BASE + 0x134)
#define PMU_AP_CORE3_WAKEUP			(PMU_AP_BASE + 0x138)
#define PMU_AP_CORE4_WAKEUP			(PMU_AP_BASE + 0x324)
#define PMU_AP_CORE5_WAKEUP			(PMU_AP_BASE + 0x328)
#define PMU_AP_CORE6_WAKEUP			(PMU_AP_BASE + 0x32c)
#define PMU_AP_CORE7_WAKEUP			(PMU_AP_BASE + 0x330)

#define PMU_AP_CORE0_IDLE_CFG			(PMU_AP_BASE + 0x124)
#define PMU_AP_CORE1_IDLE_CFG			(PMU_AP_BASE + 0x128)
#define PMU_AP_CORE2_IDLE_CFG			(PMU_AP_BASE + 0x160)
#define PMU_AP_CORE3_IDLE_CFG			(PMU_AP_BASE + 0x164)
#define PMU_AP_CORE4_IDLE_CFG			(PMU_AP_BASE + 0x304)
#define PMU_AP_CORE5_IDLE_CFG			(PMU_AP_BASE + 0x308)
#define PMU_AP_CORE6_IDLE_CFG			(PMU_AP_BASE + 0x30c)
#define PMU_AP_CORE7_IDLE_CFG			(PMU_AP_BASE + 0x310)

/* boot entry for X100 clusters (C0, C1) */
#define C0_RVBADDR_LO_ADDR			0xd4282db0
#define C0_RVBADDR_HI_ADDR			0xd4282db4
#define C1_RVBADDR_LO_ADDR			0xd4282eb0
#define C1_RVBADDR_HI_ADDR			0xd4282eb4

/* CCI-550 base and slave interface layout */
#define CCI_550_PLATFORM_CCI_ADDR		0xd8500000
#define CCI_550_STATUS				0x000c
#define CCI_550_STATUS_CHANGE_PENDING		BIT(0)
#define CCI_550_SLAVE_IFACE0_OFFSET		0x1000
#define CCI_550_SLAVE_IFACE_OFFSET(idx)		\
		(CCI_550_SLAVE_IFACE0_OFFSET + ((0x1000) * (idx)))
#define CCI_550_SNOOP_CTRL			0x0000
#define CCI_550_SNOOP_CTRL_ENABLE_SNOOPS	BIT(0)
#define CCI_550_SNOOP_CTRL_ENABLE_DVMS		BIT(1)

/*
 * PMU AP per-core idle control register bits (PMU_AP_COREn_IDLE_CFG).
 * Bit definitions are identical in K1 and K3 User Manuals.
 * These replace the original ad-hoc names from k1.h (which incorrectly
 * called bit 1 SRAM_PWRDWN, bit 3 WAKE_MCE, and bit 4 MC_SW_REQ).
 */
/* bit 0: CORE_IDLE - gate core clock externally when core enters WFI */
#define PMU_AP_IDLE_CORE_IDLE			BIT(0)
/* bit 1: CORE_PWRDWN - power off core on WFI (requires CORE_IDLE set) */
#define PMU_AP_IDLE_CORE_PWRDWN			BIT(1)
/* bit 2: Core L1 SRAM Power Down - reserved in K1, used in K3 */
#define PMU_AP_IDLE_CORE_L1_SRAM_PWRDWN		BIT(2)
/* bit 3: MASK_GIC_NIRQ_TO_CORE - mask nIRQ from GIC, auto-cleared by HW on power-down entry */
#define PMU_AP_IDLE_MASK_GIC_NIRQ		BIT(3)
/* bit 4: MASK_GIC_NFIQ_TO_CORE - mask nFIQ from GIC, auto-cleared by HW on power-down entry */
#define PMU_AP_IDLE_MASK_GIC_NFIQ		BIT(4)

#define PLATFORM_MAX_CPUS_PER_CLUSTER		4

#endif /* __RISCV_SPACEMIT_COMMON_H__ */
