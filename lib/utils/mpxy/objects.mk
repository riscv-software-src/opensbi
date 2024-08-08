#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2024 Ventana Micro Systems Inc.
#
# Authors:
#   Anup Patel <apatel@ventanamicro.com>
#

libsbiutils-objs-$(CONFIG_FDT_MPXY) += mpxy/fdt_mpxy.o
libsbiutils-objs-$(CONFIG_FDT_MPXY) += mpxy/fdt_mpxy_drivers.carray.o

carray-fdt_mpxy_drivers-$(CONFIG_FDT_MPXY_RPMI_MBOX) += fdt_mpxy_rpmi_mbox
libsbiutils-objs-$(CONFIG_FDT_MPXY_RPMI_MBOX) += mpxy/fdt_mpxy_rpmi_mbox.o
