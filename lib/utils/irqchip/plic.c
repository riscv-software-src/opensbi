/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 *   Samuel Holland <samuel@sholland.org>
 */

#include <sbi/riscv_io.h>
#include <sbi/riscv_encoding.h>
#include <sbi/sbi_bitops.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_heap.h>
#include <sbi/sbi_string.h>
#include <sbi_utils/irqchip/plic.h>

#define PLIC_PRIORITY_BASE 0x0
#define PLIC_PENDING_BASE 0x1000
#define PLIC_ENABLE_BASE 0x2000
#define PLIC_ENABLE_STRIDE 0x80
#define PLIC_CONTEXT_BASE 0x200000
#define PLIC_CONTEXT_STRIDE 0x1000

#define THEAD_PLIC_CTRL_REG 0x1ffffc

static unsigned long plic_ptr_offset;

#define plic_get_hart_data_ptr(__scratch)				\
	sbi_scratch_read_type((__scratch), void *, plic_ptr_offset)

#define plic_set_hart_data_ptr(__scratch, __plic)			\
	sbi_scratch_write_type((__scratch), void *, plic_ptr_offset, (__plic))

struct plic_data *plic_get(void)
{
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();

	return plic_get_hart_data_ptr(scratch);
}

static u32 plic_get_priority(const struct plic_data *plic, u32 source)
{
	volatile void *plic_priority = (char *)plic->addr +
			PLIC_PRIORITY_BASE + 4 * source;
	return readl(plic_priority);
}

static void plic_set_priority(const struct plic_data *plic, u32 source, u32 val)
{
	volatile void *plic_priority = (char *)plic->addr +
			PLIC_PRIORITY_BASE + 4 * source;
	writel(val, plic_priority);
}

static u32 plic_get_thresh(const struct plic_data *plic, u32 cntxid)
{
	volatile void *plic_thresh;

	plic_thresh = (char *)plic->addr +
		      PLIC_CONTEXT_BASE + PLIC_CONTEXT_STRIDE * cntxid;

	return readl(plic_thresh);
}

static void plic_set_thresh(const struct plic_data *plic, u32 cntxid, u32 val)
{
	volatile void *plic_thresh;

	plic_thresh = (char *)plic->addr +
		      PLIC_CONTEXT_BASE + PLIC_CONTEXT_STRIDE * cntxid;
	writel(val, plic_thresh);
}

static u32 plic_get_ie(const struct plic_data *plic, u32 cntxid,
		       u32 word_index)
{
	volatile void *plic_ie;

	plic_ie = (char *)plic->addr +
		   PLIC_ENABLE_BASE + PLIC_ENABLE_STRIDE * cntxid +
		   4 * word_index;

	return readl(plic_ie);
}

static void plic_set_ie(const struct plic_data *plic, u32 cntxid,
			u32 word_index, u32 val)
{
	volatile void *plic_ie;

	plic_ie = (char *)plic->addr +
		   PLIC_ENABLE_BASE + PLIC_ENABLE_STRIDE * cntxid +
		   4 * word_index;
	writel(val, plic_ie);
}

static void plic_delegate(const struct plic_data *plic)
{
	/* If this is a T-HEAD PLIC, delegate access to S-mode */
	if (plic->flags & PLIC_FLAG_THEAD_DELEGATION)
		writel_relaxed(BIT(0), (char *)plic->addr + THEAD_PLIC_CTRL_REG);
}

static int plic_context_init(const struct plic_data *plic, int context_id,
			     bool enable, u32 threshold)
{
	u32 ie_words, ie_value;

	if (!plic || context_id < 0)
		return SBI_EINVAL;

	ie_words = PLIC_IE_WORDS(plic);
	ie_value = enable ? 0xffffffffU : 0U;

	for (u32 i = 0; i < ie_words; i++)
		plic_set_ie(plic, context_id, i, ie_value);

	plic_set_thresh(plic, context_id, threshold);

	return 0;
}

void plic_suspend(void)
{
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();
	const struct plic_data *plic = plic_get_hart_data_ptr(scratch);
	u32 ie_words = PLIC_IE_WORDS(plic);
	u32 *data_word = plic->pm_data;
	u8 *data_byte;

	if (!data_word)
		return;

	sbi_for_each_hartindex(h) {
		s16 context_id = plic->context_map[h][PLIC_S_CONTEXT];

		if (context_id < 0)
			continue;

		/* Save the enable bits */
		for (u32 i = 0; i < ie_words; i++)
			*data_word++ = plic_get_ie(plic, context_id, i);

		/* Save the context threshold */
		*data_word++ = plic_get_thresh(plic, context_id);
	}

	/* Restore the input priorities */
	data_byte = (u8 *)data_word;
	for (u32 i = 1; i <= plic->num_src; i++)
		*data_byte++ = plic_get_priority(plic, i);
}

