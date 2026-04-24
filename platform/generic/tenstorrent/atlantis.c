/*
 * SPDX-FileCopyrightText: (c) 2025-2026 Tenstorrent USA, Inc.
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <platform_override.h>
#include <libfdt.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_system.h>
#include <sbi/sbi_console.h>

#include <tenstorrent/ascalon.h>
#include <tenstorrent/pma.h>

static int tt_atlantis_final_init(bool cold_boot)
{
	if (cold_boot) {
		/* Boot firmware sets HART PMAs. Read and verify them. */
		tt_ascalon_discover_pmas_from_boot_hart();
	} else {
		/* Verify nonboot HARTs have PMAs matching boot HART */
		tt_ascalon_verify_pmas_nonboot_hart();
	}

	return generic_final_init(cold_boot);
}

static bool tt_atlantis_single_fw_region(void)
{
	return true;
}

static int tt_atlantis_platform_init(const void *fdt, int nodeoff, const struct fdt_match *match)
{
	generic_platform_ops.final_init = tt_atlantis_final_init;
	generic_platform_ops.single_fw_region = tt_atlantis_single_fw_region;

	return 0;
}

static const struct fdt_match tt_atlantis_match[] = {
	{ .compatible = "tenstorrent,atlantis" },
	{ },
};

const struct fdt_driver tenstorrent_atlantis = {
	.match_table = tt_atlantis_match,
	.init = tt_atlantis_platform_init,
};
