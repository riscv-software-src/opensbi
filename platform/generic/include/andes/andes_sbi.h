// SPDX-License-Identifier: BSD-2-Clause

#ifndef _RISCV_ANDES_SBI_H
#define _RISCV_ANDES_SBI_H

#include <sbi/sbi_ecall.h>
#include <sbi/sbi_trap.h>

int andes_sbi_vendor_ext_provider(long funcid,
				  struct sbi_trap_regs *regs,
				  struct sbi_ecall_return *out);

#endif /* _RISCV_ANDES_SBI_H */
