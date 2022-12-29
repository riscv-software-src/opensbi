/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022 StarFive
 *
 * Authors:
 *   Wei Liang Lim <weiliang.lim@starfivetech.com>
 */

#include <libfdt.h>
#include <platform_override.h>
#include <sbi_utils/fdt/fdt_helper.h>

static u32 selected_hartid = -1;

static bool starfive_jh7110_cold_boot_allowed(u32 hartid,
				   const struct fdt_match *match)
{
	if (selected_hartid != -1)
		return (selected_hartid == hartid);

	return true;
}

static void starfive_jh7110_fw_init(void *fdt, const struct fdt_match *match)
{
	const fdt32_t *val;
	int len, coff;

	coff = fdt_path_offset(fdt, "/chosen");
	if (-1 < coff) {
		val = fdt_getprop(fdt, coff, "starfive,boot-hart-id", &len);
		if (val && len >= sizeof(fdt32_t))
			selected_hartid = (u32) fdt32_to_cpu(*val);
	}
}

static const struct fdt_match starfive_jh7110_match[] = {
	{ .compatible = "starfive,jh7110" },
	{ },
};

const struct platform_override starfive_jh7110 = {
	.match_table = starfive_jh7110_match,
	.cold_boot_allowed = starfive_jh7110_cold_boot_allowed,
	.fw_init = starfive_jh7110_fw_init,
};
