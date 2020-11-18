// SPDX-License-Identifier: BSD-2-Clause
/*
 * fdt_domain.c - Flat Device Tree Domain helper routines
 *
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <libfdt.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_hartmask.h>
#include <sbi/sbi_scratch.h>
#include <sbi_utils/fdt/fdt_domain.h>
#include <sbi_utils/fdt/fdt_helper.h>

void fdt_iterate_each_domain(void *fdt, void *opaque,
			     void (*fn)(void *fdt, int domain_offset,
					void *opaque))
{
	int doffset, poffset;

	if (!fdt || !fn)
		return;

	poffset = fdt_path_offset(fdt, "/chosen");
	if (poffset < 0)
		return;
	poffset = fdt_node_offset_by_compatible(fdt, poffset,
						"opensbi,domain,config");
	if (poffset < 0)
		return;

	fdt_for_each_subnode(doffset, fdt, poffset) {
		if (fdt_node_check_compatible(fdt, doffset,
					      "opensbi,domain,instance"))
			continue;

		fn(fdt, doffset, opaque);
	}
}

void fdt_iterate_each_memregion(void *fdt, int domain_offset, void *opaque,
				void (*fn)(void *fdt, int domain_offset,
					   int region_offset, u32 region_access,
					   void *opaque))
{
	u32 i, rcount;
	int len, region_offset;
	const u32 *regions;

	if (!fdt || (domain_offset < 0) || !fn)
		return;

	if (fdt_node_check_compatible(fdt, domain_offset,
				      "opensbi,domain,instance"))
		return;

	regions = fdt_getprop(fdt, domain_offset, "regions", &len);
	if (!regions)
		return;

	rcount = (u32)len / (sizeof(u32) * 2);
	for (i = 0; i < rcount; i++) {
		region_offset = fdt_node_offset_by_phandle(fdt,
						fdt32_to_cpu(regions[2 * i]));
		if (region_offset < 0)
			continue;

		if (fdt_node_check_compatible(fdt, region_offset,
					      "opensbi,domain,memregion"))
			continue;

		fn(fdt, domain_offset, region_offset,
		   fdt32_to_cpu(regions[(2 * i) + 1]), opaque);
	}
}

struct __fixup_find_domain_offset_info {
	const char *name;
	int *doffset;
};

static void __fixup_find_domain_offset(void *fdt, int doff, void *p)
{
	struct __fixup_find_domain_offset_info *fdo = p;

	if (sbi_strcmp(fdo->name, fdt_get_name(fdt, doff, NULL)))
		return;

	*fdo->doffset = doff;
}

#define DISABLE_DEVICES_MASK	(SBI_DOMAIN_MEMREGION_READABLE | \
				 SBI_DOMAIN_MEMREGION_WRITEABLE | \
				 SBI_DOMAIN_MEMREGION_EXECUTABLE)

static void __fixup_count_disable_devices(void *fdt, int doff, int roff,
					  u32 perm, void *p)
{
	int len;
	u32 *dcount = p;

	if (perm & DISABLE_DEVICES_MASK)
		return;

	len = 0;
	if (fdt_getprop(fdt, roff, "devices", &len))
		*dcount += len / sizeof(u32);
}

static void __fixup_disable_devices(void *fdt, int doff, int roff,
				    u32 raccess, void *p)
{
	int i, len, coff;
	const u32 *devices;

	if (raccess & DISABLE_DEVICES_MASK)
		return;

	len = 0;
	devices = fdt_getprop(fdt, roff, "devices", &len);
	if (!devices)
		return;
	len = len / sizeof(u32);

	for (i = 0; i < len; i++) {
		coff = fdt_node_offset_by_phandle(fdt,
					fdt32_to_cpu(devices[i]));
		if (coff < 0)
			continue;

		fdt_setprop_string(fdt, coff, "status", "disabled");
	}
}

void fdt_domain_fixup(void *fdt)
{
	u32 i, dcount;
	int err, poffset, doffset;
	struct sbi_domain *dom = sbi_domain_thishart_ptr();
	struct __fixup_find_domain_offset_info fdo;

	/* Remove the domain assignment DT property from CPU DT nodes */
	poffset = fdt_path_offset(fdt, "/cpus");
	if (poffset < 0)
		return;
	fdt_for_each_subnode(doffset, fdt, poffset) {
		err = fdt_parse_hart_id(fdt, doffset, &i);
		if (err)
			continue;

		fdt_nop_property(fdt, doffset, "opensbi-domain");
	}

	/* Skip device disable for root domain */
	if (!dom->index)
		goto skip_device_disable;

	/* Find current domain DT node */
	doffset = -1;
	fdo.name = dom->name;
	fdo.doffset = &doffset;
	fdt_iterate_each_domain(fdt, &fdo, __fixup_find_domain_offset);
	if (doffset < 0)
		goto skip_device_disable;

	/* Count current domain device DT nodes to be disabled */
	dcount = 0;
	fdt_iterate_each_memregion(fdt, doffset, &dcount,
				   __fixup_count_disable_devices);
	if (!dcount)
		goto skip_device_disable;

	/* Expand FDT based on device DT nodes to be disabled */
	err = fdt_open_into(fdt, fdt, fdt_totalsize(fdt) + dcount * 32);
	if (err < 0)
		return;

	/* Again find current domain DT node */
	doffset = -1;
	fdo.name = dom->name;
	fdo.doffset = &doffset;
	fdt_iterate_each_domain(fdt, &fdo, __fixup_find_domain_offset);
	if (doffset < 0)
		goto skip_device_disable;

	/* Disable device DT nodes for current domain */
	fdt_iterate_each_memregion(fdt, doffset, NULL,
				   __fixup_disable_devices);
