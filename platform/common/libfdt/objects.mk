#
# Copyright (c) 2018 Western Digital Corporation or its affiliates.
#
# Authors:
#   Atish Patra<atish.patra@wdc.com>
#
# SPDX-License-Identifier: BSD-2-Clause
#

libfdt_files = fdt.o fdt_ro.o fdt_wip.o fdt_rw.o fdt_sw.o fdt_strerror.o \
               fdt_empty_tree.o
$(foreach file, $(libfdt_files), \
        $(eval CFLAGS_$(file) = -I$(src)/../../common/libfdt))

platform-common-objs-$(PLATFORM_INCLUDE_LIBFDT) += $(addprefix libfdt/,$(libfdt_files))
