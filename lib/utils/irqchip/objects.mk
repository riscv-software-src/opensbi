#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2019 Western Digital Corporation or its affiliates.
#
# Authors:
#   Anup Patel <anup.patel@wdc.com>
#

libsbiutils-objs-y += irqchip/fdt_irqchip.o
libsbiutils-objs-y += irqchip/fdt_irqchip_drivers.o

carray-fdt_irqchip_drivers-y += fdt_irqchip_aplic
libsbiutils-objs-y += irqchip/fdt_irqchip_aplic.o

carray-fdt_irqchip_drivers-y += fdt_irqchip_imsic
libsbiutils-objs-y += irqchip/fdt_irqchip_imsic.o

carray-fdt_irqchip_drivers-y += fdt_irqchip_plic
libsbiutils-objs-y += irqchip/fdt_irqchip_plic.o

libsbiutils-objs-y += irqchip/aplic.o
libsbiutils-objs-y += irqchip/imsic.o
libsbiutils-objs-y += irqchip/plic.o
