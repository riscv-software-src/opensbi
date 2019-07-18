#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2019 Andes Technology Corporation
#
# Authors:
#   Zong Li <zong@andestech.com>
#   Nylon Chen <nylon7@andestech.com>

# Compiler flags
platform-cppflags-y =
platform-cflags-y =
platform-asflags-y =
platform-ldflags-y =

# Blobs to build
FW_TEXT_START=0x00000000

FW_DYNAMIC=y

FW_JUMP=y
ifeq ($(PLATFORM_RISCV_XLEN), 32)
  FW_JUMP_ADDR=0x400000
else
  FW_JUMP_ADDR=0x200000
endif
FW_JUMP_FDT_ADDR=0x2000000

FW_PAYLOAD=y
ifeq ($(PLATFORM_RISCV_XLEN), 32)
  FW_PAYLOAD_OFFSET=0x400000
else
  FW_PAYLOAD_OFFSET=0x200000
endif

FW_PAYLOAD_FDT_ADDR=0x2000000
