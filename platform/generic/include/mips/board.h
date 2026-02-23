/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 MIPS
 *
 */

#ifndef __BOARD_H__
#define __BOARD_H__

/* Please review all defines to change for your board. */

/* Use in cps-vec.S */
#define DRAM_ADDRESS		0x80000000
#define DRAM_SIZE		0x80000000
#define DRAM_PMP_ADDR		((DRAM_ADDRESS >> 2) | ((DRAM_SIZE - 1) >> 3))

#endif
