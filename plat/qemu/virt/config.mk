#
# Copyright (c) 2018 Western Digital Corporation or its affiliates.
#
# Authors:
#   Anup Patel <anup.patel@wdc.com>
#
# SPDX-License-Identifier: BSD-2-Clause
#

# Essential defines required by SBI platform
plat-cppflags-y = -DPLAT_NAME="QEMU Virt Machine"
plat-cppflags-y+= -DPLAT_HART_COUNT=8
plat-cppflags-y+= -DPLAT_HART_STACK_SIZE=8192
plat-cppflags-y+= -DPLAT_TEXT_START=0x80000000

# Compiler flags
plat-cflags-y =-mabi=lp64 -march=rv64imafdc -mcmodel=medany
plat-asflags-y =-mabi=lp64 -march=rv64imafdc -mcmodel=medany
plat-ldflags-y =

# Common drivers to enable
PLAT_IRQCHIP_PLIC=y
PLAT_SERIAL_UART8250=y
PLAT_SYS_CLINT=y

# Blobs to build
FW_JUMP=y
FW_JUMP_ADDR=0x80200000
FW_JUMP_FDT_OFFSET=0x2000000
FW_PAYLOAD=y
FW_PAYLOAD_FDT_OFFSET=0x2000000
