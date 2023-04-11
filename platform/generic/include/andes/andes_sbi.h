// SPDX-License-Identifier: BSD-2-Clause

#ifndef _RISCV_ANDES_SBI_H
#define _RISCV_ANDES_SBI_H

#include <sbi/sbi_trap.h>
#include <sbi_utils/fdt/fdt_helper.h>

int andes_sbi_vendor_ext_provider(long funcid,
				  const struct sbi_trap_regs *regs,
				  unsigned long *out_value,
				  struct sbi_trap_info *out_trap,
				  const struct fdt_match *match);

#endif /* _RISCV_ANDES_SBI_H */
