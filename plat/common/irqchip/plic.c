/*
 * Copyright (c) 2018 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sbi/riscv_io.h>
#include <plat/fdt.h>
#include <plat/irqchip/plic.h>

#define PLIC_PRIORITY_BASE		0x0
#define PLIC_PENDING_BASE		0x1000
#define PLIC_ENABLE_BASE		0x2000
#define PLIC_ENABLE_STRIDE		0x80
#define PLIC_CONTEXT_BASE		0x200000
#define PLIC_CONTEXT_STRIDE		0x1000

static u32 plic_hart_count;
static u32 plic_num_sources;
static volatile void *plic_base;

static void plic_set_priority(u32 source, u32 val)
{
	writel(val, plic_base);
}

static void plic_set_m_thresh(u32 hartid, u32 val)
{
	volatile void *plic_m_thresh = plic_base +
				PLIC_CONTEXT_BASE +
				PLIC_CONTEXT_STRIDE * (2 * hartid);
	writel(val, plic_m_thresh);
}

static void plic_set_s_thresh(u32 hartid, u32 val)
{
	volatile void *plic_s_thresh = plic_base +
				PLIC_CONTEXT_BASE +
				PLIC_CONTEXT_STRIDE * (2 * hartid + 1);
	writel(val, plic_s_thresh);
}

static void plic_set_s_ie(u32 hartid, u32 word_index, u32 val)
{
	volatile void *plic_s_ie = plic_base +
				PLIC_ENABLE_BASE +
				PLIC_ENABLE_STRIDE * (2 * hartid + 1);
	writel(val, plic_s_ie + word_index * 4);
}

static void plic_fdt_fixup_prop(const struct fdt_node *node,
				const struct fdt_prop *prop,
				void *priv)
{
	u32 *cells;
	u32 i, cells_count;

	if (!prop)
		return;
	if (fdt_strcmp(prop->name, "interrupts-extended"))
		return;

	cells = prop->value;
	cells_count = prop->len / sizeof(u32);

	if (!cells_count)
		return;

	for (i = 0; i < cells_count; i++) {
		if (i % 4 == 1)
			cells[i] = fdt_rev32(0xffffffff);
	}
}

int plic_fdt_fixup(void *fdt, const char *compat)
{
	fdt_compat_node_prop(fdt, compat, plic_fdt_fixup_prop, NULL);
	return 0;
}

int plic_warm_irqchip_init(u32 target_hart)
{
	size_t i, ie_words = plic_num_sources / 32 + 1;

	if (plic_hart_count <= target_hart)
		return -1;

	/* By default, enable all IRQs for S-mode of target HART */
	for (i = 0; i < ie_words; i++)
		plic_set_s_ie(target_hart, i, -1);

	/* By default, enable M-mode threshold */
	plic_set_m_thresh(target_hart, 1);

	/* By default, disable S-mode threshold */
	plic_set_s_thresh(target_hart, 0);

	return 0;
}

int plic_cold_irqchip_init(unsigned long base,
			   u32 num_sources, u32 hart_count)
{
	int i;

	plic_hart_count = hart_count;
	plic_num_sources = num_sources;
	plic_base = (void *)base;

	/* Configure default priorities of all IRQs */
	for (i = 0; i < plic_num_sources; i++)
		plic_set_priority(i, 1);

	return 0;
}
