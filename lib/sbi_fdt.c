/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 HardenedLinux.
 *
 * Authors:
 *   Xiang Wang<wxjstz@126.com>
 *
 */
#include <libfdt/libfdt.h>
#include <sbi/riscv_atomic.h>
#include <sbi/riscv_barrier.h>
#include <sbi/sbi_types.h>
#include <sbi/sbi_scratch.h>

static uint64_t next_addr;
static atomic_t done = ATOMIC_INITIALIZER(0);

int sbi_fdt_update_next_addr(struct sbi_scratch *scratch, bool coolboot)
{
	if (coolboot) {
		do {
			void *fdt = sbi_scratch_thishart_arg1_ptr();
			int chosen_offset = fdt_path_offset(fdt, "/chosen");
			if (chosen_offset < 0)
				break;
			int len;
			const void *prop =
			    fdt_getprop(fdt, chosen_offset, "opensbi,next_addr",
					&len);
			if (prop == NULL)
				break;
			if (len == 4)
				next_addr = fdt32_ld(prop);
			if (len == 8)
				next_addr = fdt64_ld(prop);
		} while (0);
		atomic_write(&done, 1);
	}

	while (atomic_read(&done) == 0) {
		/* reduce memory bus load and let coolboot complete faster */
		asm volatile("nop" ::: "memory");
		asm volatile("nop" ::: "memory");
	};

	smp_mb();

	if (next_addr)
		scratch->next_addr = next_addr;
	return 0;
}
