#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2019 Western Digital Corporation or its affiliates.
#
# Authors:
#   Damien Le Moal <damien.lemoal@wdc.com>
#

# Compiler flags
platform-cppflags-y =
platform-cflags-y =
platform-asflags-y =
platform-ldflags-y =

# Common drivers to enable
PLATFORM_IRQCHIP_PLIC=y
PLATFORM_SYS_CLINT=y

# Blobs to build
FW_TEXT_START=0x80000000
FW_PAYLOAD=y
FW_PAYLOAD_ALIGN=0x1000
FW_PAYLOAD_FDT=k210.dtb

# External Libraries to include
PLATFORM_INCLUDE_LIBC=y

