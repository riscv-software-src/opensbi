/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <libfdt.h>
#include <platform_override.h>
#include <sbi/riscv_asm.h>
#include <sbi/sbi_bitops.h>
#include <sbi/sbi_hartmask.h>
#include <sbi/sbi_heap.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_string.h>
#include <sbi/sbi_system.h>
#include <sbi/sbi_tlb.h>
#include <sbi_utils/cache/fdt_cmo_helper.h>
#include <sbi_utils/fdt/fdt_domain.h>
#include <sbi_utils/fdt/fdt_driver.h>
#include <sbi_utils/fdt/fdt_fixup.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/fdt/fdt_pmu.h>
#include <sbi_utils/irqchip/fdt_irqchip.h>
#include <sbi_utils/irqchip/imsic.h>
#include <sbi_utils/mpxy/fdt_mpxy.h>
#include <sbi_utils/serial/fdt_serial.h>
#include <sbi_utils/serial/semihosting.h>
#include <sbi_utils/timer/fdt_timer.h>

/* List of platform override modules generated at compile time */
extern const struct fdt_driver *const platform_override_modules[];

static u32 fw_platform_calculate_heap_size(u32 hart_count)
{
	u32 heap_size;

	heap_size = SBI_PLATFORM_DEFAULT_HEAP_SIZE(hart_count);

	/* For TLB fifo */
	heap_size += SBI_TLB_INFO_SIZE * (hart_count) * (hart_count);

	return BIT_ALIGN(heap_size, HEAP_BASE_ALIGN);
}

static u32 fw_platform_get_heap_size(const void *fdt, u32 hart_count)
{
	int chosen_offset, config_offset, len;
	const fdt32_t *val;

	/* Get the heap size from device tree */
	chosen_offset = fdt_path_offset(fdt, "/chosen");
	if (chosen_offset < 0)
		goto default_config;

	config_offset = fdt_node_offset_by_compatible(fdt, chosen_offset, "opensbi,config");
	if (config_offset < 0)
		goto default_config;

	val = (fdt32_t *)fdt_getprop(fdt, config_offset, "heap-size", &len);
	if (len > 0 && val)
		return BIT_ALIGN(fdt32_to_cpu(*val), HEAP_BASE_ALIGN);

default_config:
	return fw_platform_calculate_heap_size(hart_count);
}

extern struct sbi_platform platform;
static bool platform_has_mlevel_imsic = false;
static u32 generic_hart_index2id[SBI_HARTMASK_MAX_BITS] = { 0 };

static DECLARE_BITMAP(generic_coldboot_harts, SBI_HARTMASK_MAX_BITS);

/*
 * The fw_platform_coldboot_harts_init() function is called by fw_platform_init()
 * function to initialize the cold boot harts allowed by the generic platform
 * according to the DT property "cold-boot-harts" in "/chosen/opensbi-config"
 * DT node. If there is no "cold-boot-harts" in DT, all harts will be allowed.
 */
static void fw_platform_coldboot_harts_init(const void *fdt)
{
	int chosen_offset, config_offset, cpu_offset, len, err;
	u32 val32;
	const u32 *val;

	bitmap_zero(generic_coldboot_harts, SBI_HARTMASK_MAX_BITS);

	chosen_offset = fdt_path_offset(fdt, "/chosen");
	if (chosen_offset < 0)
		goto default_config;

	config_offset = fdt_node_offset_by_compatible(fdt, chosen_offset, "opensbi,config");
	if (config_offset < 0)
		goto default_config;

	val = fdt_getprop(fdt, config_offset, "cold-boot-harts", &len);
	if (!val || !len)
		goto default_config;

	len = len / sizeof(u32);
	for (int i = 0; i < len; i++) {
		cpu_offset = fdt_node_offset_by_phandle(fdt,
						fdt32_to_cpu(val[i]));
		if (cpu_offset < 0)
			goto default_config;

		err = fdt_parse_hart_id(fdt, cpu_offset, &val32);
		if (err)
			goto default_config;

		if (!fdt_node_is_enabled(fdt, cpu_offset))
			continue;

		for (int i = 0; i < platform.hart_count; i++) {
			if (val32 == generic_hart_index2id[i])
				bitmap_set(generic_coldboot_harts, i, 1);
		}
	}

	return;

default_config:
	bitmap_fill(generic_coldboot_harts, SBI_HARTMASK_MAX_BITS);
	return;
}

