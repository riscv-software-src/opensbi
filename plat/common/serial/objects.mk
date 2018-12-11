#
# Copyright (c) 2018 Western Digital Corporation or its affiliates.
#
# Authors:
#   Anup Patel <anup.patel@wdc.com>
#
# SPDX-License-Identifier: BSD-2-Clause
#

plat-common-objs-$(PLAT_SERIAL_UART8250) += serial/uart8250.o
plat-common-objs-$(PLAT_SERIAL_SIFIVE_UART) += serial/sifive-uart.o
