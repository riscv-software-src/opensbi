// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Renesas Electronics Corp.
 * Copyright (c) 2024 Andes Technology Corporation
 *
 * Authors:
 *      Ben Zong-You Xie <ben717@andestech.com>
 *      Lad Prabhakar <prabhakar.mahadev-lad.rj@bp.renesas.com>
 */

#include <andes/andes.h>
#include <andes/andes_pma.h>
#include <libfdt.h>
#include <sbi/riscv_asm.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_error.h>
#include <sbi_utils/fdt/fdt_helper.h>

static unsigned long andes_pma_read_num(unsigned int csr_num)
{
#define switchcase_csr_read(__csr_num, __val)		\
	case __csr_num:					\
		__val = csr_read(__csr_num);		\
		break;
#define switchcase_csr_read_2(__csr_num, __val)		\
	switchcase_csr_read(__csr_num + 0, __val)	\
	switchcase_csr_read(__csr_num + 1, __val)
#define switchcase_csr_read_4(__csr_num, __val)		\
	switchcase_csr_read_2(__csr_num + 0, __val)	\
	switchcase_csr_read_2(__csr_num + 2, __val)
#define switchcase_csr_read_8(__csr_num, __val)		\
	switchcase_csr_read_4(__csr_num + 0, __val)	\
	switchcase_csr_read_4(__csr_num + 4, __val)
#define switchcase_csr_read_16(__csr_num, __val)	\
	switchcase_csr_read_8(__csr_num + 0, __val)	\
	switchcase_csr_read_8(__csr_num + 8, __val)

	unsigned long ret = 0;

	switch (csr_num) {
	switchcase_csr_read_4(CSR_PMACFG0, ret)
	switchcase_csr_read_16(CSR_PMAADDR0, ret)
	default:
		sbi_panic("%s: Unknown Andes PMA CSR %#x", __func__, csr_num);
		break;
	}

	return ret;

#undef switchcase_csr_read_16
#undef switchcase_csr_read_8
#undef switchcase_csr_read_4
#undef switchcase_csr_read_2
#undef switchcase_csr_read
}

static void andes_pma_write_num(unsigned int csr_num, unsigned long val)
{
#define switchcase_csr_write(__csr_num, __val)		\
	case __csr_num:					\
		csr_write(__csr_num, __val);		\
		break;
#define switchcase_csr_write_2(__csr_num, __val)	\
	switchcase_csr_write(__csr_num + 0, __val)	\
	switchcase_csr_write(__csr_num + 1, __val)
#define switchcase_csr_write_4(__csr_num, __val)	\
	switchcase_csr_write_2(__csr_num + 0, __val)	\
	switchcase_csr_write_2(__csr_num + 2, __val)
#define switchcase_csr_write_8(__csr_num, __val)	\
	switchcase_csr_write_4(__csr_num + 0, __val)	\
	switchcase_csr_write_4(__csr_num + 4, __val)
#define switchcase_csr_write_16(__csr_num, __val)	\
	switchcase_csr_write_8(__csr_num + 0, __val)	\
	switchcase_csr_write_8(__csr_num + 8, __val)

	switch (csr_num) {
	switchcase_csr_write_4(CSR_PMACFG0, val)
	switchcase_csr_write_16(CSR_PMAADDR0, val)
	default:
		sbi_panic("%s: Unknown Andes PMA CSR %#x", __func__, csr_num);
		break;
	}

#undef switchcase_csr_write_16
#undef switchcase_csr_write_8
#undef switchcase_csr_write_4
#undef switchcase_csr_write_2
#undef switchcase_csr_write
}

static inline bool not_napot(unsigned long addr, unsigned long size)
{
	return ((size & (size - 1)) || (addr & (size - 1)));
}

