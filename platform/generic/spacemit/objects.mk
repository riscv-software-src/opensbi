#
# SPDX-License-Identifier: BSD-2-Clause
#

ifeq ($(PLATFORM_RISCV_XLEN), 64)
platform-objs-$(CONFIG_PLATFORM_SPACEMIT) += spacemit/spacemit.o
carray-platform_override_modules-$(CONFIG_PLATFORM_SPACEMIT_K1) += spacemit_k1
platform-objs-$(CONFIG_PLATFORM_SPACEMIT_K1) += spacemit/k1.o
carray-platform_override_modules-$(CONFIG_PLATFORM_SPACEMIT_K3) += spacemit_k3
platform-objs-$(CONFIG_PLATFORM_SPACEMIT_K3) += spacemit/k3.o spacemit/k3_asm.o
endif
