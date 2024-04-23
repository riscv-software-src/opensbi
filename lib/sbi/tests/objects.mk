libsbi-objs-$(CONFIG_SBIUNIT) += tests/sbi_unit_test.o
libsbi-objs-$(CONFIG_SBIUNIT) += tests/sbi_unit_tests.o

carray-sbi_unit_tests-$(CONFIG_SBIUNIT) += bitmap_test_suite
libsbi-objs-$(CONFIG_SBIUNIT) += tests/sbi_bitmap_test.o

carray-sbi_unit_tests-$(CONFIG_SBIUNIT) += console_test_suite
libsbi-objs-$(CONFIG_SBIUNIT) += tests/sbi_console_test.o

carray-sbi_unit_tests-$(CONFIG_SBIUNIT) += atomic_test_suite
libsbi-objs-$(CONFIG_SBIUNIT) += tests/riscv_atomic_test.o

carray-sbi_unit_tests-$(CONFIG_SBIUNIT) += locks_test_suite
libsbi-objs-$(CONFIG_SBIUNIT) += tests/riscv_locks_test.o