static unsigned long andes_pma_setup(const struct andes_pma_region *pma_region,
				     unsigned int entry_id)
{
	unsigned long size = pma_region->size;
	unsigned long addr = pma_region->pa;
	unsigned int pma_cfg_addr;
	unsigned long pmacfg_val;
	unsigned long pmaaddr;
	char *pmaxcfg;

	/* Check for a 4KiB granularity NAPOT region*/
	if (size < ANDES_PMA_GRANULARITY || not_napot(addr, size) ||
	    !(pma_region->flags & ANDES_PMACFG_ETYP_NAPOT))
		return SBI_EINVAL;

#if __riscv_xlen == 64
	pma_cfg_addr = CSR_PMACFG0 + ((entry_id / 8) ? 2 : 0);
	pmacfg_val = andes_pma_read_num(pma_cfg_addr);
	pmaxcfg = (char *)&pmacfg_val + (entry_id % 8);
#elif __riscv_xlen == 32
	pma_cfg_addr = CSR_PMACFG0 + (entry_id / 4);
	pmacfg_val = andes_pma_read_num(pma_cfg_addr);
	pmaxcfg = (char *)&pmacfg_val + (entry_id % 4);
#else
#error "Unexpected __riscv_xlen"
#endif
	*pmaxcfg = pma_region->flags;

	andes_pma_write_num(pma_cfg_addr, pmacfg_val);

	pmaaddr = (addr >> 2) + (size >> 3) - 1;

	andes_pma_write_num(CSR_PMAADDR0 + entry_id, pmaaddr);

	return andes_pma_read_num(CSR_PMAADDR0 + entry_id) == pmaaddr ?
	       pmaaddr : SBI_EINVAL;
}

static int andes_fdt_pma_resv(void *fdt, const struct andes_pma_region *pma,
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

	if (na > 1 && addr_high) {
		sbi_snprintf(name, sizeof(name),
			     "pma_resv%d@%x,%x",
			     index, addr_high, addr_low);
	} else {
		sbi_snprintf(name, sizeof(name),
			     "pma_resv%d@%x",
			     index, addr_low);
	}
	subnode = fdt_add_subnode(fdt, parent, name);
	if (subnode < 0)
		return subnode;

	if (pma->shared_dma) {
		err = fdt_setprop_string(fdt, subnode, "compatible",
					 "shared-dma-pool");
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

	/* Encode the <reg> property value */
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

static int andes_fdt_reserved_memory_fixup(void *fdt,
					   const struct andes_pma_region *pma,
					   unsigned int entry)
{
	int parent;

	/* Try to locate the reserved memory node */
	parent = fdt_path_offset(fdt, "/reserved-memory");
	if (parent < 0) {
		int na = fdt_address_cells(fdt, 0);
		int ns = fdt_size_cells(fdt, 0);
		int err;

		/* If such node does not exist, create one */
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

	return andes_fdt_pma_resv(fdt, pma, entry, parent);
}

int andes_pma_setup_regions(const struct andes_pma_region *pma_regions,
			    unsigned int pma_regions_count)
{
	unsigned long mmsc = csr_read(CSR_MMSC_CFG);
	unsigned int dt_populate_cnt;
	unsigned int i, j;
	unsigned long pa;
	void *fdt;
	int ret;

	if (!pma_regions || !pma_regions_count)
		return 0;

	if (pma_regions_count > ANDES_MAX_PMA_REGIONS)
		return SBI_EINVAL;

	if ((mmsc & MMSC_CFG_PPMA_MASK) == 0)
		return SBI_ENOTSUPP;

	/* Configure the PMA regions */
	dt_populate_cnt = 0;
	for (i = 0; i < pma_regions_count; i++) {
		pa = andes_pma_setup(&pma_regions[i], i);
		if (pa == SBI_EINVAL)
			return SBI_EINVAL;
		else if (pma_regions[i].dt_populate)
			dt_populate_cnt++;
	}

	if (!dt_populate_cnt)
		return 0;

	fdt = fdt_get_address();

	ret = fdt_open_into(fdt, fdt,
			    fdt_totalsize(fdt) + (64 * dt_populate_cnt));
	if (ret < 0)
		return ret;

	for (i = 0, j = 0; i < pma_regions_count; i++) {
		if (!pma_regions[i].dt_populate)
			continue;

		ret = andes_fdt_reserved_memory_fixup(fdt,
						      &pma_regions[i],
						      j++);
		if (ret)
			return ret;
	}

	return 0;
}
