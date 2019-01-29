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
# If we know the compillers xlen use it below
ifeq ($(OPENSBI_CC_XLEN), 32)
	platform-cflags-y =-mabi=ilp$(OPENSBI_CC_XLEN) -march=rv$(OPENSBI_CC_XLEN)imafdc
	platform-asflags-y =-mabi=ilp$(OPENSBI_CC_XLEN) -march=rv$(OPENSBI_CC_XLEN)imafdc
endif
ifeq ($(OPENSBI_CC_XLEN), 64)
	platform-cflags-y =-mabi=lp$(OPENSBI_CC_XLEN) -march=rv$(OPENSBI_CC_XLEN)imafdc
	platform-asflags-y =-mabi=lp$(OPENSBI_CC_XLEN) -march=rv$(OPENSBI_CC_XLEN)imafdc
endif
platform-cflags-y +=  -mcmodel=medany
platform-asflags-y += -mcmodel=medany
platform-ldflags-y =

# Common drivers to enable
PLATFORM_IRQCHIP_PLIC=y
PLATFORM_SERIAL_UART8250=y
PLATFORM_SYS_CLINT=y

# Blobs to build
FW_TEXT_START=0x80000000
FW_JUMP=y
# This needs to be 4MB alligned for 32-bit support
FW_JUMP_ADDR=0x80400000
FW_JUMP_FDT_ADDR=0x82200000
FW_PAYLOAD=y
# This needs to be 4MB alligned for 32-bit support
FW_PAYLOAD_OFFSET=0x400000
FW_PAYLOAD_FDT_ADDR=0x82200000

# External Libraries to include
PLATFORM_INCLUDE_LIBC=y
