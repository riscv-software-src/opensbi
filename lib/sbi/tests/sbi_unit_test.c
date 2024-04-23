/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Author: Ivan Orlov <ivan.orlov0322@gmail.com>
 */
#include <sbi/sbi_unit_test.h>
#include <sbi/sbi_types.h>
#include <sbi/sbi_console.h>

extern struct sbiunit_test_suite *sbi_unit_tests[];
extern unsigned long sbi_unit_tests_size;

static void run_test_suite(struct sbiunit_test_suite *suite)
{
	struct sbiunit_test_case *s_case;
	u32 count_pass = 0, count_fail = 0;

	sbi_printf("## Running test suite: %s\n", suite->name);

	if (suite->init)
		suite->init();

	s_case = suite->cases;
	while (s_case->test_func) {
		s_case->test_func(s_case);
		if (s_case->failed)
			count_fail++;
		else
			count_pass++;
		sbi_printf("[%s] %s\n", s_case->failed ? "FAILED" : "PASSED",
			   s_case->name);
		s_case++;
	}
	sbi_printf("%u PASSED / %u FAILED / %u TOTAL\n", count_pass, count_fail,
		   count_pass + count_fail);
}

void run_all_tests(void)
{
	u32 i;

	sbi_printf("\n# Running SBIUNIT tests #\n");

	for (i = 0; i < sbi_unit_tests_size; i++)
		run_test_suite(sbi_unit_tests[i]);
}
