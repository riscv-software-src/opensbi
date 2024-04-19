// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Renesas Electronics Corp.
 *
 * Copyright (c) 2020 Andes Technology Corporation
 *
 * Authors:
 *      Nick Hu <nickhu@andestech.com>
 *      Nylon Chen <nylon7@andestech.com>
 *      Lad Prabhakar <prabhakar.mahadev-lad.rj@bp.renesas.com>
 */

#include <andes/andes45_pma.h>
#include <libfdt.h>
#include <sbi/riscv_asm.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_error.h>
#include <sbi_utils/fdt/fdt_helper.h>

/* Configuration Registers */
#define ANDES45_CSR_MMSC_CFG		0xFC2
#define ANDES45_CSR_MMSC_PPMA_OFFSET	(1 << 30)

#define ANDES45_PMAADDR_0		0xBD0

#define ANDES45_PMACFG_0		0xBC0

static inline unsigned long andes45_pma_read_cfg(unsigned int pma_cfg_off)
{
#define switchcase_pma_cfg_read(__pma_cfg_off, __val)		\
	case __pma_cfg_off:					\
		__val = csr_read(__pma_cfg_off);		\
		break;
#define switchcase_pma_cfg_read_2(__pma_cfg_off, __val)		\
	switchcase_pma_cfg_read(__pma_cfg_off + 0, __val)	\
	switchcase_pma_cfg_read(__pma_cfg_off + 2, __val)

	unsigned long ret = 0;

	switch (pma_cfg_off) {
	switchcase_pma_cfg_read_2(ANDES45_PMACFG_0, ret)

	default:
		sbi_panic("%s: Unknown PMA CFG offset %#x", __func__, pma_cfg_off);
		break;
	}

	return ret;

#undef switchcase_pma_cfg_read_2
#undef switchcase_pma_cfg_read
}

static inline void andes45_pma_write_cfg(unsigned int pma_cfg_off, unsigned long val)
{
#define switchcase_pma_cfg_write(__pma_cfg_off, __val)		\
	case __pma_cfg_off:					\
		csr_write(__pma_cfg_off, __val);		\
		break;
#define switchcase_pma_cfg_write_2(__pma_cfg_off, __val)	\
	switchcase_pma_cfg_write(__pma_cfg_off + 0, __val)	\
	switchcase_pma_cfg_write(__pma_cfg_off + 2, __val)

	switch (pma_cfg_off) {
	switchcase_pma_cfg_write_2(ANDES45_PMACFG_0, val)

	default:
		sbi_panic("%s: Unknown PMA CFG offset %#x", __func__, pma_cfg_off);
		break;
	}

#undef switchcase_pma_cfg_write_2
#undef switchcase_pma_cfg_write
}

static inline void andes45_pma_write_addr(unsigned int pma_addr_off, unsigned long val)
{
#define switchcase_pma_write(__pma_addr_off, __val)		\
	case __pma_addr_off:					\
		csr_write(__pma_addr_off, __val);		\
		break;
#define switchcase_pma_write_2(__pma_addr_off, __val)		\
	switchcase_pma_write(__pma_addr_off + 0, __val)		\
	switchcase_pma_write(__pma_addr_off + 1, __val)
#define switchcase_pma_write_4(__pma_addr_off, __val)		\
	switchcase_pma_write_2(__pma_addr_off + 0, __val)	\
	switchcase_pma_write_2(__pma_addr_off + 2, __val)
#define switchcase_pma_write_8(__pma_addr_off, __val)		\
	switchcase_pma_write_4(__pma_addr_off + 0, __val)	\
	switchcase_pma_write_4(__pma_addr_off + 4, __val)
#define switchcase_pma_write_16(__pma_addr_off, __val)		\
	switchcase_pma_write_8(__pma_addr_off + 0, __val)	\
	switchcase_pma_write_8(__pma_addr_off + 8, __val)

	switch (pma_addr_off) {
	switchcase_pma_write_16(ANDES45_PMAADDR_0, val)

	default:
		sbi_panic("%s: Unknown PMA ADDR offset %#x", __func__, pma_addr_off);
		break;
	}

#undef switchcase_pma_write_16
#undef switchcase_pma_write_8
#undef switchcase_pma_write_4
#undef switchcase_pma_write_2
#undef switchcase_pma_write
}

