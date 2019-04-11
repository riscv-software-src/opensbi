/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#ifndef __SBI_TYPES_H__
#define __SBI_TYPES_H__

/* clang-format off */

typedef char			s8;
typedef unsigned char		u8;
typedef unsigned char		uint8_t;

typedef short			s16;
typedef unsigned short		u16;
typedef short			int16_t;
typedef unsigned short		uint16_t;

typedef int			s32;
typedef unsigned int		u32;
typedef int			int32_t;
typedef unsigned int		uint32_t;

#if __riscv_xlen == 64
typedef long			s64;
typedef unsigned long		u64;
typedef long			int64_t;
typedef unsigned long		uint64_t;
#define PRILX			"016lx"
#elif __riscv_xlen == 32
typedef long long		s64;
typedef unsigned long long	u64;
typedef long long		int64_t;
typedef unsigned long long	uint64_t;
#define PRILX			"08lx"
#else
#error "Unexpected __riscv_xlen"
#endif

typedef int			bool;
typedef unsigned long		ulong;
typedef unsigned long		uintptr_t;
typedef unsigned long		size_t;
typedef long			ssize_t;
typedef unsigned long		virtual_addr_t;
typedef unsigned long		virtual_size_t;
typedef unsigned long		physical_addr_t;
typedef unsigned long		physical_size_t;

#define TRUE			1
#define FALSE			0

#define NULL			((void *)0)

#define __packed		__attribute__((packed))
#define __noreturn		__attribute__((noreturn))

/* clang-format on */

#endif
