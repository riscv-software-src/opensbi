#include <sbi/sbi_unit_test.h>
#include <sbi/riscv_locks.h>

static spinlock_t test_lock = SPIN_LOCK_INITIALIZER;

static void spin_lock_test(struct sbiunit_test_case *test)
{
	/* We don't want to accidentally get locked */
	SBIUNIT_ASSERT(test, !spin_lock_check(&test_lock));

	spin_lock(&test_lock);
	SBIUNIT_EXPECT(test, spin_lock_check(&test_lock));
	spin_unlock(&test_lock);

	SBIUNIT_ASSERT(test, !spin_lock_check(&test_lock));
}

static void spin_trylock_fail(struct sbiunit_test_case *test)
{
	/* We don't want to accidentally get locked */
	SBIUNIT_ASSERT(test, !spin_lock_check(&test_lock));

	spin_lock(&test_lock);
	SBIUNIT_EXPECT(test, !spin_trylock(&test_lock));
	spin_unlock(&test_lock);
}

static void spin_trylock_success(struct sbiunit_test_case *test)
{
	SBIUNIT_EXPECT(test, spin_trylock(&test_lock));
	spin_unlock(&test_lock);
}

static struct sbiunit_test_case locks_test_cases[] = {
	SBIUNIT_TEST_CASE(spin_lock_test),
	SBIUNIT_TEST_CASE(spin_trylock_fail),
	SBIUNIT_TEST_CASE(spin_trylock_success),
	SBIUNIT_END_CASE,
};

SBIUNIT_TEST_SUITE(locks_test_suite, locks_test_cases);
