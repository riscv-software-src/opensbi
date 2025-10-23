#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2020 Western Digital Corporation or its affiliates.
#

carray-platform_override_modules-$(CONFIG_PLATFORM_OPENHWGROUP_OPENPITON) += openhwgroup_openpiton
platform-objs-$(CONFIG_PLATFORM_OPENHWGROUP_OPENPITON) += openhwgroup/openpiton.o

carray-platform_override_modules-$(CONFIG_PLATFORM_OPENHWGROUP_ARIANE) += openhwgroup_ariane
platform-objs-$(CONFIG_PLATFORM_OPENHWGROUP_ARIANE) += openhwgroup/ariane.o
