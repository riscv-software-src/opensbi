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
#include <sbi/sbi_scratch.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/irqchip/fdt_irqchip.h>
#include <sbi_utils/irqchip/plic.h>

static unsigned long plic_ptr_offset;

#define plic_get_hart_data_ptr(__scratch)				\
	sbi_scratch_read_type((__scratch), void *, plic_ptr_offset)

#define plic_set_hart_data_ptr(__scratch, __plic)			\
	sbi_scratch_write_type((__scratch), void *, plic_ptr_offset, (__plic))

static unsigned long plic_mcontext_offset;

#define plic_get_hart_mcontext(__scratch)				\
	(sbi_scratch_read_type((__scratch), long, plic_mcontext_offset) - 1)

#define plic_set_hart_mcontext(__scratch, __mctx)			\
	sbi_scratch_write_type((__scratch), long, plic_mcontext_offset, (__mctx) + 1)

static unsigned long plic_scontext_offset;

#define plic_get_hart_scontext(__scratch)				\
	(sbi_scratch_read_type((__scratch), long, plic_scontext_offset) - 1)

#define plic_set_hart_scontext(__scratch, __sctx)			\
	sbi_scratch_write_type((__scratch), long, plic_scontext_offset, (__sctx) + 1)

void fdt_plic_priority_save(u8 *priority, u32 num)
{
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();

	plic_priority_save(plic_get_hart_data_ptr(scratch), priority, num);
}

void fdt_plic_priority_restore(const u8 *priority, u32 num)
{
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();

	plic_priority_restore(plic_get_hart_data_ptr(scratch), priority, num);
}

void fdt_plic_context_save(bool smode, u32 *enable, u32 *threshold, u32 num)
{
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();

	plic_context_save(plic_get_hart_data_ptr(scratch),
			  smode ? plic_get_hart_scontext(scratch) :
				  plic_get_hart_mcontext(scratch),
			  enable, threshold, num);
}

void fdt_plic_context_restore(bool smode, const u32 *enable, u32 threshold,
			      u32 num)
{
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();

	plic_context_restore(plic_get_hart_data_ptr(scratch),
			     smode ? plic_get_hart_scontext(scratch) :
				     plic_get_hart_mcontext(scratch),
			     enable, threshold, num);
}

static int irqchip_plic_warm_init(void)
{
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();

	return plic_warm_irqchip_init(plic_get_hart_data_ptr(scratch),
				      plic_get_hart_mcontext(scratch),
				      plic_get_hart_scontext(scratch));
}

static int irqchip_plic_update_hartid_table(void *fdt, int nodeoff,
					    struct plic_data *pd)
{
	const fdt32_t *val;
	u32 phandle, hwirq, hartid;
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

		scratch = sbi_hartid_to_scratch(hartid);
		if (!scratch)
			continue;

		plic_set_hart_data_ptr(scratch, pd);
		switch (hwirq) {
		case IRQ_M_EXT:
			plic_set_hart_mcontext(scratch, i / 2);
			break;
		case IRQ_S_EXT:
			plic_set_hart_scontext(scratch, i / 2);
			break;
		}
	}

	return 0;
}

static int irqchip_plic_cold_init(void *fdt, int nodeoff,
				  const struct fdt_match *match)
{
	int rc;
	struct plic_data *pd;

	if (!plic_ptr_offset) {
		plic_ptr_offset = sbi_scratch_alloc_type_offset(void *);
		if (!plic_ptr_offset)
			return SBI_ENOMEM;
	}

	if (!plic_mcontext_offset) {
		plic_mcontext_offset = sbi_scratch_alloc_type_offset(long);
		if (!plic_mcontext_offset)
			return SBI_ENOMEM;
	}

	if (!plic_scontext_offset) {
		plic_scontext_offset = sbi_scratch_alloc_type_offset(long);
		if (!plic_scontext_offset)
			return SBI_ENOMEM;
	}

	pd = sbi_zalloc(sizeof(*pd));
	if (!pd)
		return SBI_ENOMEM;

	rc = fdt_parse_plic_node(fdt, nodeoff, pd);
	if (rc)
		goto fail_free_data;

	if (match->data) {
		void (*plic_plat_init)(struct plic_data *) = match->data;
		plic_plat_init(pd);
	}

	rc = plic_cold_irqchip_init(pd);
	if (rc)
		goto fail_free_data;

	rc = irqchip_plic_update_hartid_table(fdt, nodeoff, pd);
	if (rc)
		goto fail_free_data;

	return 0;

fail_free_data:
	sbi_free(pd);
	return rc;
}

#define THEAD_PLIC_CTRL_REG 0x1ffffc

static void thead_plic_plat_init(struct plic_data *pd)
{
	writel_relaxed(BIT(0), (char *)pd->addr + THEAD_PLIC_CTRL_REG);
}

void thead_plic_restore(void)
{
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();
	struct plic_data *plic = plic_get_hart_data_ptr(scratch);

	thead_plic_plat_init(plic);
}

static const struct fdt_match irqchip_plic_match[] = {
	{ .compatible = "andestech,nceplic100" },
	{ .compatible = "riscv,plic0" },
	{ .compatible = "sifive,plic-1.0.0" },
	{ .compatible = "thead,c900-plic",
	  .data = thead_plic_plat_init },
	{ /* sentinel */ }
};

struct fdt_irqchip fdt_irqchip_plic = {
	.match_table = irqchip_plic_match,
	.cold_init = irqchip_plic_cold_init,
	.warm_init = irqchip_plic_warm_init,
	.exit = NULL,
};
