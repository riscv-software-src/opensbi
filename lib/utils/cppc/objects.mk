#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2024 Ventana Micro Systems Inc.
#
# Authors:
#   Anup Patel <apatel@ventanamicro.com>
#

carray-fdt_early_drivers-$(CONFIG_FDT_CPPC_RPMI) += fdt_cppc_rpmi
libsbiutils-objs-$(CONFIG_FDT_CPPC_RPMI) += cppc/fdt_cppc_rpmi.o
