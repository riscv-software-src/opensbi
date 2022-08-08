#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2020 Western Digital Corporation or its affiliates.
#
# Authors:
#   Anup Patel <anup.patel@wdc.com>
#

libsbiutils-objs-$(CONFIG_IPI_MSWI) += ipi/aclint_mswi.o

libsbiutils-objs-$(CONFIG_FDT_IPI) += ipi/fdt_ipi.o
libsbiutils-objs-$(CONFIG_FDT_IPI) += ipi/fdt_ipi_drivers.o

carray-fdt_ipi_drivers-$(CONFIG_FDT_IPI_MSWI) += fdt_ipi_mswi
libsbiutils-objs-$(CONFIG_FDT_IPI_MSWI) += ipi/fdt_ipi_mswi.o
