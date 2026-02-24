/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2026 SiFive Inc.
 */

#include <platform_override.h>

static int sifive_platform_init(const void *fdt, int nodeoff,
				const struct fdt_match *match)
{
	return 0;
}

static const struct fdt_match sifive_dev_platform_match[] = {
	{ .compatible = "sifive-dev" },
	{ },
};

const struct fdt_driver sifive_dev_platform = {
	.match_table = sifive_dev_platform_match,
	.init = sifive_platform_init,
};
