#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2019 Western Digital Corporation or its affiliates.
#
# Authors:
#   Atish Patra<atish.patra@wdc.com>
#

libfdt_files = fdt.o fdt_ro.o fdt_wip.o fdt_rw.o fdt_sw.o fdt_strerror.o \
               fdt_empty_tree.o
$(foreach file, $(libfdt_files), \
        $(eval CFLAGS_$(file) = -I$(src)/../../common/libfdt))

platform-common-objs-$(PLATFORM_INCLUDE_LIBFDT) += $(addprefix libfdt/,$(libfdt_files))
platform-common-genflags-$(PLATFORM_INCLUDE_LIBFDT) += -I$(platform_common_src_dir)/libfdt/
