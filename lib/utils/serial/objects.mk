#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2019 Western Digital Corporation or its affiliates.
#
# Authors:
#   Anup Patel <anup.patel@wdc.com>
#

libsbiutils-objs-y += serial/fdt_serial.o
libsbiutils-objs-y += serial/fdt_serial_drivers.o

carray-fdt_serial_drivers-y += fdt_serial_gaisler
libsbiutils-objs-y += serial/fdt_serial_gaisler.o

carray-fdt_serial_drivers-y += fdt_serial_htif
libsbiutils-objs-y += serial/fdt_serial_htif.o

carray-fdt_serial_drivers-y += fdt_serial_shakti
libsbiutils-objs-y += serial/fdt_serial_shakti.o

carray-fdt_serial_drivers-y += fdt_serial_sifive
libsbiutils-objs-y += serial/fdt_serial_sifive.o

carray-fdt_serial_drivers-y += fdt_serial_litex
libsbiutils-objs-y += serial/fdt_serial_litex.o

carray-fdt_serial_drivers-y += fdt_serial_uart8250
libsbiutils-objs-y += serial/fdt_serial_uart8250.o

carray-fdt_serial_drivers-y += fdt_serial_xlnx_uartlite
libsbiutils-objs-y += serial/fdt_serial_xlnx_uartlite.o

libsbiutils-objs-y += serial/gaisler-uart.o
libsbiutils-objs-y += serial/shakti-uart.o
libsbiutils-objs-y += serial/sifive-uart.o
libsbiutils-objs-y += serial/litex-uart.o
libsbiutils-objs-y += serial/uart8250.o
libsbiutils-objs-y += serial/xlnx-uartlite.o