/*
 * The fw_platform_init() function is called very early on the boot HART
 * OpenSBI reference firmwares so that platform specific code get chance
 * to update "platform" instance before it is used.
 *
 * The arguments passed to fw_platform_init() function are boot time state
 * of A0 to A4 register. The "arg0" will be boot HART id and "arg1" will
 * be address of FDT passed by previous booting stage.
 *
 * The return value of fw_platform_init() function is the FDT location. If
 * FDT is unchanged (or FDT is modified in-place) then fw_platform_init()
 * can always return the original FDT location (i.e. 'arg1') unmodified.
 */
unsigned long fw_platform_init(unsigned long arg0, unsigned long arg1,
				unsigned long arg2, unsigned long arg3,
				unsigned long arg4)
{
	const char *model;
	const void *fdt = (void *)arg1;
	u32 hartid, hart_count = 0;
	int rc, root_offset, cpus_offset, cpu_offset, len;
	unsigned long cbom_block_size = 0;
	unsigned long tmp = 0;

	root_offset = fdt_path_offset(fdt, "/");
	if (root_offset < 0)
		goto fail;

	fdt_driver_init_by_offset(fdt, root_offset, platform_override_modules);

	model = fdt_getprop(fdt, root_offset, "model", &len);
	if (model)
		sbi_strncpy(platform.name, model, sizeof(platform.name) - 1);

	cpus_offset = fdt_path_offset(fdt, "/cpus");
	if (cpus_offset < 0)
		goto fail;

	fdt_for_each_subnode(cpu_offset, fdt, cpus_offset) {
		rc = fdt_parse_hart_id(fdt, cpu_offset, &hartid);
		if (rc)
			continue;

		if (SBI_HARTMASK_MAX_BITS <= hart_count)
			break;

		if (!fdt_node_is_enabled(fdt, cpu_offset))
			continue;

		generic_hart_index2id[hart_count++] = hartid;

		rc = fdt_parse_cbom_block_size(fdt, cpu_offset, &tmp);
		if (rc)
			continue;
		cbom_block_size = MAX(tmp, cbom_block_size);
	}

	platform.hart_count = hart_count;
	platform.heap_size = fw_platform_get_heap_size(fdt, hart_count);
	platform_has_mlevel_imsic = fdt_check_imsic_mlevel(fdt);
	platform.cbom_block_size = cbom_block_size;

	fw_platform_coldboot_harts_init(fdt);

	/* Return original FDT pointer */
	return arg1;

fail:
	while (1)
		wfi();
}

bool generic_cold_boot_allowed(u32 hartid)
{
	for (int i = 0; i < platform.hart_count; i++) {
		if (hartid == generic_hart_index2id[i])
			return bitmap_test(generic_coldboot_harts, i);
	}

	return false;
}

int generic_nascent_init(void)
{
	if (platform_has_mlevel_imsic)
		imsic_local_irqchip_init();
	return 0;
}

int generic_early_init(bool cold_boot)
{
	const void *fdt = fdt_get_address();
	int rc;

	if (cold_boot) {
		if (semihosting_enabled())
			rc = semihosting_init();
		else
			rc = fdt_serial_init(fdt);
		if (rc)
			return rc;

		fdt_driver_init_all(fdt, fdt_early_drivers);
	}

	return fdt_cmo_init(cold_boot);
}

int generic_final_init(bool cold_boot)
{
	void *fdt = fdt_get_address_rw();

	if (!cold_boot)
		return 0;

	fdt_cpu_fixup(fdt);
	fdt_fixups(fdt);
	fdt_domain_fixup(fdt);

	fdt_pack(fdt);

	return 0;
}

