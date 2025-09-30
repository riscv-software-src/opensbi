// SPDX-License-Identifier: BSD-2-Clause
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

static inline bool not_napot(unsigned long addr, unsigned long size)
{
	return ((size & (size - 1)) || (addr & (size - 1)));
}

static inline bool is_pma_entry_disable(char pmaxcfg)
{
	return (pmaxcfg & ANDES_PMACFG_ETYP_MASK) == ANDES_PMACFG_ETYP_OFF ?
	       true : false;
}

static char get_pmaxcfg(int entry_id)
{
	unsigned int pmacfg_addr;
	unsigned long pmacfg_val;
	char *pmaxcfg;

#if __riscv_xlen == 64
	pmacfg_addr = CSR_PMACFG0 + ((entry_id / 8) ? 2 : 0);
	pmacfg_val = csr_read_num(pmacfg_addr);
	pmaxcfg = (char *)&pmacfg_val + (entry_id % 8);
#elif __riscv_xlen == 32
	pmacfg_addr = CSR_PMACFG0 + (entry_id / 4);
	pmacfg_val = csr_read_num(pmacfg_addr);
	pmaxcfg = (char *)&pmacfg_val + (entry_id % 4);
#else
#error "Unexpected __riscv_xlen"
#endif
	return *pmaxcfg;
}

static void set_pmaxcfg(int entry_id, char flags)
{
	unsigned int pmacfg_addr;
	unsigned long pmacfg_val;
	char *pmaxcfg;

#if __riscv_xlen == 64
	pmacfg_addr = CSR_PMACFG0 + ((entry_id / 8) ? 2 : 0);
	pmacfg_val = csr_read_num(pmacfg_addr);
	pmaxcfg = (char *)&pmacfg_val + (entry_id % 8);
#elif __riscv_xlen == 32
	pmacfg_addr = CSR_PMACFG0 + (entry_id / 4);
	pmacfg_val = csr_read_num(pmacfg_addr);
	pmaxcfg = (char *)&pmacfg_val + (entry_id % 4);
#else
#error "Unexpected __riscv_xlen"
#endif
	*pmaxcfg = flags;
	csr_write_num(pmacfg_addr, pmacfg_val);
}

static void decode_pmaaddrx(int entry_id, unsigned long *start,
			    unsigned long *size)
{
	unsigned long pmaaddr;
	int k;

	/**
	 * Given $pmaaddr, let k = # of trailing 1s of $pmaaddr
	 * size = 2 ^ (k + 3)
	 * start = 4 * ($pmaaddr - (size / 8) + 1)
	 */
	pmaaddr = csr_read_num(CSR_PMAADDR0 + entry_id);
	k = sbi_ffz(pmaaddr);
	*size = 1 << (k + 3);
	*start = (pmaaddr - (1 << k) + 1) << 2;
}

static bool has_pma_region_overlap(unsigned long start, unsigned long size)
{
	unsigned long _start, _size, _end, end;
	char pmaxcfg;

	end = start + size - 1;
	for (int i = 0; i < ANDES_MAX_PMA_REGIONS; i++) {
		pmaxcfg = get_pmaxcfg(i);
		if (is_pma_entry_disable(pmaxcfg))
			continue;

		decode_pmaaddrx(i, &_start, &_size);
		_end = _start + _size - 1;

		if (MAX(start, _start) <= MIN(end, _end)) {
			sbi_printf(
				"ERROR %s(): %#lx ~ %#lx overlaps with PMA%d: %#lx ~ %#lx\n",
				__func__, start, end, i, _start, _end);
			return true;
		}
	}

	return false;
}

static unsigned long andes_pma_setup(const struct andes_pma_region *pma_region,
				     unsigned int entry_id)
{
	unsigned long size = pma_region->size;
	unsigned long addr = pma_region->pa;
	unsigned long pmaaddr;

	/* Check for a 4KiB granularity NAPOT region*/
	if (size < ANDES_PMA_GRANULARITY || not_napot(addr, size) ||
	    !(pma_region->flags & ANDES_PMACFG_ETYP_NAPOT))
		return SBI_EINVAL;

	set_pmaxcfg(entry_id, pma_region->flags);

	pmaaddr = (addr >> 2) + (size >> 3) - 1;

	csr_write_num(CSR_PMAADDR0 + entry_id, pmaaddr);

	return csr_read_num(CSR_PMAADDR0 + entry_id) == pmaaddr ?
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

int andes_pma_setup_regions(void *fdt,
			    const struct andes_pma_region *pma_regions,
			    unsigned int pma_regions_count)
{
	unsigned int dt_populate_cnt;
	unsigned int i, j;
	unsigned long pa;
	int ret;

	if (!pma_regions || !pma_regions_count)
		return 0;

	if (pma_regions_count > ANDES_MAX_PMA_REGIONS)
		return SBI_EINVAL;

	if (!andes_sbi_probe_pma())
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

bool andes_sbi_probe_pma(void)
{
	return (csr_read(CSR_MMSC_CFG) & MMSC_CFG_PPMA_MASK) ? true : false;
}

int andes_sbi_set_pma(unsigned long pa, unsigned long size, u8 flags)
{
	unsigned int entry_id;
	unsigned long rc;
	char pmaxcfg;
	struct andes_pma_region region;

	if (!andes_sbi_probe_pma()) {
		sbi_printf("ERROR %s(): Platform does not support PPMA.\n",
			   __func__);
		return SBI_ERR_NOT_SUPPORTED;
	}

	if (has_pma_region_overlap(pa, size))
		return SBI_ERR_INVALID_PARAM;

	for (entry_id = 0; entry_id < ANDES_MAX_PMA_REGIONS; entry_id++) {
		pmaxcfg = get_pmaxcfg(entry_id);
		if (is_pma_entry_disable(pmaxcfg)) {
			region.pa = pa;
			region.size = size;
			region.flags = flags;
			break;
		}
	}

	if (entry_id == ANDES_MAX_PMA_REGIONS) {
		sbi_printf("ERROR %s(): All PMA entries have run out\n",
			   __func__);
		return SBI_ERR_FAILED;
	}

	rc = andes_pma_setup(&region, entry_id);
	if (rc == SBI_EINVAL) {
		sbi_printf("ERROR %s(): Failed to set PMAADDR%d\n",
			   __func__, entry_id);
		return SBI_ERR_FAILED;
	}

	return SBI_SUCCESS;
}

int andes_sbi_free_pma(unsigned long pa)
{
	unsigned long start, size;
	char pmaxcfg;

	if (!andes_sbi_probe_pma()) {
		sbi_printf("ERROR %s(): Platform does not support PPMA.\n",
			   __func__);
		return SBI_ERR_NOT_SUPPORTED;
	}

	for (int i = 0; i < ANDES_MAX_PMA_REGIONS; i++) {
		pmaxcfg = get_pmaxcfg(i);
		if (is_pma_entry_disable(pmaxcfg))
			continue;

		decode_pmaaddrx(i, &start, &size);
		if (start != pa)
			continue;

		set_pmaxcfg(i, ANDES_PMACFG_ETYP_OFF);
		csr_write_num(CSR_PMAADDR0 + i, 0);

		return SBI_SUCCESS;
	}

	sbi_printf("ERROR %s(): Failed to find the entry with PA %#lx\n",
		   __func__, pa);

	return SBI_ERR_FAILED;
}
