/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2021 Western Digital Corporation or its affiliates.
 * Copyright (c) 2022 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <libfdt.h>
#include <sbi/riscv_asm.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_heap.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/irqchip/fdt_irqchip.h>
#include <sbi_utils/irqchip/aplic.h>

static int irqchip_aplic_warm_init(void)
{
	/* Nothing to do here. */
	return 0;
}

static int irqchip_aplic_cold_init(void *fdt, int nodeoff,
				  const struct fdt_match *match)
{
	int rc;
	struct aplic_data *pd;

	pd = sbi_zalloc(sizeof(*pd));
	if (!pd)
		return SBI_ENOMEM;

	rc = fdt_parse_aplic_node(fdt, nodeoff, pd);
	if (rc)
		goto fail_free_data;

	rc = aplic_cold_irqchip_init(pd);
	if (rc)
		goto fail_free_data;

	return 0;

fail_free_data:
	sbi_free(pd);
	return rc;
}

static const struct fdt_match irqchip_aplic_match[] = {
	{ .compatible = "riscv,aplic" },
	{ },
};

struct fdt_irqchip fdt_irqchip_aplic = {
	.match_table = irqchip_aplic_match,
	.cold_init = irqchip_aplic_cold_init,
	.warm_init = irqchip_aplic_warm_init,
	.exit = NULL,
};
