/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Author: Ivan Orlov <ivan.orlov0322@gmail.com>
 */
#ifdef CONFIG_SBIUNIT
#ifndef __SBI_UNIT_H__
#define __SBI_UNIT_H__

#include <sbi/sbi_types.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_string.h>

struct sbiunit_test_case {
	const char *name;
	bool failed;
	void (*test_func)(struct sbiunit_test_case *test);
};

struct sbiunit_test_suite {
	const char *name;
	void (*init)(void);
	struct sbiunit_test_case *cases;
};

#define SBIUNIT_TEST_CASE(func)		\
	{				\
		.name = #func,		\
		.failed = false,	\
		.test_func = (func)	\
	}

#define SBIUNIT_END_CASE { }

#define SBIUNIT_TEST_SUITE(suite_name, cases_arr)		\
	struct sbiunit_test_suite suite_name = {		\
		.name = #suite_name,				\
		.init = NULL,					\
		.cases = cases_arr				\
	}

#define _sbiunit_msg(test, msg) "[SBIUnit] [%s:%d]: %s: %s", __FILE__,	\
				__LINE__, test->name, msg

#define SBIUNIT_INFO(test, msg) sbi_printf(_sbiunit_msg(test, msg))
#define SBIUNIT_PANIC(test, msg) sbi_panic(_sbiunit_msg(test, msg))

#define SBIUNIT_EXPECT(test, cond) do {							\
	if (!(cond)) {									\
		test->failed = true;							\
		SBIUNIT_INFO(test, "Condition \"" #cond "\" expected to be true!\n");	\
	}										\
} while (0)

#define SBIUNIT_ASSERT(test, cond) do {						\
	if (!(cond))								\
		SBIUNIT_PANIC(test, "Condition \"" #cond "\" must be true!\n");	\
} while (0)

#define SBIUNIT_EXPECT_EQ(test, a, b) SBIUNIT_EXPECT(test, (a) == (b))
#define SBIUNIT_ASSERT_EQ(test, a, b) SBIUNIT_ASSERT(test, (a) == (b))
#define SBIUNIT_EXPECT_NE(test, a, b) SBIUNIT_EXPECT(test, (a) != (b))
#define SBIUNIT_ASSERT_NE(test, a, b) SBIUNIT_ASSERT(test, (a) != (b))
#define SBIUNIT_EXPECT_MEMEQ(test, a, b, len) SBIUNIT_EXPECT(test, !sbi_memcmp(a, b, len))
#define SBIUNIT_ASSERT_MEMEQ(test, a, b, len) SBIUNIT_ASSERT(test, !sbi_memcmp(a, b, len))
#define SBIUNIT_EXPECT_STREQ(test, a, b, len) SBIUNIT_EXPECT(test, !sbi_strncmp(a, b, len))
#define SBIUNIT_ASSERT_STREQ(test, a, b, len) SBIUNIT_ASSERT(test, !sbi_strncmp(a, b, len))

void run_all_tests(void);
#endif
#else
#define run_all_tests()
#endif
