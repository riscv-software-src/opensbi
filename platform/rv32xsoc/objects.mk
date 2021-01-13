#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2019 Western Digital Corporation or its affiliates.
#

# Space separated list of object file names to be compiled for the platform
platform-objs-y += platform.o

#
# If the platform support requires a builtin device tree file, the name of
# the device tree compiled file should be specified here. The device tree
# source file be in the form <dt file name>.dts
#
# platform-objs-y += <dt file name>.o
