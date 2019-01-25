#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2019 Western Digital Corporation or its affiliates.
#
# Authors:
#   Atish Patra <atish.patra@wdc.com>
#

ifdef PLATFORM_ENABLED_HART_MASK
platform-genflags-y += -DPLATFORM_ENABLED_HART_MASK=$(PLATFORM_ENABLED_HART_MASK)
endif

