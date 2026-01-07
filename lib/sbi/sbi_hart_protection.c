/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 Ventana Micro Systems Inc.
 */

#include <sbi/sbi_error.h>
#include <sbi/sbi_hart_protection.h>
#include <sbi/sbi_scratch.h>

static SBI_LIST_HEAD(hart_protection_list);

struct sbi_hart_protection *sbi_hart_protection_best(void)
{
	if (sbi_list_empty(&hart_protection_list))
		return NULL;

	return sbi_list_first_entry(&hart_protection_list, struct sbi_hart_protection, head);
}

int sbi_hart_protection_register(struct sbi_hart_protection *hprot)
{
	struct sbi_hart_protection *pos = NULL;
	bool found_pos = false;

	if (!hprot)
		return SBI_EINVAL;

	sbi_list_for_each_entry(pos, &hart_protection_list, head) {
		if (hprot->rating > pos->rating) {
			found_pos = true;
			break;
		}
	}

	if (found_pos)
		sbi_list_add_tail(&hprot->head, &pos->head);
	else
		sbi_list_add_tail(&hprot->head, &hart_protection_list);

	return 0;
}

void sbi_hart_protection_unregister(struct sbi_hart_protection *hprot)
{
	if (!hprot)
		return;

	sbi_list_del(&hprot->head);
}

int sbi_hart_protection_configure(struct sbi_scratch *scratch)
{
	struct sbi_hart_protection *hprot = sbi_hart_protection_best();

	if (!hprot)
		return 0;
	if (!hprot->configure)
		return SBI_ENOSYS;

	return hprot->configure(scratch);
}

void sbi_hart_protection_unconfigure(struct sbi_scratch *scratch)
{
	struct sbi_hart_protection *hprot = sbi_hart_protection_best();

	if (!hprot || !hprot->unconfigure)
		return;

	hprot->unconfigure(scratch);
}

int sbi_hart_protection_map_range(unsigned long base, unsigned long size)
{
	struct sbi_hart_protection *hprot = sbi_hart_protection_best();

	if (!hprot || !hprot->map_range)
		return 0;

	return hprot->map_range(sbi_scratch_thishart_ptr(), base, size);
}

int sbi_hart_protection_unmap_range(unsigned long base, unsigned long size)
{
	struct sbi_hart_protection *hprot = sbi_hart_protection_best();

	if (!hprot || !hprot->unmap_range)
		return 0;

	return hprot->unmap_range(sbi_scratch_thishart_ptr(), base, size);
}
