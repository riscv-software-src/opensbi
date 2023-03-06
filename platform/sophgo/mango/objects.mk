#
# SPDX-License-Identifier: BSD-2-Clause
#
MANGO_PLATFORM_PLD = n
MANGO_COLD_BOOT = y
MANGO_C920 = y
MANGO_DVM = n


MANGO_CFLAGS = -DMANGO -Iplatform/sophgo/mango

ifeq ($(strip $(MANGO_PLATFORM_PLD)), y)
MANGO_CFLAGS += -DMANGO_PLATFORM_PLD

# boot hart
MANGO_COLD_BOOT_A0 = 0
# dtb load address
MANGO_COLD_BOOT_A1 = 0x20000000

MANGO_CFLAGS += -DMANGO_COLD_BOOT_A0=$(MANGO_COLD_BOOT_A0)
MANGO_CFLAGS += -DMANGO_COLD_BOOT_A1=$(MANGO_COLD_BOOT_A1)
endif


ifeq ($(strip $(MANGO_COLD_BOOT)), y)
MANGO_CFLAGS += -DMANGO_COLD_BOOT

ifeq ($(strip $(MANGO_C920)), y)
MANGO_CFLAGS += -DMANGO_C920
endif

ifeq ($(strip $(MANGO_DVM)), y)
MANGO_CFLAGS += -DMANGO_DVM
endif
endif

# Compiler flags
platform-cppflags-y =
platform-cflags-y = $(MANGO_CFLAGS)
platform-asflags-y = $(MANGO_CFLAGS)
platform-ldflags-y =

# Blobs to build
FW_TEXT_START?=0x0
FW_JUMP=y
FW_JUMP_ADDR?=0x00200000

platform-objs-y += platform.o
