#
# SPDX-FileCopyrightText: (c) 2025-2026 Tenstorrent USA, Inc.
# SPDX-License-Identifier: BSD-2-Clause
#

ifeq ($(PLATFORM_RISCV_XLEN), 64)
platform-objs-y += tenstorrent/pma.o
platform-objs-$(CONFIG_CPU_TENSTORRENT_ASCALON) += tenstorrent/ascalon.o

carray-platform_override_modules-$(CONFIG_PLATFORM_TENSTORRENT_ATLANTIS) += tenstorrent_atlantis
platform-objs-$(CONFIG_PLATFORM_TENSTORRENT_ATLANTIS) += tenstorrent/atlantis.o
endif