static inline unsigned long andes45_pma_read_addr(unsigned int pma_addr_off)
{
#define switchcase_pma_read(__pma_addr_off, __val)		\
	case __pma_addr_off:					\
		__val = csr_read(__pma_addr_off);		\
		break;
#define switchcase_pma_read_2(__pma_addr_off, __val)		\
	switchcase_pma_read(__pma_addr_off + 0, __val)		\
	switchcase_pma_read(__pma_addr_off + 1, __val)
#define switchcase_pma_read_4(__pma_addr_off, __val)		\
	switchcase_pma_read_2(__pma_addr_off + 0, __val)	\
	switchcase_pma_read_2(__pma_addr_off + 2, __val)
#define switchcase_pma_read_8(__pma_addr_off, __val)		\
	switchcase_pma_read_4(__pma_addr_off + 0, __val)	\
	switchcase_pma_read_4(__pma_addr_off + 4, __val)
#define switchcase_pma_read_16(__pma_addr_off, __val)		\
	switchcase_pma_read_8(__pma_addr_off + 0, __val)	\
	switchcase_pma_read_8(__pma_addr_off + 8, __val)

	unsigned long ret = 0;

	switch (pma_addr_off) {
	switchcase_pma_read_16(ANDES45_PMAADDR_0, ret)

	default:
		sbi_panic("%s: Unknown PMA ADDR offset %#x", __func__, pma_addr_off);
		break;
	}

	return ret;

#undef switchcase_pma_read_16
#undef switchcase_pma_read_8
#undef switchcase_pma_read_4
#undef switchcase_pma_read_2
#undef switchcase_pma_read
}

static unsigned long
andes45_pma_setup(const struct andes45_pma_region *pma_region,
		  unsigned int entry_id)
{
	unsigned long size = pma_region->size;
	unsigned long addr = pma_region->pa;
	unsigned int pma_cfg_addr;
	unsigned long pmacfg_val;
	unsigned long pmaaddr;
	char *pmaxcfg;

	/* Check for 4KiB granularity */
	if (size < (1 << 12))
		return SBI_EINVAL;

	/* Check size is power of 2 */
	if (size & (size - 1))
		return SBI_EINVAL;

	if (entry_id > 15)
		return SBI_EINVAL;

	if (!(pma_region->flags & ANDES45_PMACFG_ETYP_NAPOT))
		return SBI_EINVAL;

	if ((addr & (size - 1)) != 0)
		return SBI_EINVAL;

	pma_cfg_addr = entry_id / 8 ? ANDES45_PMACFG_0 + 2 : ANDES45_PMACFG_0;
	pmacfg_val = andes45_pma_read_cfg(pma_cfg_addr);
	pmaxcfg = (char *)&pmacfg_val + (entry_id % 8);
	*pmaxcfg = 0;
	*pmaxcfg = pma_region->flags;

	andes45_pma_write_cfg(pma_cfg_addr, pmacfg_val);

	pmaaddr = (addr >> 2) + (size >> 3) - 1;

	andes45_pma_write_addr(ANDES45_PMAADDR_0 + entry_id, pmaaddr);

	return andes45_pma_read_addr(ANDES45_PMAADDR_0 + entry_id) == pmaaddr ?
			pmaaddr : SBI_EINVAL;
}

