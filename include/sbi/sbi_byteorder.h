/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Ventana Micro Systems Inc.
 */

#ifndef __SBI_BYTEORDER_H__
#define __SBI_BYTEORDER_H__

#include <sbi/sbi_types.h>

#define EXTRACT_BYTE(x, n)	((unsigned long long)((uint8_t *)&x)[n])

#define BSWAP16(x)	((EXTRACT_BYTE(x, 0) << 8) | EXTRACT_BYTE(x, 1))
#define BSWAP32(x)	((EXTRACT_BYTE(x, 0) << 24) | (EXTRACT_BYTE(x, 1) << 16) | \
			 (EXTRACT_BYTE(x, 2) << 8) | EXTRACT_BYTE(x, 3))
#define BSWAP64(x)	((EXTRACT_BYTE(x, 0) << 56) | (EXTRACT_BYTE(x, 1) << 48) | \
			 (EXTRACT_BYTE(x, 2) << 40) | (EXTRACT_BYTE(x, 3) << 32) | \
			 (EXTRACT_BYTE(x, 4) << 24) | (EXTRACT_BYTE(x, 5) << 16) | \
			 (EXTRACT_BYTE(x, 6) << 8) | EXTRACT_BYTE(x, 7))


#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__  /* CPU(little-endian) */
#define cpu_to_be16(x)		((uint16_t)BSWAP16(x))
#define cpu_to_be32(x)		((uint32_t)BSWAP32(x))
#define cpu_to_be64(x)		((uint64_t)BSWAP64(x))

#define be16_to_cpu(x)		((uint16_t)BSWAP16(x))
#define be32_to_cpu(x)		((uint32_t)BSWAP32(x))
#define be64_to_cpu(x)		((uint64_t)BSWAP64(x))

#define cpu_to_le16(x)		((uint16_t)(x))
#define cpu_to_le32(x)		((uint32_t)(x))
#define cpu_to_le64(x)		((uint64_t)(x))

#define le16_to_cpu(x)		((uint16_t)(x))
#define le32_to_cpu(x)		((uint32_t)(x))
#define le64_to_cpu(x)		((uint64_t)(x))
#else /* CPU(big-endian) */
#define cpu_to_be16(x)		((uint16_t)(x))
#define cpu_to_be32(x)		((uint32_t)(x))
#define cpu_to_be64(x)		((uint64_t)(x))

#define be16_to_cpu(x)		((uint16_t)(x))
#define be32_to_cpu(x)		((uint32_t)(x))
#define be64_to_cpu(x)		((uint64_t)(x))

#define cpu_to_le16(x)		((uint16_t)BSWAP16(x))
#define cpu_to_le32(x)		((uint32_t)BSWAP32(x))
#define cpu_to_le64(x)		((uint64_t)BSWAP64(x))

#define le16_to_cpu(x)		((uint16_t)BSWAP16(x))
#define le32_to_cpu(x)		((uint32_t)BSWAP32(x))
#define le64_to_cpu(x)		((uint64_t)BSWAP64(x))
#endif

#endif /* __SBI_BYTEORDER_H__ */
