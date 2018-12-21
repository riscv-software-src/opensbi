#
# Copyright (c) 2018 Western Digital Corporation or its affiliates.
#
# Authors:
#   Anup Patel <anup.patel@wdc.com>
#
# SPDX-License-Identifier: BSD-2-Clause
#

firmware-cppflags-y =
firmware-cflags-y =
firmware-asflags-y =
firmware-ldflags-y =

ifdef FW_TEXT_START
firmware-cppflags-y += -DFW_TEXT_START=$(FW_TEXT_START)
endif

firmware-bins-$(FW_JUMP) += fw_jump.bin
ifdef FW_JUMP_ADDR
firmware-cppflags-$(FW_JUMP) += -DFW_JUMP_ADDR=$(FW_JUMP_ADDR)
endif
ifdef FW_JUMP_FDT_ADDR
firmware-cppflags-$(FW_JUMP) += -DFW_JUMP_FDT_ADDR=$(FW_JUMP_FDT_ADDR)
endif

firmware-bins-$(FW_PAYLOAD) += fw_payload.bin
ifdef FW_PAYLOAD_PATH
firmware-cppflags-$(FW_PAYLOAD) += -DFW_PAYLOAD_PATH=$(FW_PAYLOAD_PATH)
endif
ifdef FW_PAYLOAD_OFFSET
firmware-cppflags-$(FW_PAYLOAD) += -DFW_PAYLOAD_OFFSET=$(FW_PAYLOAD_OFFSET)
endif
ifdef FW_PAYLOAD_FDT_PATH
firmware-cppflags-$(FW_PAYLOAD) += -DFW_PAYLOAD_FDT_PATH=$(FW_PAYLOAD_FDT_PATH)
endif
ifdef FW_PAYLOAD_FDT_ADDR
firmware-cppflags-$(FW_PAYLOAD) += -DFW_PAYLOAD_FDT_ADDR=$(FW_PAYLOAD_FDT_ADDR)
endif
