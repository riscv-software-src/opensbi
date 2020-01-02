#
# SPDX-License-Identifier: BSD-2-Clause
#

# Compiler flags
platform-cppflags-y =
platform-cflags-y =
platform-asflags-y =
platform-ldflags-y =

# Blobs to build
FW_TEXT_START?=0x0
FW_JUMP=y
FW_JUMP_ADDR?=0x00200000
