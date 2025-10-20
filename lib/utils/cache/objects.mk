#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2025 SiFive
#

libsbiutils-objs-$(CONFIG_FDT_CACHE) += cache/fdt_cache.o
libsbiutils-objs-$(CONFIG_FDT_CACHE) += cache/fdt_cache_drivers.carray.o

libsbiutils-objs-$(CONFIG_CACHE) += cache/cache.o
