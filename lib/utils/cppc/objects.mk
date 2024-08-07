#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2024 Ventana Micro Systems Inc.
#
# Authors:
#   Anup Patel <apatel@ventanamicro.com>
#

libsbiutils-objs-$(CONFIG_FDT_CPPC) += cppc/fdt_cppc.o
libsbiutils-objs-$(CONFIG_FDT_CPPC) += cppc/fdt_cppc_drivers.carray.o

carray-fdt_cppc_drivers-$(CONFIG_FDT_CPPC_RPMI) += fdt_cppc_rpmi
libsbiutils-objs-$(CONFIG_FDT_CPPC_RPMI) += cppc/fdt_cppc_rpmi.o
