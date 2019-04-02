#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2019 Western Digital Corporation or its affiliates.
#
# Authors:
#   Anup Patel <anup.patel@wdc.com>
#

lib-objs-y += riscv_asm.o
lib-objs-y += riscv_atomic.o
lib-objs-y += riscv_hardfp.o
lib-objs-y += riscv_locks.o

lib-objs-y += sbi_console.o
lib-objs-y += sbi_ecall.o
lib-objs-y += sbi_emulate_csr.o
lib-objs-y += sbi_fifo.o
lib-objs-y += sbi_hart.o
lib-objs-y += sbi_illegal_insn.o
lib-objs-y += sbi_init.o
lib-objs-y += sbi_ipi.o
lib-objs-y += sbi_misaligned_ldst.o
lib-objs-y += sbi_system.o
lib-objs-y += sbi_timer.o
lib-objs-y += sbi_trap.o

# External Libraries to include
PLATFORM_INCLUDE_LIBC=y
