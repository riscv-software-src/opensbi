libsbi-objs-$(CONFIG_SBIUNIT) += tests/sbi_unit_test.o
libsbi-objs-$(CONFIG_SBIUNIT) += tests/sbi_unit_tests.o

carray-sbi_unit_tests-$(CONFIG_SBIUNIT) += bitmap_test_suite
libsbi-objs-$(CONFIG_SBIUNIT) += tests/sbi_bitmap_test.o

carray-sbi_unit_tests-$(CONFIG_SBIUNIT) += console_test_suite
libsbi-objs-$(CONFIG_SBIUNIT) += tests/sbi_console_test.o
