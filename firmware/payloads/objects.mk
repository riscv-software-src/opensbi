#
# Copyright (c) 2018 Western Digital Corporation or its affiliates.
#
# Authors:
#   Anup Patel <anup.patel@wdc.com>
#
# SPDX-License-Identifier: BSD-2-Clause
#

firmware-bins-$(FW_PAYLOAD) += payloads/dummy.bin

dummy-y += dummy_head.o
dummy-y += dummy_main.o

%/dummy.o: $(foreach obj,$(dummy-y),%/$(obj))
	$(call merge_objs,$@,$^)

%/dummy.dep: $(foreach dep,$(dummy-y:.o=.dep),%/$(dep))
	$(call merge_deps,$@,$^)