skip_device_disable:

	/* Remove the OpenSBI domain config DT node */
	poffset = fdt_path_offset(fdt, "/chosen");
	if (poffset < 0)
		return;
	poffset = fdt_node_offset_by_compatible(fdt, poffset,
						"opensbi,domain,config");
	if (poffset < 0)
		return;
	fdt_nop_node(fdt, poffset);
}

static struct sbi_domain *fdt_hartid_to_domain[SBI_HARTMASK_MAX_BITS];

#define FDT_DOMAIN_MAX_COUNT		8
#define FDT_DOMAIN_REGION_MAX_COUNT	16

static u32 fdt_domains_count;
static struct sbi_domain fdt_domains[FDT_DOMAIN_MAX_COUNT];
static struct sbi_hartmask fdt_masks[FDT_DOMAIN_MAX_COUNT];
static struct sbi_domain_memregion
	fdt_regions[FDT_DOMAIN_MAX_COUNT][FDT_DOMAIN_REGION_MAX_COUNT + 2];

struct sbi_domain *fdt_domain_get(u32 hartid)
{
	if (SBI_HARTMASK_MAX_BITS <= hartid)
		return NULL;
	return fdt_hartid_to_domain[hartid];
}

static void __fdt_parse_region(void *fdt, int domain_offset,
			       int region_offset, u32 region_access,
			       void *opaque)
{
	int len;
	u32 val32;
	u64 val64;
	const u32 *val;
	u32 *region_count = opaque;
	struct sbi_domain_memregion *region;

	/* Find next region of the domain */
	if (FDT_DOMAIN_REGION_MAX_COUNT <= *region_count)
		return;
	region = &fdt_regions[fdt_domains_count][*region_count];

	/* Read "base" DT property */
	val = fdt_getprop(fdt, region_offset, "base", &len);
	if (!val && len >= 8)
		return;
	val64 = fdt32_to_cpu(val[0]);
	val64 = (val64 << 32) | fdt32_to_cpu(val[1]);
	region->base = val64;

	/* Read "order" DT property */
	val = fdt_getprop(fdt, region_offset, "order", &len);
	if (!val && len >= 4)
		return;
	val32 = fdt32_to_cpu(*val);
	if (val32 < 3 || __riscv_xlen < val32)
		return;
	region->order = val32;

	/* Read "mmio" DT property */
	region->flags = region_access & SBI_DOMAIN_MEMREGION_ACCESS_MASK;
	if (fdt_get_property(fdt, region_offset, "mmio", NULL))
		region->flags |= SBI_DOMAIN_MEMREGION_MMIO;

	(*region_count)++;
}

