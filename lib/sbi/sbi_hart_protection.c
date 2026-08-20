/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 Ventana Micro Systems Inc.
 */

#include <sbi/sbi_console.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_hart_protection.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_string.h>

static SBI_LIST_HEAD(hart_protection_list);

static struct sbi_hart_protection *__hart_memory_protection_best(void)
{
	struct sbi_hart_protection *pos;

	sbi_list_for_each_entry(pos, &hart_protection_list, head) {
		if (pos->type == SBI_HART_PROTECTION_TYPE_MEMORY)
			return pos;
	}

	return NULL;
}

void sbi_hart_protection_get_str(char *out_str, int out_str_size)
{
	bool memory_protect_done = false;
	struct sbi_hart_protection *pos;
	int offset = 0;

	if (!out_str || out_str_size <= 0)
		return;
	sbi_memset(out_str, 0, out_str_size);

	sbi_list_for_each_entry(pos, &hart_protection_list, head) {
		if (pos->type == SBI_HART_PROTECTION_TYPE_MEMORY) {
			if (memory_protect_done)
				continue;
			memory_protect_done = true;
		}
		sbi_snprintf(out_str + offset, out_str_size - offset, "%s,", pos->name);
		offset = offset + sbi_strlen(pos->name) + 1;
	}

	if (offset)
		out_str[offset - 1] = '\0';
	else
		sbi_strncpy(out_str, "none", out_str_size);
}

int sbi_hart_protection_register(struct sbi_hart_protection *hprot)
{
	struct sbi_hart_protection *pos = NULL;
	bool found_pos = false;

	if (!hprot)
		return SBI_EINVAL;
	if (hprot->type >= SBI_HART_PROTECTION_TYPE_MAX)
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

static int __hart_protection_configure(struct sbi_scratch *scratch,
				       struct sbi_hart_protection *hprot,
				       struct sbi_domain *dom)
{
	if (!hprot)
		return 0;
	if (!hprot->configure)
		return SBI_ENOSYS;

	return hprot->configure(scratch, dom);
}

static void __hart_protection_unconfigure(struct sbi_scratch *scratch,
					  struct sbi_hart_protection *hprot,
					  struct sbi_domain *dom)
{
	if (!hprot || !hprot->unconfigure)
		return;

	hprot->unconfigure(scratch, dom);
}

int sbi_hart_protection_configure(struct sbi_scratch *scratch,
				  struct sbi_domain *dom)
{
	bool do_configure, memory_protect_done = false;
	struct sbi_hart_protection *hprot;
	int ret;

	sbi_list_for_each_entry(hprot, &hart_protection_list, head) {
		do_configure = false;
		switch (hprot->type) {
		case SBI_HART_PROTECTION_TYPE_MEMORY:
			do_configure = !memory_protect_done;
			memory_protect_done = true;
			break;
		case SBI_HART_PROTECTION_TYPE_ID:
			do_configure = true;
			break;
		default:
			break;
		}
		if (!do_configure)
			continue;

		ret = __hart_protection_configure(scratch, hprot, dom);
		if (ret)
			return ret;
	}

	return 0;
}

void sbi_hart_protection_unconfigure(struct sbi_scratch *scratch,
				     struct sbi_domain *dom)
{

	bool do_unconfigure, memory_protect_done = false;
	struct sbi_hart_protection *hprot;

	sbi_list_for_each_entry(hprot, &hart_protection_list, head) {
		do_unconfigure = false;
		switch (hprot->type) {
		case SBI_HART_PROTECTION_TYPE_MEMORY:
			do_unconfigure = !memory_protect_done;
			memory_protect_done = true;
			break;
		case SBI_HART_PROTECTION_TYPE_ID:
			do_unconfigure = true;
			break;
		default:
			break;
		}
		if (!do_unconfigure)
			continue;

		__hart_protection_unconfigure(scratch, hprot, dom);
	}
}

int sbi_hart_protection_reconfigure(struct sbi_scratch *scratch,
				    struct sbi_domain *current_dom,
				    struct sbi_domain *next_dom)
{
	bool do_reconfigure, memory_protect_done = false;
	struct sbi_hart_protection *hprot;
	int ret;

	sbi_list_for_each_entry(hprot, &hart_protection_list, head) {
		do_reconfigure = false;
		switch (hprot->type) {
		case SBI_HART_PROTECTION_TYPE_MEMORY:
			do_reconfigure = !memory_protect_done;
			memory_protect_done = true;
			break;
		case SBI_HART_PROTECTION_TYPE_ID:
			do_reconfigure = true;
			break;
		default:
			break;
		}
		if (!do_reconfigure)
			continue;

		__hart_protection_unconfigure(scratch, hprot, current_dom);
		ret = __hart_protection_configure(scratch, hprot, next_dom);
		if (ret)
			return ret;
	}

	return 0;
}

int sbi_hart_protection_temp_map_range(unsigned long base, unsigned long size)
{
	struct sbi_hart_protection *hprot = __hart_memory_protection_best();

	if (!hprot || !hprot->temp_map_range)
		return 0;

	return hprot->temp_map_range(sbi_scratch_thishart_ptr(), base, size);
}

int sbi_hart_protection_temp_unmap_range(unsigned long base, unsigned long size)
{
	struct sbi_hart_protection *hprot = __hart_memory_protection_best();

	if (!hprot || !hprot->temp_unmap_range)
		return 0;

	return hprot->temp_unmap_range(sbi_scratch_thishart_ptr(), base, size);
}
