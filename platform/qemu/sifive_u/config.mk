#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2019 Western Digital Corporation or its affiliates.
#
# Authors:
#   Anup Patel <anup.patel@wdc.com>
#

# Compiler flags
platform-cppflags-y =
platform-cflags-y =-mabi=lp64 -march=rv64imafdc -mcmodel=medany
platform-asflags-y =-mabi=lp64 -march=rv64imafdc -mcmodel=medany
platform-ldflags-y =

platform-runcmd = qemu-system-riscv64 -M sifive_u -m 256M -display none -serial stdio \
        -kernel build/platform/qemu/sifive_u/firmware/fw_payload.elf

# Common drivers to enable
PLATFORM_IRQCHIP_PLIC=y
PLATFORM_SERIAL_SIFIVE_UART=y
PLATFORM_SYS_CLINT=y

# Blobs to build
FW_TEXT_START=0x80000000
FW_JUMP=y
FW_JUMP_ADDR=0x80200000
FW_JUMP_FDT_ADDR=0x82200000
FW_PAYLOAD=y
FW_PAYLOAD_OFFSET=0x200000
FW_PAYLOAD_FDT_ADDR=0x82200000

# External Libraries to include
PLATFORM_INCLUDE_LIBC=y
