#
# SPDX-License-Identifier: BSD-2-Clause
#

ifeq ($(PLATFORM_RISCV_XLEN), 64)
carray-platform_override_modules-$(CONFIG_PLATFORM_MIPS_P8700_EYEQ7H) += mips_p8700_eyeq7h
carray-platform_override_modules-$(CONFIG_PLATFORM_MIPS_P8700_BOSTON) += mips_p8700_boston
platform-objs-$(CONFIG_CPU_MIPS_P8700) += mips/p8700.o mips/mips_warm_boot.o
platform-objs-$(CONFIG_PLATFORM_MIPS_P8700_EYEQ7H) += mips/eyeq7h.o
platform-objs-$(CONFIG_PLATFORM_MIPS_P8700_BOSTON) += mips/boston.o
endif
