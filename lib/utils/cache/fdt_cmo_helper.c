/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 SiFive Inc.
 */

#include <libfdt.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_scratch.h>
#include <sbi_utils/cache/fdt_cache.h>
#include <sbi_utils/cache/fdt_cmo_helper.h>
#include <sbi_utils/fdt/fdt_helper.h>

static unsigned long flc_offset;

#define get_hart_flc(_s) \
	sbi_scratch_read_type(_s, struct cache_device *, flc_offset)
#define set_hart_flc(_s, _p) \
	sbi_scratch_write_type(_s, struct cache_device *, flc_offset, _p)

int fdt_cmo_private_flc_flush_all(void)
{
	struct cache_device *flc = get_hart_flc(sbi_scratch_thishart_ptr());

	if (!flc || !flc->cpu_private)
		return SBI_ENODEV;

	return cache_flush_all(flc);
}

int fdt_cmo_llc_flush_all(void)
{
	struct cache_device *llc = get_hart_flc(sbi_scratch_thishart_ptr());

	if (!llc)
		return SBI_ENODEV;

	while (llc->next)
		llc = llc->next;

	return cache_flush_all(llc);
}

static int fdt_cmo_cold_init(const void *fdt)
{
	struct sbi_scratch *scratch;
	struct cache_device *dev;
	int cpu_offset, cpus_offset, rc;
	u32 hartid;

	cpus_offset = fdt_path_offset(fdt, "/cpus");
	if (cpus_offset < 0)
		return SBI_EINVAL;

	fdt_for_each_subnode(cpu_offset, fdt, cpus_offset) {
		rc = fdt_parse_hart_id(fdt, cpu_offset, &hartid);
		if (rc)
			continue;

		scratch = sbi_hartid_to_scratch(hartid);
		if (!scratch)
			continue;

		rc = fdt_next_cache_get(fdt, cpu_offset, &dev);
		if (rc && rc != SBI_ENOENT)
			return rc;
		if (rc == SBI_ENOENT)
			dev = NULL;

		set_hart_flc(scratch, dev);
	}

	return SBI_OK;
}

static int fdt_cmo_warm_init(void)
{
	struct cache_device *cur = get_hart_flc(sbi_scratch_thishart_ptr());
	int rc;

	while (cur) {
		if (cur->ops && cur->ops->warm_init) {
			rc = cur->ops->warm_init(cur);
			if (rc)
				return rc;
		}

		cur = cur->next;
	}

	return SBI_OK;
}

int fdt_cmo_init(bool cold_boot)
{
	const void *fdt = fdt_get_address();
	int rc;

	if (cold_boot) {
		flc_offset = sbi_scratch_alloc_type_offset(struct cache_device *);
		if (!flc_offset)
			return SBI_ENOMEM;

		rc = fdt_cmo_cold_init(fdt);
		if (rc)
			return rc;
	}

	rc = fdt_cmo_warm_init();
	if (rc)
		return rc;

	return SBI_OK;
}
