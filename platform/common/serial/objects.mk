#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2019 Western Digital Corporation or its affiliates.
#
# Authors:
#   Anup Patel <anup.patel@wdc.com>
#

platform-common-objs-$(PLATFORM_SERIAL_UART8250) += serial/uart8250.o
platform-common-objs-$(PLATFORM_SERIAL_SIFIVE_UART) += serial/sifive-uart.o