static void __fdt_parse_domain(void *fdt, int domain_offset, void *opaque)
{
	u32 val32;
	u64 val64;
	const u32 *val;
	struct sbi_domain *dom;
	struct sbi_hartmask *mask;
	int i, err, len, cpu_offset;
	int *cold_domain_offset = opaque;
	struct sbi_domain_memregion *regions;

	/* Sanity check on maximum domains we can handle */
	if (FDT_DOMAIN_MAX_COUNT <= fdt_domains_count)
		return;
	dom = &fdt_domains[fdt_domains_count];
	mask = &fdt_masks[fdt_domains_count];
	regions = &fdt_regions[fdt_domains_count][0];

	/* Read DT node name */
	sbi_strncpy(dom->name, fdt_get_name(fdt, domain_offset, NULL),
		    sizeof(dom->name));
	dom->name[sizeof(dom->name) - 1] = '\0';

	/* Setup possible HARTs mask */
	SBI_HARTMASK_INIT(mask);
	dom->possible_harts = mask;
	val = fdt_getprop(fdt, domain_offset, "possible-harts", &len);
	len = len / sizeof(u32);
	if (val && len) {
		for (i = 0; i < len; i++) {
			cpu_offset = fdt_node_offset_by_phandle(fdt,
							fdt32_to_cpu(val[i]));
			if (cpu_offset < 0)
				continue;

			err = fdt_parse_hart_id(fdt, cpu_offset, &val32);
			if (err)
				continue;

			sbi_hartmask_set_hart(val32, mask);
		}
	}

	/* Setup memregions from DT */
	val32 = 0;
	sbi_memset(regions, 0,
		   sizeof(*regions) * (FDT_DOMAIN_REGION_MAX_COUNT + 2));
	dom->regions = regions;
	fdt_iterate_each_memregion(fdt, domain_offset, &val32,
				   __fdt_parse_region);
	sbi_domain_memregion_initfw(&regions[val32]);

	/* Read "boot-hart" DT property */
	val32 = -1U;
	val = fdt_getprop(fdt, domain_offset, "boot-hart", &len);
	if (val && len >= 4) {
		cpu_offset = fdt_node_offset_by_phandle(fdt,
							 fdt32_to_cpu(*val));
		if (cpu_offset >= 0)
			fdt_parse_hart_id(fdt, cpu_offset, &val32);
	} else {
		if (domain_offset == *cold_domain_offset)
			val32 = current_hartid();
	}
	dom->boot_hartid = val32;

	/* Read "next-arg1" DT property */
	val64 = 0;
	val = fdt_getprop(fdt, domain_offset, "next-arg1", &len);
	if (val && len >= 8) {
		val64 = fdt32_to_cpu(val[0]);
		val64 = (val64 << 32) | fdt32_to_cpu(val[1]);
	} else {
		if (domain_offset == *cold_domain_offset)
			val64 = sbi_scratch_thishart_ptr()->next_arg1;
	}
	dom->next_arg1 = val64;

	/* Read "next-addr" DT property */
	val64 = 0;
	val = fdt_getprop(fdt, domain_offset, "next-addr", &len);
	if (val && len >= 8) {
		val64 = fdt32_to_cpu(val[0]);
		val64 = (val64 << 32) | fdt32_to_cpu(val[1]);
	} else {
		if (domain_offset == *cold_domain_offset)
			val64 = sbi_scratch_thishart_ptr()->next_addr;
	}
	dom->next_addr = val64;

	/* Read "next-mode" DT property */
	val32 = 0x1;
	val = fdt_getprop(fdt, domain_offset, "next-mode", &len);
	if (val && len >= 4) {
		val32 = fdt32_to_cpu(val[0]);
		if (val32 != 0x0 && val32 != 0x1)
			val32 = 0x1;
	} else {
		if (domain_offset == *cold_domain_offset)
			val32 = sbi_scratch_thishart_ptr()->next_mode;
	}
	dom->next_mode = val32;

	/* Read "system-reset-allowed" DT property */
	if (fdt_get_property(fdt, domain_offset,
			     "system-reset-allowed", NULL))
		dom->system_reset_allowed = TRUE;
	else
		dom->system_reset_allowed = FALSE;

	/* Increment domains count */
	fdt_domains_count++;
}

int fdt_domains_populate(void *fdt)
{
	const u32 *val;
	int cold_domain_offset;
	u32 i, hartid, cold_hartid;
	int err, len, cpus_offset, cpu_offset, domain_offset;

	/* Sanity checks */
	if (!fdt)
		return SBI_EINVAL;

	/* Find /cpus DT node */
	cpus_offset = fdt_path_offset(fdt, "/cpus");
	if (cpus_offset < 0)
		return cpus_offset;

	/* Find coldboot HART domain DT node offset */
	cold_domain_offset = -1;
	cold_hartid = current_hartid();
	fdt_for_each_subnode(cpu_offset, fdt, cpus_offset) {
		err = fdt_parse_hart_id(fdt, cpu_offset, &hartid);
		if (err)
			continue;

		if (hartid != cold_hartid)
			continue;

		val = fdt_getprop(fdt, cpu_offset, "opensbi-domain", &len);
		if (val && len >= 4)
			cold_domain_offset = fdt_node_offset_by_phandle(fdt,
							   fdt32_to_cpu(*val));

		break;
	}

	/* Iterate over each domain in FDT and populate details */
	fdt_iterate_each_domain(fdt, &cold_domain_offset, __fdt_parse_domain);

	/* HART to domain assignment based on CPU DT nodes*/
	fdt_for_each_subnode(cpu_offset, fdt, cpus_offset) {
		err = fdt_parse_hart_id(fdt, cpu_offset, &hartid);
		if (err)
			continue;

		if (SBI_HARTMASK_MAX_BITS <= hartid)
			continue;

		val = fdt_getprop(fdt, cpu_offset, "opensbi-domain", &len);
		if (!val || len < 4)
			continue;

		domain_offset = fdt_node_offset_by_phandle(fdt,
							   fdt32_to_cpu(*val));
		if (domain_offset < 0)
			continue;

		for (i = 0; i < fdt_domains_count; i++) {
			if (!sbi_strcmp(fdt_domains[i].name,
				fdt_get_name(fdt, domain_offset, NULL))) {
				fdt_hartid_to_domain[hartid] = &fdt_domains[i];
				break;
			}
		}
	}

	return 0;
}
