/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Ventana Micro Systems Inc.
 */

#ifndef __SBI_BYTEORDER_H__
#define __SBI_BYTEORDER_H__

#ifdef __ASSEMBLER__
# define _conv_cast(type, val) (val)
#else
# include <sbi/sbi_types.h>
# define _conv_cast(type, val) ((type)(val))
#endif

#define __BSWAP16(x)	((((x) & 0x00ff) << 8) | \
			 (((x) & 0xff00) >> 8))
#define __BSWAP32(x)	((((x) & 0x000000ff) << 24) | \
			 (((x) & 0x0000ff00) << 8) | \
			 (((x) & 0x00ff0000) >> 8) | \
			 (((x) & 0xff000000) >> 24))
#define __BSWAP64(x)	((((x) & 0x00000000000000ffULL) << 56) | \
			 (((x) & 0x000000000000ff00ULL) << 40) | \
			 (((x) & 0x0000000000ff0000ULL) << 24) | \
			 (((x) & 0x00000000ff000000ULL) << 8) | \
			 (((x) & 0x000000ff00000000ULL) >> 8) | \
			 (((x) & 0x0000ff0000000000ULL) >> 24) | \
			 (((x) & 0x00ff000000000000ULL) >> 40) | \
			 (((x) & 0xff00000000000000ULL) >> 56))

#define BSWAP64(x) ({ uint64_t _sv = (x); __BSWAP64(_sv); })
#define BSWAP32(x) ({ uint32_t _sv = (x); __BSWAP32(_sv); })
#define BSWAP16(x) ({ uint16_t _sv = (x); __BSWAP16(_sv); })

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__  /* CPU(little-endian) */
#define cpu_to_be16(x)		_conv_cast(uint16_t, BSWAP16(x))
#define cpu_to_be32(x)		_conv_cast(uint32_t, BSWAP32(x))
#define cpu_to_be64(x)		_conv_cast(uint64_t, BSWAP64(x))

#define be16_to_cpu(x)		_conv_cast(uint16_t, BSWAP16(x))
#define be32_to_cpu(x)		_conv_cast(uint32_t, BSWAP32(x))
#define be64_to_cpu(x)		_conv_cast(uint64_t, BSWAP64(x))

#define cpu_to_le16(x)		_conv_cast(uint16_t, (x))
#define cpu_to_le32(x)		_conv_cast(uint32_t, (x))
#define cpu_to_le64(x)		_conv_cast(uint64_t, (x))

#define le16_to_cpu(x)		_conv_cast(uint16_t, (x))
#define le32_to_cpu(x)		_conv_cast(uint32_t, (x))
#define le64_to_cpu(x)		_conv_cast(uint64_t, (x))
#else /* CPU(big-endian) */
#define cpu_to_be16(x)		_conv_cast(uint16_t, (x))
#define cpu_to_be32(x)		_conv_cast(uint32_t, (x))
#define cpu_to_be64(x)		_conv_cast(uint64_t, (x))

#define be16_to_cpu(x)		_conv_cast(uint16_t, (x))
#define be32_to_cpu(x)		_conv_cast(uint32_t, (x))
#define be64_to_cpu(x)		_conv_cast(uint64_t, (x))

#define cpu_to_le16(x)		_conv_cast(uint16_t, BSWAP16(x))
#define cpu_to_le32(x)		_conv_cast(uint32_t, BSWAP32(x))
#define cpu_to_le64(x)		_conv_cast(uint64_t, BSWAP64(x))

#define le16_to_cpu(x)		_conv_cast(uint16_t, BSWAP16(x))
#define le32_to_cpu(x)		_conv_cast(uint32_t, BSWAP32(x))
#define le64_to_cpu(x)		_conv_cast(uint64_t, BSWAP64(x))
#endif

#if __riscv_xlen == 64
#define cpu_to_lle		cpu_to_le64
#define lle_to_cpu		le64_to_cpu
#elif __riscv_xlen == 32
#define cpu_to_lle		cpu_to_le32
#define lle_to_cpu		le32_to_cpu
#else
#error "Unknown __riscv_xlen"
#endif

#endif /* __SBI_BYTEORDER_H__ */
