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
	$(eval CFLAGS_$(file) = -I$(src)/../../sbi/libc))

libsbi-objs-y += $(addprefix libc/,$(libc_files))
