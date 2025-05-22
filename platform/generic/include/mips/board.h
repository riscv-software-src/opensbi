/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 MIPS
 *
 */

#ifndef __BOARD_H__
#define __BOARD_H__

/* Please review all defines to change for your board. */

/* Use in stw.S, p8700.c, p8700.h, mips-cm.h */
#define CM_BASE			0x16100000

/* Use in mips-cm.h, p8700.c */
#define CLUSTERS_IN_PLATFORM	1
#if CLUSTERS_IN_PLATFORM > 1
/* Define global CM bases for cluster 0, 1, 2, and more. */
#define GLOBAL_CM_BASE0		0
#define GLOBAL_CM_BASE1		0
#define GLOBAL_CM_BASE2		0
#endif

/* Use in stw.S */
#define TIMER_ADDR		(CM_BASE + 0x8050)

/* Use in cps-vec.S */
#define DRAM_ADDRESS		0x80000000
#define DRAM_SIZE		0x80000000
#define DRAM_PMP_ADDR		((DRAM_ADDRESS >> 2) | ((DRAM_SIZE - 1) >> 3))

#endif
