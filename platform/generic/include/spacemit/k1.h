/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2025 SpacemiT
 * Authors:
 *   Xianbin Zhu <xianbin.zhu@linux.spacemit.com>
 *   Troy Mitchell <troy.mitchell@linux.spacemit.com>
 */

#ifndef __RISCV_SPACEMIT_K1_H__
#define __RISCV_SPACEMIT_K1_H__

#include <spacemit/common.h>

#define CSR_MRAOP				0x7c2

#define MRAOP_CACHE_FLUSH			GENMASK(1, 0)

#define PMU_AP_IDLE_PWRDOWN_K1_MASK		(PMU_AP_IDLE_CORE_IDLE |	\
						 PMU_AP_IDLE_CORE_PWRDWN |	\
						 PMU_AP_IDLE_MASK_GIC_NIRQ |	\
						 PMU_AP_IDLE_MASK_GIC_NFIQ)

#define CPU_TO_CLUSTER(cpu)			((cpu) / PLATFORM_MAX_CPUS_PER_CLUSTER)

#define PLAT_CCI_K1_CLUSTER0_IFACE_IX		0
#define PLAT_CCI_K1_CLUSTER1_IFACE_IX		1

#endif /* __RISCV_SPACEMIT_K1_H__ */
