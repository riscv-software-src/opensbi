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
#include <sbi/riscv_io.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_heap.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_scratch.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/irqchip/fdt_irqchip.h>
#include <sbi_utils/irqchip/plic.h>

static unsigned long plic_ptr_offset;

#define plic_get_hart_data_ptr(__scratch)				\
	sbi_scratch_read_type((__scratch), void *, plic_ptr_offset)

#define plic_set_hart_data_ptr(__scratch, __plic)			\
	sbi_scratch_write_type((__scratch), void *, plic_ptr_offset, (__plic))

struct plic_data *fdt_plic_get(void)
{
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();

	return plic_get_hart_data_ptr(scratch);
}

void fdt_plic_suspend(void)
{
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();

	plic_suspend(plic_get_hart_data_ptr(scratch));
}

void fdt_plic_resume(void)
{
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();

	plic_resume(plic_get_hart_data_ptr(scratch));
}

static int irqchip_plic_warm_init(void)
{
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();

	return plic_warm_irqchip_init(plic_get_hart_data_ptr(scratch));
}

static int irqchip_plic_update_context_map(const void *fdt, int nodeoff,
					   struct plic_data *pd)
{
	const fdt32_t *val;
	u32 phandle, hwirq, hartid, hartindex;
	struct sbi_scratch *scratch;
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
			continue;

		hartindex = sbi_hartid_to_hartindex(hartid);
		scratch = sbi_hartindex_to_scratch(hartindex);
		if (!scratch)
			continue;

		plic_set_hart_data_ptr(scratch, pd);
		switch (hwirq) {
		case IRQ_M_EXT:
			pd->context_map[hartindex][PLIC_M_CONTEXT] = i / 2;
			break;
		case IRQ_S_EXT:
			pd->context_map[hartindex][PLIC_S_CONTEXT] = i / 2;
			break;
		}
	}

	return 0;
}

static int irqchip_plic_cold_init(const void *fdt, int nodeoff,
				  const struct fdt_match *match)
{
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();
	const struct sbi_platform *plat = sbi_platform_ptr(scratch);
	int rc;
	struct plic_data *pd;

	if (!plic_ptr_offset) {
		plic_ptr_offset = sbi_scratch_alloc_type_offset(void *);
		if (!plic_ptr_offset)
			return SBI_ENOMEM;
	}

	pd = sbi_zalloc(PLIC_DATA_SIZE(plat->hart_count));
	if (!pd)
		return SBI_ENOMEM;

	rc = fdt_parse_plic_node(fdt, nodeoff, pd);
	if (rc)
		goto fail_free_data;

	pd->flags = (unsigned long)match->data;

	rc = irqchip_plic_update_context_map(fdt, nodeoff, pd);
	if (rc)
		goto fail_free_data;

	rc = plic_cold_irqchip_init(pd);
	if (rc)
		goto fail_free_data;

	return 0;

fail_free_data:
	sbi_free(pd);
	return rc;
}

static const struct fdt_match irqchip_plic_match[] = {
	{ .compatible = "andestech,nceplic100" },
	{ .compatible = "riscv,plic0" },
	{ .compatible = "sifive,plic-1.0.0" },
	{ .compatible = "thead,c900-plic",
	  .data = (void *)(PLIC_FLAG_THEAD_DELEGATION | PLIC_FLAG_ENABLE_PM) },
	{ /* sentinel */ }
};

struct fdt_irqchip fdt_irqchip_plic = {
	.match_table = irqchip_plic_match,
	.cold_init = irqchip_plic_cold_init,
	.warm_init = irqchip_plic_warm_init,
	.exit = NULL,
};
