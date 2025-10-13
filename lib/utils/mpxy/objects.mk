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

carray-fdt_mpxy_drivers-$(CONFIG_FDT_MPXY_RPMI_PERFORMANCE) += fdt_mpxy_rpmi_performance
libsbiutils-objs-$(CONFIG_FDT_MPXY_RPMI_PERFORMANCE) += mpxy/fdt_mpxy_rpmi_performance.o

carray-fdt_mpxy_drivers-$(CONFIG_FDT_MPXY_RPMI_SYSMSI) += fdt_mpxy_rpmi_sysmsi
libsbiutils-objs-$(CONFIG_FDT_MPXY_RPMI_SYSMSI) += mpxy/fdt_mpxy_rpmi_sysmsi.o

carray-fdt_mpxy_drivers-$(CONFIG_FDT_MPXY_RPMI_VOLTAGE) += fdt_mpxy_rpmi_voltage
libsbiutils-objs-$(CONFIG_FDT_MPXY_RPMI_VOLTAGE) += mpxy/fdt_mpxy_rpmi_voltage.o

carray-fdt_mpxy_drivers-$(CONFIG_FDT_MPXY_RPMI_DEVICE_POWER) += fdt_mpxy_rpmi_device_power
libsbiutils-objs-$(CONFIG_FDT_MPXY_RPMI_DEVICE_POWER) += mpxy/fdt_mpxy_rpmi_device_power.o
