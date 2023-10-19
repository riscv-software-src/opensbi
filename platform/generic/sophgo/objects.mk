#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (C) 2023 Inochi Amaoto <inochiama@outlook.com>
# Copyright (C) 2023 Alibaba Group Holding Limited.
#

carray-platform_override_modules-$(CONFIG_PLATFORM_SOPHGO_SG2042) += sophgo_sg2042
platform-objs-$(CONFIG_PLATFORM_SOPHGO_SG2042) += sophgo/sg2042.o
