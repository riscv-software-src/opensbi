/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 MIPS
 *
 */

#ifndef __MIPS_CM_H__
#define __MIPS_CM_H__

#include <mips/p8700.h>
#include <sbi/sbi_console.h>

/* Define 1 to print out CM read and write info */
#define DEBUG_CM 0

#if CLUSTERS_IN_PLATFORM > 1
static long GLOBAL_CM_BASE[CLUSTERS_IN_PLATFORM] = {GLOBAL_CM_BASE0, GLOBAL_CM_BASE1, GLOBAL_CM_BASE2};
#else
static long GLOBAL_CM_BASE[CLUSTERS_IN_PLATFORM] = {CM_BASE};
#endif

#define CPS_ACCESSOR_R(unit, sz, base, off, name)			\
static inline u##sz read_##unit##_##name(u32 hartid, bool local_p)	\
{									\
	u##sz value;							\
	long cmd_reg;							\
	int cl, co;							\
	cl = cpu_cluster(hartid);					\
	co = cpu_core(hartid);						\
	cmd_reg = (local_p ? (base) : ((base) - CM_BASE + GLOBAL_CM_BASE[cl]))	\
		  + (co << CM_BASE_CORE_SHIFT)				\
		  + off;						\
	if (DEBUG_CM)							\
		sbi_printf("CM READ%d cmd_reg=%lx\n", sz, cmd_reg);	\
	if (sz == 32)							\
		asm volatile("lw %0,0(%1)":"=r"(value):"r"(cmd_reg));	\
	else if (sz == 64)						\
		asm volatile("ld %0,0(%1)":"=r"(value):"r"(cmd_reg));	\
	asm volatile("fence");						\
	return value;							\
}

#define CPS_ACCESSOR_W(unit, sz, base, off, name)			\
static inline void write_##unit##_##name(u32 hartid, u##sz value, bool local_p)	\
{									\
	long cmd_reg;							\
	int cl, co;							\
	cl = cpu_cluster(hartid);					\
	co = cpu_core(hartid);						\
	cmd_reg = (local_p ? (base) : ((base) - CM_BASE +  GLOBAL_CM_BASE[cl]))	\
		  + (co << CM_BASE_CORE_SHIFT)				\
		  + off;						\
	if (DEBUG_CM)							\
		sbi_printf("CM WRITE%d cmd_reg=%lx value=%lx\n", sz, 	\
			    cmd_reg, (unsigned long)value);		\
	if (sz == 32)							\
		asm volatile("sw %0,0(%1)"::"r"(value),"r"(cmd_reg));	\
	else if (sz == 64)						\
		asm volatile("sd %0,0(%1)"::"r"(value),"r"(cmd_reg));	\
	asm volatile("fence");						\
}

#define CPS_ACCESSOR_RW(unit, sz, base, off, name)			\
	CPS_ACCESSOR_R(unit, sz, base, off, name)			\
	CPS_ACCESSOR_W(unit, sz, base, off, name)

#define CPC_CX_ACCESSOR_RW(sz, off, name)				\
	CPS_ACCESSOR_RW(cpc, sz, CPC_BASE, CPC_OFF_LOCAL + (off), co_##name)

#define GCR_CX_ACCESSOR_RW(sz, off, name)				\
	CPS_ACCESSOR_RW(gcr, sz, CM_BASE, GCR_OFF_LOCAL + (off), co_##name)

GCR_CX_ACCESSOR_RW(64, cpu_hart(hartid) << CM_BASE_HART_SHIFT, reset_base)
GCR_CX_ACCESSOR_RW(32, GCR_CORE_COH_EN, coherence)

CPC_CX_ACCESSOR_RW(32, CPC_Cx_VP_RUN, vp_run)
CPC_CX_ACCESSOR_RW(32, CPC_Cx_VP_STOP, vp_stop)
CPC_CX_ACCESSOR_RW(32, CPC_Cx_CMD, cmd)
CPC_CX_ACCESSOR_RW(32, CPC_Cx_STAT_CONF, stat_conf)

#define CPC_ACCESSOR_RW(sz, off, name)					\
	CPS_ACCESSOR_RW(cpc, sz, CPC_BASE, off, name)

CPC_ACCESSOR_RW(32, CPC_PWRUP_CTL, pwrup_ctl)
CPC_ACCESSOR_RW(32, CPC_CM_STAT_CONF, cm_stat_conf)

#endif
