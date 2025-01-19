/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Anup Patel <apatel@ventanamicro.com>
 */

#include <libfdt.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_system.h>
#include <sbi_utils/fdt/fdt_driver.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/regmap/fdt_regmap.h>

struct syscon_reset {
	struct regmap *rmap;
	u32 priority;
	u32 offset;
	u32 value;
	u32 mask;
};

static struct syscon_reset poweroff;
static struct syscon_reset reboot;

static struct syscon_reset *syscon_reset_get(bool is_reboot, u32 type)
{
	struct syscon_reset *reset = NULL;

	switch (type) {
	case SBI_SRST_RESET_TYPE_SHUTDOWN:
		if (!is_reboot)
			reset = &poweroff;
		break;
	case SBI_SRST_RESET_TYPE_COLD_REBOOT:
	case SBI_SRST_RESET_TYPE_WARM_REBOOT:
		if (is_reboot)
			reset = &reboot;
		break;
	}

	if (reset && !reset->rmap)
		reset = NULL;

	return reset;
}

static void syscon_reset_exec(struct syscon_reset *reset)
{
	/* Issue the reset through regmap */
	if (reset)
		regmap_update_bits(reset->rmap, reset->offset,
				   reset->mask, reset->value);

	/* hang !!! */
	sbi_hart_hang();
}

static int syscon_poweroff_check(u32 type, u32 reason)
{
	struct syscon_reset *reset = syscon_reset_get(false, type);

	return (reset) ? reset->priority : 0;
}

static void syscon_do_poweroff(u32 type, u32 reason)
{
	syscon_reset_exec(syscon_reset_get(false, type));
}

static struct sbi_system_reset_device syscon_poweroff = {
	.name = "syscon-poweroff",
	.system_reset_check = syscon_poweroff_check,
	.system_reset = syscon_do_poweroff
};

static int syscon_reboot_check(u32 type, u32 reason)
{
	struct syscon_reset *reset = syscon_reset_get(true, type);

	return (reset) ? reset->priority : 0;
}

static void syscon_do_reboot(u32 type, u32 reason)
{
	syscon_reset_exec(syscon_reset_get(true, type));
}

static struct sbi_system_reset_device syscon_reboot = {
	.name = "syscon-reboot",
	.system_reset_check = syscon_reboot_check,
	.system_reset = syscon_do_reboot
};

static int syscon_reset_init(const void *fdt, int nodeoff,
			     const struct fdt_match *match)
{
	int rc, len;
	const fdt32_t *val, *mask;
	bool is_reboot = (ulong)match->data;
	struct syscon_reset *reset = (is_reboot) ? &reboot : &poweroff;

	if (reset->rmap)
		return SBI_EALREADY;

	rc = fdt_regmap_get(fdt, nodeoff, &reset->rmap);
	if (rc)
		return rc;

	val = fdt_getprop(fdt, nodeoff, "priority", &len);
	reset->priority = (val && len > 0) ? fdt32_to_cpu(*val) : 192;

	val = fdt_getprop(fdt, nodeoff, "offset", &len);
	if (val && len > 0)
		reset->offset = fdt32_to_cpu(*val);
	else
		return SBI_EINVAL;

	val = fdt_getprop(fdt, nodeoff, "value", &len);
	mask = fdt_getprop(fdt, nodeoff, "mask", &len);
	if (!val && !mask)
		return SBI_EINVAL;

	if (!val) {
		/* support old binding */
		reset->value = fdt32_to_cpu(*mask);
		reset->mask = 0xFFFFFFFF;
	} else if (!mask) {
		/* support value without mask */
		reset->value = fdt32_to_cpu(*val);
		reset->mask = 0xFFFFFFFF;
	} else {
		reset->value = fdt32_to_cpu(*val);
		reset->mask = fdt32_to_cpu(*mask);
	}

	if (is_reboot)
		sbi_system_reset_add_device(&syscon_reboot);
	else
		sbi_system_reset_add_device(&syscon_poweroff);

	return 0;
}

static const struct fdt_match syscon_poweroff_match[] = {
	{ .compatible = "syscon-poweroff", .data = (const void *)false },
	{ },
};

const struct fdt_driver fdt_syscon_poweroff = {
	.match_table = syscon_poweroff_match,
	.init = syscon_reset_init,
};

static const struct fdt_match syscon_reboot_match[] = {
	{ .compatible = "syscon-reboot", .data = (const void *)true },
	{ },
};

const struct fdt_driver fdt_syscon_reboot = {
	.match_table = syscon_reboot_match,
	.init = syscon_reset_init,
};
