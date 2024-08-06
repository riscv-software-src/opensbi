#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2024 Ventana Micro Systems Inc.
#
# Authors:
#   Anup Patel <apatel@ventanamicro.com>
#

libsbiutils-objs-$(CONFIG_FDT_MAILBOX) += mailbox/fdt_mailbox.o
libsbiutils-objs-$(CONFIG_FDT_MAILBOX) += mailbox/fdt_mailbox_drivers.carray.o

libsbiutils-objs-$(CONFIG_MAILBOX) += mailbox/mailbox.o

libsbiutils-objs-$(CONFIG_RPMI_MAILBOX) += mailbox/rpmi_mailbox.o

carray-fdt_mailbox_drivers-$(CONFIG_FDT_MAILBOX_RPMI_SHMEM) += fdt_mailbox_rpmi_shmem
libsbiutils-objs-$(CONFIG_FDT_MAILBOX_RPMI_SHMEM) += mailbox/fdt_mailbox_rpmi_shmem.o
