#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2019 Western Digital Corporation or its affiliates.
#
# Authors:
#   Atish Patra<atish.patra@wdc.com>
#

libc_files = string.o

$(foreach file, $(libc_files), \
	$(eval CFLAGS_$(file) = -I$(src)/../../common/libc))

platform-common-objs-$(PLATFORM_INCLUDE_LIBC) += $(addprefix libc/,$(libc_files))
