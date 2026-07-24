#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (C) 2026 Metanoia Communication Inc.
#

ifeq ($(PLATFORM_RISCV_XLEN), 64)
carray-platform_override_modules-$(CONFIG_PLATFORM_METANOIA_MT2824) += metanoia_mt2824
platform-objs-$(CONFIG_PLATFORM_METANOIA_MT2824) += metanoia/mt2824/mt2824.o
endif
