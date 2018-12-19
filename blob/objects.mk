#
# Copyright (c) 2018 Western Digital Corporation or its affiliates.
#
# Authors:
#   Anup Patel <anup.patel@wdc.com>
#
# SPDX-License-Identifier: BSD-2-Clause
#

blob-cppflags-y =
blob-cflags-y =
blob-asflags-y =
blob-ldflags-y =

blob-bins-$(FW_JUMP) += fw_jump.bin
ifdef FW_JUMP_ADDR
blob-cppflags-$(FW_JUMP) += -DFW_JUMP_ADDR=$(FW_JUMP_ADDR)
endif
ifdef FW_JUMP_FDT_OFFSET
blob-cppflags-$(FW_JUMP) += -DFW_JUMP_FDT_OFFSET=$(FW_JUMP_FDT_OFFSET)
endif

blob-bins-$(FW_PAYLOAD) += fw_payload.bin
ifdef FW_PAYLOAD_PATH
blob-cppflags-$(FW_PAYLOAD) += -DFW_PAYLOAD_PATH=$(FW_PAYLOAD_PATH)
endif
ifdef FW_PAYLOAD_OFFSET
blob-cppflags-$(FW_PAYLOAD) += -DFW_PAYLOAD_OFFSET=$(FW_PAYLOAD_OFFSET)
endif
ifdef FW_PAYLOAD_FDT_OFFSET
blob-cppflags-$(FW_PAYLOAD) += -DFW_PAYLOAD_FDT_OFFSET=$(FW_PAYLOAD_FDT_OFFSET)
endif
