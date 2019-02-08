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

# If you always know these you can set them here, otherwise we will use the compiler defaults
PLATFORM_RISCV_XLEN=64
PLATFORM_HAS_C_EXT=y

# Linker flags: additional libraries and object files that the platform
# code needs can be added here
platform-ldflags-y =

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
FW_JUMP_ADDR=0x80200000
# FW_JUMP_FDT_ADDR=0x82200000

#
# Firmware with payload configuration.
# Optional parameters are commented out. Uncomment and define these parameters
# as needed.
#
FW_PAYLOAD=<y|n>
FW_PAYLOAD_OFFSET=0x200000
# FW_PAYLOAD_ALIGN=0x1000
# FW_PAYLOAD_PATH="path to next boot stage binary image file"
# FW_PAYLOAD_FDT_PATH="path to platform flattened device tree file"
# FW_PAYLOAD_FDT="name of the platform defined flattened device tree file"
# FW_PAYLOAD_FDT_ADDR=0x82200000

#
# Allow linking against static libc for standard functions (memset, memcpy, etc)
#
# PLATFORM_INCLUDE_LIBC=y

