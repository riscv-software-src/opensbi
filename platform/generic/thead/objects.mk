#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (C) 2023 Inochi Amaoto <inochiama@outlook.com>
# Copyright (C) 2023 Alibaba Group Holding Limited.
#

platform-objs-$(CONFIG_THEAD_C9XX_PMU) += thead/thead_c9xx_pmu.o

platform-objs-$(CONFIG_THEAD_C9XX_ERRATA) += thead/thead_c9xx_tlb_trap_handler.o
platform-objs-$(CONFIG_THEAD_C9XX_ERRATA) += thead/thead_c9xx_errata_tlb_flush.o

carray-platform_override_modules-$(CONFIG_PLATFORM_THEAD) += thead_generic
platform-objs-$(CONFIG_PLATFORM_THEAD) += thead/thead-generic.o
