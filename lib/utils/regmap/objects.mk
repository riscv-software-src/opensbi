#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2023 Ventana Micro Systems Inc.
#
# Authors:
#   Anup Patel <apatel@ventanamicro.com>
#

libsbiutils-objs-$(CONFIG_FDT_REGMAP) += regmap/fdt_regmap.o
libsbiutils-objs-$(CONFIG_FDT_REGMAP) += regmap/fdt_regmap_drivers.carray.o

carray-fdt_regmap_drivers-$(CONFIG_FDT_REGMAP_SYSCON) += fdt_regmap_syscon
libsbiutils-objs-$(CONFIG_FDT_REGMAP_SYSCON) += regmap/fdt_regmap_syscon.o

libsbiutils-objs-$(CONFIG_REGMAP) += regmap/regmap.o
