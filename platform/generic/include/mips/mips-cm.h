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

#define CPS_ACCESSOR_R(unit, sz, off, name)				\
static inline u##sz read_##unit##_##name(u32 hartid)			\
{									\
	u##sz value;							\
	int cl = cpu_cluster(hartid);					\
	int co = cpu_core(hartid);					\
	long cmd_reg = p8700_cm_info->gcr_base[cl]			\
		       + (co << CM_BASE_CORE_SHIFT)			\
		  + off;						\
	if (DEBUG_CM)							\
		sbi_printf("CM_READ%d(0x%lx) ...\n", sz, cmd_reg);	\
	if (sz == 32)							\
		asm volatile("lw %0,0(%1)":"=r"(value):"r"(cmd_reg));	\
	else if (sz == 64)						\
		asm volatile("ld %0,0(%1)":"=r"(value):"r"(cmd_reg));	\
	asm volatile("fence");						\
	if (DEBUG_CM)							\
		sbi_printf("CM_READ%d(0x%lx) -> 0x%lx\n", sz, cmd_reg, (unsigned long)value);	\
	return value;							\
}

#define CPS_ACCESSOR_W(unit, sz, off, name)				\
static inline void write_##unit##_##name(u32 hartid, u##sz value)	\
{									\
	int cl = cpu_cluster(hartid);					\
	int co = cpu_core(hartid);					\
	long cmd_reg = p8700_cm_info->gcr_base[cl]			\
		       + (co << CM_BASE_CORE_SHIFT)			\
		  + off;						\
	if (DEBUG_CM)							\
		sbi_printf("CM_WRITE%d(0x%lx, 0x%lx)\n", sz, 	\
			    cmd_reg, (unsigned long)value);		\
	if (sz == 32)							\
		asm volatile("sw %0,0(%1)"::"r"(value),"r"(cmd_reg));	\
	else if (sz == 64)						\
		asm volatile("sd %0,0(%1)"::"r"(value),"r"(cmd_reg));	\
	asm volatile("fence");						\
}

#define CPS_ACCESSOR_RW(unit, sz, off, name)			\
	CPS_ACCESSOR_R(unit, sz, off, name)			\
	CPS_ACCESSOR_W(unit, sz, off, name)

#define CPC_CX_ACCESSOR_RW(sz, off, name)				\
	CPS_ACCESSOR_RW(cpc, sz, CPC_OFFSET + CPC_OFF_LOCAL + (off), co_##name)

#define GCR_CX_ACCESSOR_RW(sz, off, name)				\
	CPS_ACCESSOR_RW(gcr, sz, GCR_OFF_LOCAL + (off), co_##name)

GCR_CX_ACCESSOR_RW(64, cpu_hart(hartid) << CM_BASE_HART_SHIFT, reset_base)
GCR_CX_ACCESSOR_RW(32, GCR_CORE_COH_EN, coherence)
GCR_CX_ACCESSOR_RW(64, GCR_BASE_OFFSET, base)

CPC_CX_ACCESSOR_RW(32, CPC_Cx_VP_RUN, vp_run)
CPC_CX_ACCESSOR_RW(32, CPC_Cx_VP_STOP, vp_stop)
CPC_CX_ACCESSOR_RW(32, CPC_Cx_CMD, cmd)
CPC_CX_ACCESSOR_RW(32, CPC_Cx_STAT_CONF, stat_conf)

#define CPC_ACCESSOR_RW(sz, off, name)					\
	CPS_ACCESSOR_RW(cpc, sz, CPC_OFFSET + (off), name)

CPC_ACCESSOR_RW(32, CPC_PWRUP_CTL, pwrup_ctl)
CPC_ACCESSOR_RW(64, CPC_TIMECTL, timectl)
CPC_ACCESSOR_RW(64, CPC_HRTIME, hrtime)
CPC_ACCESSOR_RW(32, CPC_CM_STAT_CONF, cm_stat_conf)

#endif
