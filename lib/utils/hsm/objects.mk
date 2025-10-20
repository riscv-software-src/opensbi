#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2024 Ventana Micro Systems Inc.
#
# Authors:
#   Anup Patel <apatel@ventanamicro.com>
#

carray-fdt_early_drivers-$(CONFIG_FDT_HSM_RPMI) += fdt_hsm_rpmi
libsbiutils-objs-$(CONFIG_FDT_HSM_RPMI) += hsm/fdt_hsm_rpmi.o

carray-fdt_early_drivers-$(CONFIG_FDT_HSM_SPACEMIT) += fdt_hsm_spacemit
libsbiutils-objs-$(CONFIG_FDT_HSM_SPACEMIT) += hsm/fdt_hsm_spacemit.o

carray-fdt_early_drivers-$(CONFIG_FDT_HSM_SIFIVE_TMC0) += fdt_hsm_sifive_tmc0
libsbiutils-objs-$(CONFIG_FDT_HSM_SIFIVE_TMC0) += hsm/fdt_hsm_sifive_tmc0.o