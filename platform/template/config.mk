#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2019 Western Digital Corporation or its affiliates.
#

# Compiler pre-processor flags
platform-cppflags-y =

# C Compiler and assembler flags.
platform-cflags-y =
platform-asflags-y =

# Linker flags: additional libraries and object files that the platform
# code needs can be added here
platform-ldflags-y =

#
# Command for platform specific "make run"
# Useful for development and debugging on plaftform simulator (such as QEMU)
#
# platform-runcmd = your_platform_run.sh

#
# Platform RISC-V XLEN, ABI, ISA and Code Model configuration.
# These are optional parameters but platforms can optionaly provide it.
# Some of these are guessed based on GCC compiler capabilities
#
# PLATFORM_RISCV_XLEN = 64
# PLATFORM_RISCV_ABI = lp64
# PLATFORM_RISCV_ISA = rv64imafdc
# PLATFORM_RISCV_CODE_MODEL = medany

#
# OpenSBI implements generic drivers for some common generic hardware. The
# drivers currently available are the RISC-V Platform Level Interrupt
# Controller (PLIC), RISC-V Core Local Interrupt controller (CLINT) and a UART
# 8250 compliant serial line driver (UART8250). The following definitions allow
# enabling the use of these generic drivers for the platform.
#
# PLATFORM_IRQCHIP_PLIC=<y|n>
# PLATFORM_SYS_CLINT=<y|n>
# PLATFORM_SERIAL_UART8250=<y|n>

# Firmware load address configuration. This is mandatory.
FW_TEXT_START=0x80000000

#
# Jump firmware configuration.
# Optional parameters are commented out. Uncomment and define these parameters
# as needed.
#
FW_JUMP=<y|n>
# This needs to be 4MB aligned for 32-bit support
# This needs to be 2MB aligned for 64-bit support
# ifeq ($(PLATFORM_RISCV_XLEN), 32)
# FW_JUMP_ADDR=0x80400000
# else
# FW_JUMP_ADDR=0x80200000
# endif
# FW_JUMP_FDT_ADDR=0x82200000

#
# Firmware with payload configuration.
# Optional parameters are commented out. Uncomment and define these parameters
# as needed.
#
FW_PAYLOAD=<y|n>
# This needs to be 4MB aligned for 32-bit support
# This needs to be 2MB aligned for 64-bit support
ifeq ($(PLATFORM_RISCV_XLEN), 32)
FW_PAYLOAD_OFFSET=0x400000
else
FW_PAYLOAD_OFFSET=0x200000
endif
# FW_PAYLOAD_ALIGN=0x1000
# FW_PAYLOAD_PATH="path to next boot stage binary image file"
# FW_PAYLOAD_FDT_PATH="path to platform flattened device tree file"
# FW_PAYLOAD_FDT="name of the platform defined flattened device tree file"
# FW_PAYLOAD_FDT_ADDR=0x82200000

#
# Allow linking against static libc for standard functions (memset, memcpy, etc)
#
# PLATFORM_INCLUDE_LIBC=y

