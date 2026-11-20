/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Check macro of the tests. The function that uses it has a counter named
 * failed.
 */

#ifndef __SBITEST_CHECK_H__
#define __SBITEST_CHECK_H__

#include <sbi/sbi_console.h>

#define CHECK(cond)                                                          \
	do {                                                                 \
		if (!(cond)) {                                               \
			sbi_printf("FAILED %s:%d: %s\n", __func__, __LINE__, \
				   #cond);                                   \
			failed++;                                            \
		}                                                            \
	} while (0)

#endif /* __SBITEST_CHECK_H__ */
