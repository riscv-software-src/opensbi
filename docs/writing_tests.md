Writing tests for OpenSBI
=========================

SBIUnit
-------
SBIUnit is a set of macros and functions which simplify the test development and
automate the test execution and evaluation. All of the SBIUnit definitions are
in the `include/sbi/sbi_unit_test.h` header file, and implementations are
available in `lib/sbi/tests/sbi_unit_test.c`.

Simple SBIUnit test
-------------------

For instance, we would like to test the following function from
`lib/sbi/sbi_string.c`:

```c
size_t sbi_strlen(const char *str)
{
	unsigned long ret = 0;

	while (*str != '\0') {
		ret++;
		str++;
	}

	return ret;
}
```

which calculates the string length.

Create the file `lib/sbi/tests/sbi_string_test.c` with the following content:

```c
#include <sbi/sbi_unit_test.h>
#include <sbi/sbi_string.h>

static void strlen_test(struct sbiunit_test_case *test)
{
	SBIUNIT_EXPECT_EQ(test, sbi_strlen("Hello"), 5);
	SBIUNIT_EXPECT_EQ(test, sbi_strlen("Hell\0o"), 4);
}

static struct sbiunit_test_case string_test_cases[] = {
	SBIUNIT_TEST_CASE(strlen_test),
	SBIUNIT_END_CASE,
};

SBIUNIT_TEST_SUITE(string_test_suite, string_test_cases);
```

Then, add the corresponding Makefile entries to `lib/sbi/tests/objects.mk`:
```lang-makefile
...
carray-sbi_unit_tests-$(CONFIG_SBIUNIT) += string_test_suite
libsbi-objs-$(CONFIG_SBIUNIT) += tests/sbi_string_test.o
```

Now, run `make clean` in order to regenerate the carray-related files.

Recompile OpenSBI with the CONFIG_SBIUNIT option enabled and run it in QEMU.
You will see something like this:
```
# make PLATFORM=generic run
...
# Running SBIUNIT tests #
...
## Running test suite: string_test_suite
[PASSED] strlen_test
1 PASSED / 0 FAILED / 1 TOTAL
```

Now let's try to change this test in the way that it will fail:

```c
- SBIUNIT_EXPECT_EQ(test, sbi_strlen("Hello"), 5);
+ SBIUNIT_EXPECT_EQ(test, sbi_strlen("Hello"), 100);
```

`make all` and `make run` it again:
```
...
# Running SBIUNIT tests #
...
## Running test suite: string_test_suite
[SBIUnit] [.../opensbi/lib/sbi/tests/sbi_string_test.c:6]: strlen_test: Condition "(sbi_strlen("Hello")) == (100)" expected to be true!
[FAILED] strlen_test
0 PASSED / 1 FAILED / 1 TOTAL
```
Covering the static functions / using the static definitions
------------------------------------------------------------

SBIUnit also allows you to test static functions. In order to do so, simply
include your test source in the file you would like to test. Complementing the
example above, just add this to the `lib/sbi/sbi_string.c` file:

```c
#ifdef CONFIG_SBIUNIT
#include "tests/sbi_string_test.c"
#endif
```

In this case you should only add a new carray entry pointing to the test suite
to `lib/sbi/tests/objects.mk`:
```lang-makefile
...
carray-sbi_unit_tests-$(CONFIG_SBIUNIT) += string_test_suite
```

You don't have to compile the `sbi_string_test.o` separately, because the
test code will be included into the `sbi_string` object file.

"Mocking" the structures
------------------------
See the example of structure "mocking" in `lib/sbi/tests/sbi_console_test.c`,
where the sbi_console_device structure was mocked to be used in various
console-related functions in order to test them.

API Reference
-------------
All of the `SBIUNIT_EXPECT_*` macros will cause a test case to fail if the
corresponding conditions are not met, however, the execution of a particular
test case will not be stopped.

All of the `SBIUNIT_ASSERT_*` macros will cause a test case to fail and stop
immediately, triggering a panic.
