/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <sbi/riscv_io.h>
#include <sbi/riscv_encoding.h>
#include <sbi/sbi_console.h>
#include <plat/string.h>
#include <plat/tinyfdt.h>
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
	volatile void *plic_priority = plic_base +
				PLIC_PRIORITY_BASE +
				4 * source;
	writel(val, plic_priority);
}

static void plic_set_thresh(u32 cntxid, u32 val)
{
	volatile void *plic_thresh = plic_base +
				PLIC_CONTEXT_BASE +
				PLIC_CONTEXT_STRIDE * cntxid;
	writel(val, plic_thresh);
}

static void plic_set_ie(u32 cntxid, u32 word_index, u32 val)
{
	volatile void *plic_ie = plic_base +
				PLIC_ENABLE_BASE +
				PLIC_ENABLE_STRIDE * cntxid;
	writel(val, plic_ie + word_index * 4);
}

static void plic_fdt_fixup_prop(const struct fdt_node *node,
				const struct fdt_prop *prop,
				void *priv)
{
	u32 *cells;
	u32 i, cells_count;

	if (!prop)
		return;
	if (strcmp(prop->name, "interrupts-extended"))
		return;

	cells = prop->value;
	cells_count = prop->len / sizeof(u32);

	if (!cells_count)
		return;

	for (i = 0; i < (cells_count/2); i++) {
		if (fdt_rev32(cells[2*i+1]) == IRQ_M_EXT)
			cells[2*i+1] = fdt_rev32(0xffffffff);
	}
}

void plic_fdt_fixup(void *fdt, const char *compat)
{
	fdt_compat_node_prop(fdt, compat, plic_fdt_fixup_prop, NULL);
}

int plic_warm_irqchip_init(u32 target_hart,
			   int m_cntx_id, int s_cntx_id)
{
	size_t i, ie_words = plic_num_sources / 32 + 1;

	if (plic_hart_count <= target_hart)
		return -1;
	
	/* By default, disable all IRQs for M-mode of target HART */
	if (m_cntx_id > -1) {
		for (i = 0; i < ie_words; i++)
			plic_set_ie(m_cntx_id, i, 0);
	}

	/* By default, disable all IRQs for S-mode of target HART */
	if (s_cntx_id > -1) {
		for (i = 0; i < ie_words; i++)
			plic_set_ie(s_cntx_id, i, 0);
	}

	/* By default, enable M-mode threshold */
	if (m_cntx_id > -1)
		plic_set_thresh(m_cntx_id, 1);

	/* By default, disable S-mode threshold */
	if (s_cntx_id > -1)
		plic_set_thresh(s_cntx_id, 0);

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
	for (i = 1; i <= plic_num_sources; i++)
		plic_set_priority(i, 1);

	return 0;
}
