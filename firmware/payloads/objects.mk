#
# Copyright (c) 2018 Western Digital Corporation or its affiliates.
#
# Authors:
#   Anup Patel <anup.patel@wdc.com>
#
# SPDX-License-Identifier: BSD-2-Clause
#

firmware-bins-$(FW_PAYLOAD) += payloads/test.bin

test-y += test_head.o
test-y += test_main.o

%/test.o: $(foreach obj,$(test-y),%/$(obj))
	$(call merge_objs,$@,$^)

%/test.dep: $(foreach dep,$(test-y:.o=.dep),%/$(dep))
	$(call merge_deps,$@,$^)
