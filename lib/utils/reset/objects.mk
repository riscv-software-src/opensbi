#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2020 Western Digital Corporation or its affiliates.
#
# Authors:
#   Anup Patel <anup.patel@wdc.com>
#

carray-fdt_early_drivers-$(CONFIG_FDT_RESET_ATCWDT200) += fdt_reset_atcwdt200
libsbiutils-objs-$(CONFIG_FDT_RESET_ATCWDT200) += reset/fdt_reset_atcwdt200.o

carray-fdt_early_drivers-$(CONFIG_FDT_RESET_GPIO) += fdt_poweroff_gpio
carray-fdt_early_drivers-$(CONFIG_FDT_RESET_GPIO) += fdt_reset_gpio
libsbiutils-objs-$(CONFIG_FDT_RESET_GPIO) += reset/fdt_reset_gpio.o

carray-fdt_early_drivers-$(CONFIG_FDT_RESET_HTIF) += fdt_reset_htif
libsbiutils-objs-$(CONFIG_FDT_RESET_HTIF) += reset/fdt_reset_htif.o

carray-fdt_early_drivers-$(CONFIG_FDT_RESET_SG2042_HWMON_MCU) += fdt_reset_sg2042_mcu
libsbiutils-objs-$(CONFIG_FDT_RESET_SG2042_HWMON_MCU) += reset/fdt_reset_sg2042_hwmon_mcu.o

carray-fdt_early_drivers-$(CONFIG_FDT_RESET_SUNXI_WDT) += fdt_reset_sunxi_wdt
libsbiutils-objs-$(CONFIG_FDT_RESET_SUNXI_WDT) += reset/fdt_reset_sunxi_wdt.o

carray-fdt_early_drivers-$(CONFIG_FDT_RESET_SYSCON) += fdt_syscon_poweroff
carray-fdt_early_drivers-$(CONFIG_FDT_RESET_SYSCON) += fdt_syscon_reboot
libsbiutils-objs-$(CONFIG_FDT_RESET_SYSCON) += reset/fdt_reset_syscon.o

carray-fdt_early_drivers-$(CONFIG_FDT_RESET_RPMI) += fdt_reset_rpmi
libsbiutils-objs-$(CONFIG_FDT_RESET_RPMI) += reset/fdt_reset_rpmi.o
