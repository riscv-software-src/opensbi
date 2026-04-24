/*
 * SPDX-FileCopyrightText: (c) 2025-2026 Tenstorrent USA, Inc.
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sbi/riscv_asm.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_csr_detect.h>

#include <tenstorrent/ascalon.h>
#include <tenstorrent/pma.h>

#define CSR_PMACFG0	0x7e0

void tt_ascalon_discover_pmas_from_boot_hart(void)
{
	struct sbi_trap_info trap = {0};

	/* Whisper virtual platform does not implement PMA */
	csr_read_allowed(CSR_PMACFG0, &trap);
	if (trap.cause)
		return;

	for (unsigned int i = 0; i < TT_MAX_PMAS; i++) {
		u64 pma = csr_read_num(CSR_PMACFG0 + i);
		if (!tt_pma_validate(i, pma)) {
			sbi_printf("HART%d: Bad boot PMA%02d 0x%016lx\n",
				   current_hartid(), i, pma);
		}
		tt_pma_set(i, pma);

		if (pma)
			tt_pma_print(i, pma);
	}
}

void tt_ascalon_verify_pmas_nonboot_hart(void)
{
	struct sbi_trap_info trap = {0};

	/* Whisper virtual platform does not implement PMA */
	csr_read_allowed(CSR_PMACFG0, &trap);
	if (trap.cause)
		return;

	for (unsigned int i = 0; i < TT_MAX_PMAS; i++) {
		u64 pma = csr_read_num(CSR_PMACFG0 + i);
		if (pma != tt_pma_get(i)) {
			sbi_printf("HART%d: Bad boot PMA%02d 0x%016lx does not match boot HART\n",
				   current_hartid(), i, pma);
		}
	}
}
