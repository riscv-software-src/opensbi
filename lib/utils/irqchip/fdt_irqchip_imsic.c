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
#include <sbi_utils/irqchip/imsic.h>

static int irqchip_imsic_update_hartid_table(void *fdt, int nodeoff,
					     struct imsic_data *id)
{
	const fdt32_t *val;
	u32 phandle, hwirq, hartid;
	int i, err, count, cpu_offset, cpu_intc_offset;

	val = fdt_getprop(fdt, nodeoff, "interrupts-extended", &count);
	if (!val || count < sizeof(fdt32_t))
		return SBI_EINVAL;
	count = count / sizeof(fdt32_t);

	for (i = 0; i < count; i += 2) {
		phandle = fdt32_to_cpu(val[i]);
		hwirq = fdt32_to_cpu(val[i + 1]);

		cpu_intc_offset = fdt_node_offset_by_phandle(fdt, phandle);
		if (cpu_intc_offset < 0)
			continue;

		cpu_offset = fdt_parent_offset(fdt, cpu_intc_offset);
		if (cpu_offset < 0)
			continue;

		err = fdt_parse_hart_id(fdt, cpu_offset, &hartid);
		if (err)
			return SBI_EINVAL;

		switch (hwirq) {
		case IRQ_M_EXT:
			err = imsic_map_hartid_to_data(hartid, id, i / 2);
			if (err)
				return err;
			break;
		default:
			break;
		}
	}

	return 0;
}

static int irqchip_imsic_cold_init(void *fdt, int nodeoff,
				    const struct fdt_match *match)
{
	int rc;
	struct imsic_data *id;

	id = sbi_zalloc(sizeof(*id));
	if (!id)
		return SBI_ENOMEM;

	rc = fdt_parse_imsic_node(fdt, nodeoff, id);
	if (rc || !id->targets_mmode)
		goto fail_free_data;

	rc = imsic_cold_irqchip_init(id);
	if (rc)
		goto fail_free_data;

	rc = irqchip_imsic_update_hartid_table(fdt, nodeoff, id);
	if (rc)
		goto fail_free_data;

	return 0;

fail_free_data:
	sbi_free(id);
	return rc;
}

static const struct fdt_match irqchip_imsic_match[] = {
	{ .compatible = "riscv,imsics" },
	{ },
};

struct fdt_irqchip fdt_irqchip_imsic = {
	.match_table = irqchip_imsic_match,
	.cold_init = irqchip_imsic_cold_init,
	.warm_init = imsic_warm_irqchip_init,
};
