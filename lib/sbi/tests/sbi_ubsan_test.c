/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Author: Marcos Oduardo <marcos.oduardo@gmail.com>
 */

#include <sbi/sbi_unit_test.h>
#include <sbi/sbi_types.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_ubsan.h>

#define UBSAN_EXPECT_FIRES(test, stmt)                                    \
	do {                                                              \
		unsigned long _before = sbi_ubsan_report_count;           \
		stmt;                                                     \
		SBIUNIT_EXPECT_NE(test, sbi_ubsan_report_count, _before); \
	} while (0)

static void test_ubsan_add_overflow(struct sbiunit_test_case *test)
{
	volatile int a = 0x7FFFFFFF; //INT_MAx
	volatile int b = 1;
	volatile int c;
	UBSAN_EXPECT_FIRES(test, c = a + b);
	(void)c;
}

static void test_ubsan_sub_overflow(struct sbiunit_test_case *test)
{
	volatile int a = 0x80000000; //INT_MIN
	volatile int b = 1;
	volatile int c;
	UBSAN_EXPECT_FIRES(test, c = a - b);
	(void)c;
}

static void test_ubsan_mul_overflow(struct sbiunit_test_case *test)
{
	volatile int a = 0x7FFFFFFF;
	volatile int b = 2;
	volatile int c;
	UBSAN_EXPECT_FIRES(test, c = a * b);
	(void)c;
}

static void test_ubsan_divrem(struct sbiunit_test_case *test)
{
	volatile int a = 10;
	volatile int b = 0;
	volatile int c;
	UBSAN_EXPECT_FIRES(test, c = a / b);
	(void)c;
}

static void test_ubsan_oob(struct sbiunit_test_case *test)
{
	volatile int idx = 5;
	int arr[3]	 = { 1, 2, 3 };
	volatile int val;
	UBSAN_EXPECT_FIRES(test, val = arr[idx]);
	(void)val;
}

static void test_ubsan_shift_too_large(struct sbiunit_test_case *test)
{
	volatile unsigned long val = 1;
	volatile int shift	   = 64;
	volatile unsigned long res;
	UBSAN_EXPECT_FIRES(test, res = val << shift);
	(void)res;
}

static void test_ubsan_shift_negative(struct sbiunit_test_case *test)
{
	volatile int val   = 1;
	volatile int shift = -1;
	volatile int res;

	UBSAN_EXPECT_FIRES(test, res = val << shift);
	(void)res;
}

static void test_ubsan_load_invalid_bool(struct sbiunit_test_case *test)
{
	volatile char bool_val = 5;
	volatile bool *b_ptr   = (bool *)&bool_val;
	volatile int taken     = 0;
	UBSAN_EXPECT_FIRES(test, if (*b_ptr) taken = 1);
	(void)taken;
}

static void test_ubsan_pointer_overflow(struct sbiunit_test_case *test)
{
	volatile uintptr_t base = 0xFFFFFFFFFFFFFFFEUL;
	volatile char *ptr	= (char *)base;
	volatile char *res;
	UBSAN_EXPECT_FIRES(test, res = ptr + 5);
	(void)res;
}

static struct sbiunit_test_case ubsan_tests[] = {
	SBIUNIT_TEST_CASE(test_ubsan_add_overflow),
	SBIUNIT_TEST_CASE(test_ubsan_sub_overflow),
	SBIUNIT_TEST_CASE(test_ubsan_mul_overflow),
	SBIUNIT_TEST_CASE(test_ubsan_divrem),
	SBIUNIT_TEST_CASE(test_ubsan_oob),
	SBIUNIT_TEST_CASE(test_ubsan_shift_too_large),
	SBIUNIT_TEST_CASE(test_ubsan_shift_negative),
	SBIUNIT_TEST_CASE(test_ubsan_load_invalid_bool),
	SBIUNIT_TEST_CASE(test_ubsan_pointer_overflow),
	SBIUNIT_END_CASE,
};

SBIUNIT_TEST_SUITE(ubsan_test_suite, ubsan_tests);
