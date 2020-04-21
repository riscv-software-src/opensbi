#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2020 Nuclei Corporation or its affiliates.
#
# Authors:
#   lujun <lujun@nucleisys.com>
#

PLATFORM_RISCV_XLEN = 64
PLATFORM_RISCV_ISA = rv64imac
# Compiler flags
platform-cppflags-y = -g3
platform-cflags-y = -g3
platform-asflags-y = -g3
platform-ldflags-y = -g3

# Command for platform specific "make run"
platform-runcmd = xl_spike \
  $(build_dir)/platform/nuclei/ux600/firmware/fw_payload.elf

# Blobs to build
FW_TEXT_START=0x80000000
FW_DYNAMIC=y
FW_JUMP=y
ifeq ($(PLATFORM_RISCV_XLEN), 32)
  # This needs to be 4MB aligned for 32-bit system
  FW_JUMP_ADDR=0x80400000
else
  # This needs to be 2MB aligned for 64-bit system
  FW_JUMP_ADDR=0x80200000
endif
FW_JUMP_FDT_ADDR=0x88000000
FW_PAYLOAD=y
ifeq ($(PLATFORM_RISCV_XLEN), 32)
  # This needs to be 4MB aligned for 32-bit system
  FW_PAYLOAD_OFFSET=0x400000
else
  # This needs to be 2MB aligned for 64-bit system
  FW_PAYLOAD_OFFSET=0x200000
endif
FW_PAYLOAD_FDT_ADDR=0x88000000
FW_PAYLOAD_FDT=nuclei_ux600.dtb
