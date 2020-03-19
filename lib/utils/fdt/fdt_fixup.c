// SPDX-License-Identifier: BSD-2-Clause
/*
 * fdt_fixup.c - Flat Device Tree parsing helper routines
 * Implement helper routines to parse FDT nodes on top of
 * libfdt for OpenSBI usage
 *
 * Copyright (C) 2020 Bin Meng <bmeng.cn@gmail.com>
 */

#include <libfdt.h>
#include <sbi/riscv_asm.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_string.h>

void fdt_cpu_fixup(void *fdt)
{
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();
	const struct sbi_platform *plat = sbi_platform_ptr(scratch);
	int err, len, cpu_offset, cpus_offset;
	const fdt32_t *val;
	const void *prop;
	u32 hartid;

	err = fdt_open_into(fdt, fdt, fdt_totalsize(fdt) + 32);
	if (err < 0)
		return;

	cpus_offset = fdt_path_offset(fdt, "/cpus");
	if (cpus_offset < 0)
		return;

	fdt_for_each_subnode(cpu_offset, fdt, cpus_offset) {
		prop = fdt_getprop(fdt, cpu_offset, "device_type", &len);
		if (!prop || !len)
			continue;
		if (sbi_strcmp(prop, "cpu"))
			continue;

		val = fdt_getprop(fdt, cpu_offset, "reg", &len);
		if (!val || len < sizeof(fdt32_t))
			continue;

		if (len > sizeof(fdt32_t))
			val++;
		hartid = fdt32_to_cpu(*val);

		if (sbi_platform_hart_invalid(plat, hartid))
			fdt_setprop_string(fdt, cpu_offset, "status",
					   "disabled");
	}
}

void fdt_plic_fixup(void *fdt, const char *compat)
{
	u32 *cells;
	int i, cells_count;
	int plic_off;

	plic_off = fdt_node_offset_by_compatible(fdt, 0, compat);
	if (plic_off < 0)
		return;

	cells = (u32 *)fdt_getprop(fdt, plic_off,
				   "interrupts-extended", &cells_count);
	if (!cells)
		return;

	cells_count = cells_count / sizeof(u32);
	if (!cells_count)
		return;

	for (i = 0; i < (cells_count / 2); i++) {
		if (fdt32_to_cpu(cells[2 * i + 1]) == IRQ_M_EXT)
			cells[2 * i + 1] = cpu_to_fdt32(0xffffffff);
	}
}

/**
 * We use PMP to protect OpenSBI firmware to safe-guard it from buggy S-mode
 * software, see pmp_init() in lib/sbi/sbi_hart.c. The protected memory region
 * information needs to be conveyed to S-mode software (e.g.: operating system)
 * via some well-known method.
 *
 * With device tree, this can be done by inserting a child node of the reserved
 * memory node which is used to specify one or more regions of reserved memory.
 *
 * For the reserved memory node bindings, see Linux kernel documentation at
 * Documentation/devicetree/bindings/reserved-memory/reserved-memory.txt
 *
 * Some additional memory spaces may be protected by platform codes via PMP as
 * well, and corresponding child nodes will be inserted.
 */
int fdt_reserved_memory_fixup(void *fdt)
{
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();
	const struct sbi_platform *plat = sbi_platform_ptr(scratch);
	unsigned long prot, addr, size;
	int na = fdt_address_cells(fdt, 0);
	int ns = fdt_size_cells(fdt, 0);
	fdt32_t addr_high, addr_low;
	fdt32_t size_high, size_low;
	fdt32_t reg[4];
	fdt32_t *val;
	char name[32];
	int parent, subnode;
	int i, j;
	int err;

	if (!sbi_platform_has_pmp(plat))
		return 0;

	/* expand the device tree to accommodate new node */
	err  = fdt_open_into(fdt, fdt, fdt_totalsize(fdt) + 256);
	if (err < 0)
		return err;

	/* try to locate the reserved memory node */
	parent = fdt_path_offset(fdt, "/reserved-memory");
	if (parent < 0) {
		/* if such node does not exist, create one */
		parent = fdt_add_subnode(fdt, 0, "reserved-memory");
		if (parent < 0)
			return parent;

		/*
		 * reserved-memory node has 3 required properties:
		 * - #address-cells: the same value as the root node
		 * - #size-cells: the same value as the root node
		 * - ranges: should be empty
		 */

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

	/*
	 * We assume the given device tree does not contain any memory region
	 * child node protected by PMP. Normally PMP programming happens at
	 * M-mode firmware. The memory space used by OpenSBI is protected.
	 * Some additional memory spaces may be protected by platform codes.
	 *
	 * With above assumption, we create child nodes directly.
	 */

	for (i = 0, j = 0; i < PMP_COUNT; i++) {
		pmp_get(i, &prot, &addr, &size);
		if (!(prot & PMP_A))
			continue;
		if (!(prot & (PMP_R | PMP_W | PMP_X))) {
			addr_high = (u64)addr >> 32;
			addr_low = addr;
			size_high = (u64)size >> 32;
			size_low = size;

			if (na > 1 && addr_high)
				sbi_snprintf(name, sizeof(name),
					     "mmode_pmp%d@%x,%x", j,
					     addr_high, addr_low);
			else
				sbi_snprintf(name, sizeof(name),
					     "mmode_pmp%d@%x", j,
					     addr_low);

			subnode = fdt_add_subnode(fdt, parent, name);
			if (subnode < 0)
				return subnode;

			/*
			 * Tell operating system not to create a virtual
			 * mapping of the region as part of its standard
			 * mapping of system memory.
			 */
			err = fdt_setprop_empty(fdt, subnode, "no-map");
			if (err < 0)
				return err;

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

			j++;
		}
	}

	return 0;
}

void fdt_fixups(void *fdt)
{
	fdt_plic_fixup(fdt, "riscv,plic0");

	fdt_reserved_memory_fixup(fdt);
}


