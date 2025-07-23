#
# SPDX-License-Identifier: BSD-2-Clause
#

ifeq ($(PLATFORM_RISCV_XLEN), 64)
carray-platform_override_modules-$(CONFIG_PLATFORM_MIPS_P8700) += mips_p8700
platform-objs-$(CONFIG_PLATFORM_MIPS_P8700) += mips/p8700.o mips/mips_warm_boot.o
endif