void plic_resume(void)
{
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();
	const struct plic_data *plic = plic_get_hart_data_ptr(scratch);
	u32 ie_words = PLIC_IE_WORDS(plic);
	u32 *data_word = plic->pm_data;
	u8 *data_byte;

	if (!data_word)
		return;

	sbi_for_each_hartindex(h) {
		s16 context_id = plic->context_map[h][PLIC_S_CONTEXT];

		if (context_id < 0)
			continue;

		/* Restore the enable bits */
		for (u32 i = 0; i < ie_words; i++)
			plic_set_ie(plic, context_id, i, *data_word++);

		/* Restore the context threshold */
		plic_set_thresh(plic, context_id, *data_word++);
	}

	/* Restore the input priorities */
	data_byte = (u8 *)data_word;
	for (u32 i = 1; i <= plic->num_src; i++)
		plic_set_priority(plic, i, *data_byte++);

	/* Restore the delegation */
	plic_delegate(plic);
}

static int plic_warm_irqchip_init(struct sbi_irqchip_device *dev)
{
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();
	const struct plic_data *plic = plic_get_hart_data_ptr(scratch);
	u32 hartindex = current_hartindex();
	s16 m_cntx_id = plic->context_map[hartindex][PLIC_M_CONTEXT];
	s16 s_cntx_id = plic->context_map[hartindex][PLIC_S_CONTEXT];
	bool enable;
	int ret;

	/*
	 * By default, disable all IRQs for the target HART. Ariane
	 * has a bug which requires enabling all interrupts at boot.
	 */
	enable = plic->flags & PLIC_FLAG_ARIANE_BUG;

	if (m_cntx_id > -1) {
		ret = plic_context_init(plic, m_cntx_id, enable, 0x7);
		if (ret)
			return ret;
	}

	if (s_cntx_id > -1) {
		ret = plic_context_init(plic, s_cntx_id, enable, 0x7);
		if (ret)
			return ret;
	}

	return 0;
}

int plic_cold_irqchip_init(struct plic_data *plic)
{
	int i, ret;

	if (!plic)
		return SBI_EINVAL;

	if (!plic_ptr_offset) {
		plic_ptr_offset = sbi_scratch_alloc_type_offset(void *);
		if (!plic_ptr_offset)
			return SBI_ENOMEM;
	}

	if (plic->flags & PLIC_FLAG_ENABLE_PM) {
		unsigned long data_size = 0;

		sbi_for_each_hartindex(i) {
			if (plic->context_map[i][PLIC_S_CONTEXT] < 0)
				continue;

			/* Allocate space for enable bits */
			data_size += (plic->num_src / 32 + 1) * sizeof(u32);

			/* Allocate space for the context threshold */
			data_size += sizeof(u32);
		}

		/*
		 * Allocate space for the input priorities. So far,
		 * priorities on all known implementations fit in 8 bits.
		 */
		data_size += plic->num_src * sizeof(u8);

		plic->pm_data = sbi_malloc(data_size);
		if (!plic->pm_data)
			return SBI_ENOMEM;
	}

	/* Configure default priorities of all IRQs */
	for (i = 1; i <= plic->num_src; i++)
		plic_set_priority(plic, i, 0);

	plic_delegate(plic);

	ret = sbi_domain_root_add_memrange(plic->addr, plic->size, BIT(20),
					(SBI_DOMAIN_MEMREGION_MMIO |
					 SBI_DOMAIN_MEMREGION_SHARED_SURW_MRW));
	if (ret)
		return ret;

	sbi_for_each_hartindex(i) {
		if (plic->context_map[i][PLIC_M_CONTEXT] < 0 &&
		    plic->context_map[i][PLIC_S_CONTEXT] < 0)
			continue;

		plic_set_hart_data_ptr(sbi_hartindex_to_scratch(i), plic);
	}

	/* Register irqchip device */
	plic->irqchip.warm_init = plic_warm_irqchip_init;
	sbi_irqchip_add_device(&plic->irqchip);

	return 0;
}
