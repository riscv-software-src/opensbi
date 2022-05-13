#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2020 Western Digital Corporation or its affiliates.
#
# Authors:
#   Anup Patel <anup.patel@wdc.com>
#

libsbiutils-objs-y += timer/aclint_mtimer.o

libsbiutils-objs-y += timer/fdt_timer.o
libsbiutils-objs-y += timer/fdt_timer_drivers.o

carray-fdt_timer_drivers-y += fdt_timer_mtimer
libsbiutils-objs-y += timer/fdt_timer_mtimer.o
