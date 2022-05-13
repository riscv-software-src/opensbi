#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2020 Western Digital Corporation or its affiliates.
#
# Authors:
#   Anup Patel <anup.patel@wdc.com>
#

libsbiutils-objs-y += reset/fdt_reset.o
libsbiutils-objs-y += reset/fdt_reset_drivers.o

carray-fdt_reset_drivers-y += fdt_poweroff_gpio
carray-fdt_reset_drivers-y += fdt_reset_gpio
libsbiutils-objs-y += reset/fdt_reset_gpio.o

carray-fdt_reset_drivers-y += fdt_reset_htif
libsbiutils-objs-y += reset/fdt_reset_htif.o

carray-fdt_reset_drivers-y += fdt_reset_sifive_test
libsbiutils-objs-y += reset/fdt_reset_sifive_test.o

carray-fdt_reset_drivers-y += fdt_reset_sunxi_wdt
libsbiutils-objs-y += reset/fdt_reset_sunxi_wdt.o

carray-fdt_reset_drivers-y += fdt_reset_thead
libsbiutils-objs-y += reset/fdt_reset_thead.o
libsbiutils-objs-y += reset/fdt_reset_thead_asm.o
