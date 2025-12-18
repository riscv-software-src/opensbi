#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (C) 2025 Bo Gan <ganboing@gmail.com>
#

ifeq ($(PLATFORM_RISCV_XLEN), 64)
carray-platform_override_modules-$(CONFIG_PLATFORM_ESWIN_EIC770X) += eswin_eic7700
platform-objs-$(CONFIG_PLATFORM_ESWIN_EIC770X) += eswin/eic770x.o
platform-objs-$(CONFIG_PLATFORM_ESWIN_EIC770X) += eswin/hfp.o
endif
