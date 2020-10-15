#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2019 Western Digital Corporation or its affiliates.
#
# Authors:
#   Damien Le Moal <damien.lemoal@wdc.com>
#

platform-objs-y += platform.o

platform-objs-y += k210.o
platform-varprefix-k210.o = dt_k210
platform-padding-k210.o = 2048
