#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2021 Western Digital Corporation or its affiliates.
#
# Authors:
#   Anup Patel <anup.patel@wdc.com>
#

libsbiutils-objs-$(CONFIG_FDT_GPIO) += gpio/fdt_gpio.o
libsbiutils-objs-$(CONFIG_FDT_GPIO) += gpio/fdt_gpio_drivers.carray.o

carray-fdt_gpio_drivers-$(CONFIG_FDT_GPIO_DESIGNWARE) += fdt_gpio_designware
libsbiutils-objs-$(CONFIG_FDT_GPIO_DESIGNWARE) += gpio/fdt_gpio_designware.o

carray-fdt_gpio_drivers-$(CONFIG_FDT_GPIO_SIFIVE) += fdt_gpio_sifive
libsbiutils-objs-$(CONFIG_FDT_GPIO_SIFIVE) += gpio/fdt_gpio_sifive.o

carray-fdt_gpio_drivers-$(CONFIG_FDT_GPIO_STARFIVE) += fdt_gpio_starfive
libsbiutils-objs-$(CONFIG_FDT_GPIO_STARFIVE) += gpio/fdt_gpio_starfive.o

libsbiutils-objs-$(CONFIG_GPIO) += gpio/gpio.o
