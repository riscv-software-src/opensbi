#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2019 Western Digital Corporation or its affiliates.
#
# Authors:
#   Atish Patra <atish.patra@wdc.com>
#

platform-objs-y += platform.o
ifdef FU540_ENABLED_HART_MASK
platform-genflags-y += -DFU540_ENABLED_HART_MASK=$(FU540_ENABLED_HART_MASK)
endif