static int andes45_fdt_pma_resv(void *fdt, const struct andes45_pma_region *pma,
				unsigned int index, int parent)
{
	int na = fdt_address_cells(fdt, 0);
	int ns = fdt_size_cells(fdt, 0);
	static bool dma_default = false;
	fdt32_t addr_high, addr_low;
	fdt32_t size_high, size_low;
	int subnode, err;
	fdt32_t reg[4];
	fdt32_t *val;
	char name[32];

	addr_high = (u64)pma->pa >> 32;
	addr_low = pma->pa;
	size_high = (u64)pma->size >> 32;
	size_low = pma->size;

	if (na > 1 && addr_high)
		sbi_snprintf(name, sizeof(name),
			     "pma_resv%d@%x,%x", index,
			     addr_high, addr_low);
	else
		sbi_snprintf(name, sizeof(name),
			     "pma_resv%d@%x", index,
			     addr_low);

	subnode = fdt_add_subnode(fdt, parent, name);
	if (subnode < 0)
		return subnode;

	if (pma->shared_dma) {
		err = fdt_setprop_string(fdt, subnode, "compatible", "shared-dma-pool");
		if (err < 0)
			return err;
	}

	if (pma->no_map) {
		err = fdt_setprop_empty(fdt, subnode, "no-map");
		if (err < 0)
			return err;
	}

	/* Linux allows single linux,dma-default region. */
	if (pma->dma_default) {
		if (dma_default)
			return SBI_EINVAL;

		err = fdt_setprop_empty(fdt, subnode, "linux,dma-default");
		if (err < 0)
			return err;
		dma_default = true;
	}

	/* encode the <reg> property value */
	val = reg;
	if (na > 1)
		*val++ = cpu_to_fdt32(addr_high);
	*val++ = cpu_to_fdt32(addr_low);
	if (ns > 1)
		*val++ = cpu_to_fdt32(size_high);
	*val++ = cpu_to_fdt32(size_low);

	err = fdt_setprop(fdt, subnode, "reg", reg,
			  (na + ns) * sizeof(fdt32_t));
	if (err < 0)
		return err;

	return 0;
}

static int andes45_fdt_reserved_memory_fixup(void *fdt,
					     const struct andes45_pma_region *pma,
					     unsigned int entry)
{
	int parent;

	/* try to locate the reserved memory node */
	parent = fdt_path_offset(fdt, "/reserved-memory");
	if (parent < 0) {
		int na = fdt_address_cells(fdt, 0);
		int ns = fdt_size_cells(fdt, 0);
		int err;

		/* if such node does not exist, create one */
		parent = fdt_add_subnode(fdt, 0, "reserved-memory");
		if (parent < 0)
			return parent;

		err = fdt_setprop_empty(fdt, parent, "ranges");
		if (err < 0)
			return err;

		err = fdt_setprop_u32(fdt, parent, "#size-cells", ns);
		if (err < 0)
			return err;

		err = fdt_setprop_u32(fdt, parent, "#address-cells", na);
		if (err < 0)
			return err;
	}

	return andes45_fdt_pma_resv(fdt, pma, entry, parent);
}

int andes45_pma_setup_regions(const struct andes45_pma_region *pma_regions,
			      unsigned int pma_regions_count)
{
	unsigned long mmsc = csr_read(ANDES45_CSR_MMSC_CFG);
	unsigned int dt_populate_cnt;
	unsigned int i, j;
	unsigned long pa;
	void *fdt;
	int ret;

	if (!pma_regions || !pma_regions_count)
		return 0;

	if (pma_regions_count > ANDES45_MAX_PMA_REGIONS)
		return SBI_EINVAL;

	if ((mmsc & ANDES45_CSR_MMSC_PPMA_OFFSET) == 0)
		return SBI_ENOTSUPP;

	/* Configure the PMA regions */
	for (i = 0; i < pma_regions_count; i++) {
		pa = andes45_pma_setup(&pma_regions[i], i);
		if (pa == SBI_EINVAL)
			return SBI_EINVAL;
	}

	dt_populate_cnt = 0;
	for (i = 0; i < pma_regions_count; i++) {
		if (!pma_regions[i].dt_populate)
			continue;
		dt_populate_cnt++;
	}

	if (!dt_populate_cnt)
		return 0;

	fdt = fdt_get_address();

	ret = fdt_open_into(fdt, fdt, fdt_totalsize(fdt) + (64 * dt_populate_cnt));
	if (ret < 0)
		return ret;

	for (i = 0, j = 0; i < pma_regions_count; i++) {
		if (!pma_regions[i].dt_populate)
			continue;

		ret = andes45_fdt_reserved_memory_fixup(fdt, &pma_regions[i], j++);
		if (ret)
			return ret;
	}

	return 0;
}
