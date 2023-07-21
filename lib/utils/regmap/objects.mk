#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2023 Ventana Micro Systems Inc.
#
# Authors:
#   Anup Patel <apatel@ventanamicro.com>
#

libsbiutils-objs-$(CONFIG_FDT_REGMAP) += regmap/fdt_regmap.o
libsbiutils-objs-$(CONFIG_FDT_REGMAP) += regmap/fdt_regmap_drivers.o

libsbiutils-objs-$(CONFIG_REGMAP) += regmap/regmap.o
