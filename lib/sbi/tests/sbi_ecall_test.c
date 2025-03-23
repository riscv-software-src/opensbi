#include <sbi/sbi_unit_test.h>
#include <sbi/sbi_ecall.h>
#include <sbi/sbi_ecall_interface.h>

static void test_sbi_ecall_version(struct sbiunit_test_case *test)
{
	SBIUNIT_EXPECT_EQ(test, sbi_ecall_version_major(), SBI_ECALL_VERSION_MAJOR);
	SBIUNIT_EXPECT_EQ(test, sbi_ecall_version_minor(), SBI_ECALL_VERSION_MINOR);
}

static void test_sbi_ecall_impid(struct sbiunit_test_case *test)
{
	unsigned long old_impid = sbi_ecall_get_impid();
	sbi_ecall_set_impid(42);
	SBIUNIT_EXPECT_EQ(test, sbi_ecall_get_impid(), 42);
	sbi_ecall_set_impid(old_impid);
}

static int dummy_handler(unsigned long extid, unsigned long funcid,
			 struct sbi_trap_regs *regs,
			 struct sbi_ecall_return *out)
{
	return 0;
}

static void test_sbi_ecall_register_find_extension(struct sbiunit_test_case *test)
{
	struct sbi_ecall_extension test_ext = {
		/* Use experimental extension space for no overlap */
		.extid_start = SBI_EXT_EXPERIMENTAL_START,
		.extid_end = SBI_EXT_EXPERIMENTAL_START,
		.name = "TestExt",
		.handle = dummy_handler,
	};

	SBIUNIT_EXPECT_EQ(test, sbi_ecall_register_extension(&test_ext), 0);
	SBIUNIT_EXPECT_EQ(test, sbi_ecall_find_extension(SBI_EXT_EXPERIMENTAL_START), &test_ext);

	sbi_ecall_unregister_extension(&test_ext);
	SBIUNIT_EXPECT_EQ(test, sbi_ecall_find_extension(SBI_EXT_EXPERIMENTAL_START), NULL);
}

static struct sbiunit_test_case ecall_tests[] = {
	SBIUNIT_TEST_CASE(test_sbi_ecall_version),
	SBIUNIT_TEST_CASE(test_sbi_ecall_impid),
	SBIUNIT_TEST_CASE(test_sbi_ecall_register_find_extension),
	SBIUNIT_END_CASE,
};

SBIUNIT_TEST_SUITE(ecall_test_suite, ecall_tests);
