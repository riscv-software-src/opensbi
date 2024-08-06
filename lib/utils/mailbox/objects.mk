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
