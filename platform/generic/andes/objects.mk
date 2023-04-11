#
# SPDX-License-Identifier: BSD-2-Clause
#

carray-platform_override_modules-$(CONFIG_PLATFORM_ANDES_AE350) += andes_ae350
platform-objs-$(CONFIG_PLATFORM_ANDES_AE350) += andes/ae350.o andes/sleep.o

platform-objs-$(CONFIG_ANDES45_PMA) += andes/andes45-pma.o
platform-objs-$(CONFIG_ANDES_SBI) += andes/andes_sbi.o
