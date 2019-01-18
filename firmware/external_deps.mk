#
# Copyright (c) 2018 Western Digital Corporation or its affiliates.
#
# Authors:
#   Anup Patel <anup.patel@wdc.com>
#
# SPDX-License-Identifier: BSD-2-Clause
#

$(build_dir)/$(platform_subdir)/firmware/fw_payload.o: $(FW_PAYLOAD_PATH_FINAL)
$(build_dir)/$(platform_subdir)/firmware/fw_payload.o: $(FW_PAYLOAD_FDT_PATH)
