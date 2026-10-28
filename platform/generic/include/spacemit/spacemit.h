/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2025 SpacemiT
 * Authors:
 *   Xianbin Zhu <xianbin.zhu@linux.spacemit.com>
 *   Troy Mitchell <troy.mitchell@linux.spacemit.com>
 */

#ifndef __RISCV_SPACEMIT_H__
#define __RISCV_SPACEMIT_H__

void cci_enable_snoop_dvm_reqs(const int *cci_map, unsigned int master_id);

#endif
