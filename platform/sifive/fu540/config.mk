#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2019 Western Digital Corporation or its affiliates.
#
# Authors:
#   Atish Patra <atish.patra@wdc.com>
#

# Compiler flags
platform-cppflags-y =
platform-cflags-y =
platform-asflags-y =
platform-ldflags-y =

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
PLATFORM_INCLUDE_LIBFDT=y
PLATFORM_INCLUDE_LIBC=y
