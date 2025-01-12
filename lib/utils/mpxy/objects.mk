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

libsbiutils-objs-$(CONFIG_FDT_MPXY_RPMI_MBOX) += mpxy/fdt_mpxy_rpmi_mbox.o

carray-fdt_mpxy_drivers-$(CONFIG_FDT_MPXY_RPMI_CLOCK) += fdt_mpxy_rpmi_clock
libsbiutils-objs-$(CONFIG_FDT_MPXY_RPMI_CLOCK) += mpxy/fdt_mpxy_rpmi_clock.o

carray-fdt_mpxy_drivers-$(CONFIG_FDT_MPXY_RPMI_SYSMSI) += fdt_mpxy_rpmi_sysmsi
libsbiutils-objs-$(CONFIG_FDT_MPXY_RPMI_SYSMSI) += mpxy/fdt_mpxy_rpmi_sysmsi.o
