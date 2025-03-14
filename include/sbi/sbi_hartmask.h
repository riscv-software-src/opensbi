/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#ifndef __SBI_HARTMASK_H__
#define __SBI_HARTMASK_H__

#include <sbi/sbi_bitmap.h>
#include <sbi/sbi_scratch.h>

/**
 * Maximum number of bits in a hartmask
 *
 * The hartmask is indexed using physical HART id so this define
 * also represents the maximum number of HART ids generic OpenSBI
 * can handle.
 */
#define SBI_HARTMASK_MAX_BITS		128

/** Representation of hartmask */
struct sbi_hartmask {
	DECLARE_BITMAP(bits, SBI_HARTMASK_MAX_BITS);
};

/** Initialize hartmask to zero */
#define SBI_HARTMASK_INIT(__m)		\
	bitmap_zero(((__m)->bits), SBI_HARTMASK_MAX_BITS)

/** Initialize hartmask to zero except a particular HART id */
#define SBI_HARTMASK_INIT_EXCEPT(__m, __h)	\
	do { \
		u32 __i = sbi_hartid_to_hartindex(__h); \
		bitmap_zero_except(((__m)->bits), __i, SBI_HARTMASK_MAX_BITS); \
	} while(0)

/**
 * Get underlying bitmap of hartmask
 * @param m the hartmask pointer
 */
#define sbi_hartmask_bits(__m)		((__m)->bits)

/**
 * Set a HART index in hartmask
 * @param i HART index to set
 * @param m the hartmask pointer
 */
static inline void sbi_hartmask_set_hartindex(u32 i, struct sbi_hartmask *m)
{
	if (i < SBI_HARTMASK_MAX_BITS)
		__set_bit(i, m->bits);
}

/**
 * Set a HART id in hartmask
 * @param h HART id to set
 * @param m the hartmask pointer
 */
static inline void sbi_hartmask_set_hartid(u32 h, struct sbi_hartmask *m)
{
	sbi_hartmask_set_hartindex(sbi_hartid_to_hartindex(h), m);
}

/**
 * Clear a HART index in hartmask
 * @param i HART index to clear
 * @param m the hartmask pointer
 */
static inline void sbi_hartmask_clear_hartindex(u32 i, struct sbi_hartmask *m)
{
	if (i < SBI_HARTMASK_MAX_BITS)
		__clear_bit(i, m->bits);
}

/**
 * Clear a HART id in hartmask
 * @param h HART id to clear
 * @param m the hartmask pointer
 */
static inline void sbi_hartmask_clear_hartid(u32 h, struct sbi_hartmask *m)
{
	sbi_hartmask_clear_hartindex(sbi_hartid_to_hartindex(h), m);
}

/**
 * Test a HART index in hartmask
 * @param i HART index to test
 * @param m the hartmask pointer
 */
static inline int sbi_hartmask_test_hartindex(u32 i,
					      const struct sbi_hartmask *m)
{
	if (i < SBI_HARTMASK_MAX_BITS)
		return __test_bit(i, m->bits);
	return 0;
}

/**
 * Test a HART id in hartmask
 * @param h HART id to test
 * @param m the hartmask pointer
 */
static inline int sbi_hartmask_test_hartid(u32 h, const struct sbi_hartmask *m)
{
	return sbi_hartmask_test_hartindex(sbi_hartid_to_hartindex(h), m);
}

/**
 * Set all HARTs in a hartmask
 * @param dstp the hartmask pointer
 */
static inline void sbi_hartmask_set_all(struct sbi_hartmask *dstp)
{
	bitmap_fill(sbi_hartmask_bits(dstp), SBI_HARTMASK_MAX_BITS);
}

/**
 * Clear all HARTs in a hartmask
 * @param dstp the hartmask pointer
 */
static inline void sbi_hartmask_clear_all(struct sbi_hartmask *dstp)
{
	bitmap_zero(sbi_hartmask_bits(dstp), SBI_HARTMASK_MAX_BITS);
}

/**
 * *dstp = *srcp
 * @param dstp the hartmask destination
 * @param srcp the hartmask source
 */
static inline void sbi_hartmask_copy(struct sbi_hartmask *dstp,
				     const struct sbi_hartmask *srcp)
{
	bitmap_copy(sbi_hartmask_bits(dstp), sbi_hartmask_bits(srcp),
		    SBI_HARTMASK_MAX_BITS);
}

/**
 * *dstp = *src1p & *src2p
 * @param dstp the hartmask result
 * @param src1p the first input
 * @param src2p the second input
 */
static inline void sbi_hartmask_and(struct sbi_hartmask *dstp,
				    const struct sbi_hartmask *src1p,
				    const struct sbi_hartmask *src2p)
{
	bitmap_and(sbi_hartmask_bits(dstp), sbi_hartmask_bits(src1p),
		   sbi_hartmask_bits(src2p), SBI_HARTMASK_MAX_BITS);
}

/**
 * *dstp = *src1p | *src2p
 * @param dstp the hartmask result
 * @param src1p the first input
 * @param src2p the second input
 */
static inline void sbi_hartmask_or(struct sbi_hartmask *dstp,
				   const struct sbi_hartmask *src1p,
				   const struct sbi_hartmask *src2p)
{
	bitmap_or(sbi_hartmask_bits(dstp), sbi_hartmask_bits(src1p),
		  sbi_hartmask_bits(src2p), SBI_HARTMASK_MAX_BITS);
}

/**
 * *dstp = *src1p ^ *src2p
 * @param dstp the hartmask result
 * @param src1p the first input
 * @param src2p the second input
 */
static inline void sbi_hartmask_xor(struct sbi_hartmask *dstp,
				    const struct sbi_hartmask *src1p,
				    const struct sbi_hartmask *src2p)
{
	bitmap_xor(sbi_hartmask_bits(dstp), sbi_hartmask_bits(src1p),
		   sbi_hartmask_bits(src2p), SBI_HARTMASK_MAX_BITS);
}

/**
 * Count of bits in *srcp
 * @param srcp the hartmask to count bits in
 *
 * Return: count of bits set in *srcp
 */
static inline int sbi_hartmask_weight(const struct sbi_hartmask *srcp)
{
	return bitmap_weight(sbi_hartmask_bits(srcp), SBI_HARTMASK_MAX_BITS);
}

/**
 * Iterate over each HART index in hartmask
 * __i hart index
 * __m hartmask
*/
#define sbi_hartmask_for_each_hartindex(__i, __m) \
	for((__i) = find_first_bit((__m)->bits, SBI_HARTMASK_MAX_BITS); \
		(__i) < SBI_HARTMASK_MAX_BITS; \
		(__i) = find_next_bit((__m)->bits, SBI_HARTMASK_MAX_BITS, (__i) + 1))

#endif
