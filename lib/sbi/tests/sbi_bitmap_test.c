/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Author: Ivan Orlov <ivan.orlov0322@gmail.com>
 */
#include <sbi/sbi_bitmap.h>
#include <sbi/sbi_unit_test.h>

#define DATA_SIZE sizeof(data_zero)
#define DATA_BIT_SIZE (DATA_SIZE * 8)

static unsigned long data_a[] = { 0xDEADBEEF, 0x00BAB10C, 0x1BADB002, 0xABADBABE };
static unsigned long data_b[] = { 0xC00010FF, 0x00BAB10C, 0xBAAAAAAD, 0xBADDCAFE };
static unsigned long data_zero[] = { 0, 0, 0, 0 };

static void bitmap_and_test(struct sbiunit_test_case *test)
{
	unsigned long res[DATA_SIZE];
	unsigned long a_and_b[] = { data_a[0] & data_b[0], data_a[1] & data_b[1],
				    data_a[2] & data_b[2], data_a[3] & data_b[3] };

	__bitmap_and(res, data_a, data_b, DATA_BIT_SIZE);
	SBIUNIT_EXPECT_MEMEQ(test, res, a_and_b, DATA_SIZE);

	/* a & a = a */
	__bitmap_and(res, data_a, data_a, DATA_BIT_SIZE);
	SBIUNIT_ASSERT_MEMEQ(test, res, data_a, DATA_SIZE);

	/* a & 0 = 0 */
	__bitmap_and(res, data_a, data_zero, DATA_BIT_SIZE);
	SBIUNIT_EXPECT_MEMEQ(test, res, data_zero, DATA_SIZE);

	/* 0 & 0 = 0 */
	__bitmap_and(res, data_zero, data_zero, DATA_BIT_SIZE);
	SBIUNIT_EXPECT_MEMEQ(test, res, data_zero, DATA_SIZE);

	sbi_memcpy(res, data_zero, DATA_SIZE);
	/* Cover zero 'bits' argument */
	__bitmap_and(res, data_a, data_b, 0);
	SBIUNIT_EXPECT_MEMEQ(test, res, data_zero, DATA_SIZE);
}

static void bitmap_or_test(struct sbiunit_test_case *test)
{
	unsigned long res[DATA_SIZE];
	unsigned long a_or_b[] = { data_a[0] | data_b[0], data_a[1] | data_b[1],
				   data_a[2] | data_b[2], data_a[3] | data_b[3] };

	__bitmap_or(res, data_a, data_b, DATA_BIT_SIZE);
	SBIUNIT_EXPECT_MEMEQ(test, res, a_or_b, DATA_SIZE);

	/* a | a = a */
	__bitmap_or(res, data_a, data_a, DATA_BIT_SIZE);
	SBIUNIT_EXPECT_MEMEQ(test, res, data_a, DATA_SIZE);

	/* a | 0 = a */
	__bitmap_or(res, data_a, data_zero, DATA_BIT_SIZE);
	SBIUNIT_EXPECT_MEMEQ(test, res, data_a, DATA_SIZE);

	/* 0 | 0 = 0 */
	__bitmap_or(res, data_zero, data_zero, DATA_BIT_SIZE);
	SBIUNIT_EXPECT_MEMEQ(test, res, data_zero, DATA_SIZE);

	sbi_memcpy(res, data_zero, DATA_SIZE);
	__bitmap_or(res, data_a, data_b, 0);
	SBIUNIT_EXPECT_MEMEQ(test, res, data_zero, DATA_SIZE);
}

static void bitmap_xor_test(struct sbiunit_test_case *test)
{
	unsigned long res[DATA_SIZE];
	unsigned long a_xor_b[] = { data_a[0] ^ data_b[0], data_a[1] ^ data_b[1],
				    data_a[2] ^ data_b[2], data_a[3] ^ data_b[3] };

	__bitmap_xor(res, data_a, data_b, DATA_BIT_SIZE);
	SBIUNIT_EXPECT_MEMEQ(test, res, a_xor_b, DATA_SIZE);

	/* a ^ 0 = a */
	__bitmap_xor(res, data_a, data_zero, DATA_BIT_SIZE);
	SBIUNIT_EXPECT_MEMEQ(test, res, data_a, DATA_SIZE);

	/* a ^ a = 0 */
	__bitmap_xor(res, data_a, data_a, DATA_BIT_SIZE);
	SBIUNIT_EXPECT_MEMEQ(test, res, data_zero, DATA_SIZE);

	/* 0 ^ 0 = 0 */
	__bitmap_xor(res, data_zero, data_zero, DATA_BIT_SIZE);
	SBIUNIT_EXPECT_MEMEQ(test, res, data_zero, DATA_SIZE);

	sbi_memcpy(res, data_zero, DATA_SIZE);
	__bitmap_xor(res, data_a, data_b, 0);
	SBIUNIT_EXPECT_MEMEQ(test, res, data_zero, DATA_SIZE);
}

static struct sbiunit_test_case bitmap_test_cases[] = {
	SBIUNIT_TEST_CASE(bitmap_and_test),
	SBIUNIT_TEST_CASE(bitmap_or_test),
	SBIUNIT_TEST_CASE(bitmap_xor_test),
	SBIUNIT_END_CASE,
};

SBIUNIT_TEST_SUITE(bitmap_test_suite, bitmap_test_cases);
