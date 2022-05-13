#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2020 Western Digital Corporation or its affiliates.
#
# Authors:
#   Anup Patel <anup.patel@wdc.com>
#

platform-objs-y += platform.o
platform-objs-y += platform_override_modules.o

carray-platform_override_modules-y += sifive_fu540
platform-objs-y += sifive_fu540.o

carray-platform_override_modules-y += sifive_fu740
platform-objs-y += sifive_fu740.o