int generic_extensions_init(struct sbi_hart_features *hfeatures)
{
	/* Parse the ISA string from FDT and enable the listed extensions */
	return fdt_parse_isa_extensions(fdt_get_address(), current_hartid(),
					hfeatures->extensions);
}

int generic_domains_init(void)
{
	const void *fdt = fdt_get_address();
	int offset, ret;

	ret = fdt_domains_populate(fdt);
	if (ret < 0)
		return ret;

	offset = fdt_path_offset(fdt, "/chosen");

	if (offset >= 0) {
		offset = fdt_node_offset_by_compatible(fdt, offset,
						       "opensbi,config");
		if (offset >= 0 &&
		    fdt_get_property(fdt, offset, "system-suspend-test", NULL))
			sbi_system_suspend_test_enable();
	}

	return 0;
}

u64 generic_tlbr_flush_limit(void)
{
	return SBI_PLATFORM_TLB_RANGE_FLUSH_LIMIT_DEFAULT;
}

u32 generic_tlb_num_entries(void)
{
	return sbi_hart_count();
}

int generic_pmu_init(void)
{
	int rc;

	rc = fdt_pmu_setup(fdt_get_address());
	if (rc && rc != SBI_ENOENT)
		return rc;

	return 0;
}

uint64_t generic_pmu_xlate_to_mhpmevent(uint32_t event_idx, uint64_t data)
{
	uint64_t evt_val = 0;

	/* data is valid only for raw events and is equal to event selector */
	if (event_idx == SBI_PMU_EVENT_RAW_IDX ||
		event_idx == SBI_PMU_EVENT_RAW_V2_IDX)
		evt_val = data;
	else {
		/**
		 * Generic platform follows the SBI specification recommendation
		 * i.e. zero extended event_idx is used as mhpmevent value for
		 * hardware general/cache events if platform does't define one.
		 */
		evt_val = fdt_pmu_get_select_value(event_idx);
		if (!evt_val)
			evt_val = (uint64_t)event_idx;
	}

	return evt_val;
}

int generic_mpxy_init(void)
{
	const void *fdt = fdt_get_address();

	return fdt_mpxy_init(fdt);
}

struct sbi_platform_operations generic_platform_ops = {
	.cold_boot_allowed	= generic_cold_boot_allowed,
	.nascent_init		= generic_nascent_init,
	.early_init		= generic_early_init,
	.final_init		= generic_final_init,
	.extensions_init	= generic_extensions_init,
	.domains_init		= generic_domains_init,
	.irqchip_init		= fdt_irqchip_init,
	.pmu_init		= generic_pmu_init,
	.pmu_xlate_to_mhpmevent = generic_pmu_xlate_to_mhpmevent,
	.get_tlbr_flush_limit	= generic_tlbr_flush_limit,
	.get_tlb_num_entries	= generic_tlb_num_entries,
	.timer_init		= fdt_timer_init,
	.mpxy_init		= generic_mpxy_init,
};

struct sbi_platform platform = {
	.opensbi_version	= OPENSBI_VERSION,
	.platform_version	=
		SBI_PLATFORM_VERSION(CONFIG_PLATFORM_GENERIC_MAJOR_VER,
				     CONFIG_PLATFORM_GENERIC_MINOR_VER),
	.name			= CONFIG_PLATFORM_GENERIC_NAME,
	.features		= SBI_PLATFORM_DEFAULT_FEATURES,
	.hart_count		= SBI_HARTMASK_MAX_BITS,
	.hart_index2id		= generic_hart_index2id,
	.hart_stack_size	= SBI_PLATFORM_DEFAULT_HART_STACK_SIZE,
	.heap_size		= SBI_PLATFORM_DEFAULT_HEAP_SIZE(0),
	.platform_ops_addr	= (unsigned long)&generic_platform_ops
};
