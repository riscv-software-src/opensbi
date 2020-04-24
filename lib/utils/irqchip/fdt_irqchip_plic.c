/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <libfdt.h>
#include <sbi/riscv_asm.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_hartmask.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/irqchip/fdt_irqchip.h>
#include <sbi_utils/irqchip/plic.h>

static int plic_hartid2context[SBI_HARTMASK_MAX_BITS][2];

static int irqchip_plic_warm_init(void)
{
	u32 hartid = current_hartid();

	return plic_warm_irqchip_init(hartid,
				      plic_hartid2context[hartid][0],
				      plic_hartid2context[hartid][1]);
}

static int irqchip_plic_parse_hartid2context(void *fdt, int nodeoff)
{
	const fdt32_t *val;
	u32 phandle, hwirq, hartid;
	int i, err, count, cpu_offset, cpu_intc_offset;

	for (i = 0; i < SBI_HARTMASK_MAX_BITS; i++) {
		plic_hartid2context[i][0] = -1;
		plic_hartid2context[i][1] = -1;
	}

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
		if (cpu_intc_offset < 0)
			continue;

		err = fdt_parse_hart_id(fdt, cpu_offset, &hartid);
		if (err)
			continue;

		if (SBI_HARTMASK_MAX_BITS <= hartid)
			continue;

		switch (hwirq) {
		case IRQ_M_EXT:
			plic_hartid2context[hartid][0] = i / 2;
			break;
		case IRQ_S_EXT:
			plic_hartid2context[hartid][1] = i / 2;
			break;
		}
	}

	return 0;
}

static int irqchip_plic_cold_init(void *fdt, int nodeoff,
				  const struct fdt_match *match)
{
	int rc;
	u32 max_hartid;
	struct platform_plic_data plic;

	rc = fdt_parse_max_hart_id(fdt, &max_hartid);
	if (rc)
		return rc;

	rc = fdt_parse_plic_node(fdt, nodeoff, &plic);
	if (rc)
		return rc;

	rc = plic_cold_irqchip_init(plic.addr, plic.num_src, max_hartid + 1);
	if (rc)
		return rc;

	return irqchip_plic_parse_hartid2context(fdt, nodeoff);
}

static const struct fdt_match irqchip_plic_match[] = {
	{ .compatible = "riscv,plic0" },
	{ .compatible = "sifive,plic-1.0.0" },
	{ },
};

struct fdt_irqchip fdt_irqchip_plic = {
	.match_table = irqchip_plic_match,
	.cold_init = irqchip_plic_cold_init,
	.warm_init = irqchip_plic_warm_init,
	.exit = NULL,
};
