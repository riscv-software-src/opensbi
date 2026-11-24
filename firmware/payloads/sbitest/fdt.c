/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * The harts in the device tree.
 *
 * fdt_path_offset() of libfdt finds /cpus. The children of /cpus are the
 * harts, but there can be other nodes too, so only those with the string
 * "riscv" in their compatible property count. Their reg property is the hart
 * ID. The address-cells of /cpus tell how many 32-bit cells that are.
 */

#include <libfdt.h>
#include <sbi/sbi_string.h>
#include <sbi/sbi_types.h>
#include "hart.h"
#include "heap.h"
#include "sbitest.h"

/*
 * riscv,isa-base is a string that starts with rv followed by 32 or 64, for
 * example "rv64i". Return 32 or 64, or 0 if the property is missing or has
 * another value.
 */
static unsigned int parse_isa_base(const void *fdt, int node)
{
	const char *base;
	int len;

	base = fdt_getprop(fdt, node, "riscv,isa-base", &len);
	if (!base || len < 5)
		return 0;

	if (base[0] != 'r' || base[1] != 'v')
		return 0;
	if (base[2] == '3' && base[3] == '2')
		return 32;
	if (base[2] == '6' && base[3] == '4')
		return 64;

	return 0;
}

/*
 * Sdtrig is listed in riscv,isa-extensions, a list of strings, or in the
 * legacy riscv,isa, one string with the extensions separated by underscores.
 */
static unsigned int parse_sdtrig(const void *fdt, int node)
{
	const char *p, *end;
	int len;

	p = fdt_getprop(fdt, node, "riscv,isa-extensions", &len);
	if (p)
		return fdt_stringlist_contains(p, len, "sdtrig");

	p = fdt_getprop(fdt, node, "riscv,isa", &len);
	if (!p)
		return 0;
	end = p + len;

	/* Skip the base, e.g. rv64imafdc, extensions follow after '_' */
	while (p < end && *p && *p != '_')
		p++;
	while (p < end && *p == '_') {
		p++;
		if (end - p >= 6 && !sbi_memcmp(p, "sdtrig", 6) &&
		    (p + 6 == end || p[6] == '_' || !p[6]))
			return 1;
		while (p < end && *p && *p != '_')
			p++;
	}

	return 0;
}

struct hart *harts_parse(const void *fdt, unsigned int *count)
{
	struct hart *head = NULL, **tail = &head, *hart;
	unsigned long long id;
	const fdt32_t *reg;
	const char *compatible;
	int cpus, cpu, cells, len, i;

	*count = 0;

	cpus = fdt_path_offset(fdt, "/cpus");
	if (cpus < 0)
		return NULL;

	/* The reg properties of the harts are as wide as this */
	cells = fdt_address_cells(fdt, cpus);
	if (cells < 1 || cells > 2)
		return NULL;

	for (cpu = fdt_first_subnode(fdt, cpus); cpu >= 0;
	     cpu = fdt_next_subnode(fdt, cpu)) {
		compatible = fdt_getprop(fdt, cpu, "compatible", &len);
		if (!compatible ||
		    !fdt_stringlist_contains(compatible, len, "riscv"))
			continue;

		reg = fdt_getprop(fdt, cpu, "reg", &len);
		if (!reg || len < cells * (int)sizeof(fdt32_t))
			continue;

		id = 0;
		for (i = 0; i < cells; i++)
			id = (id << 32) | fdt32_to_cpu(reg[i]);
		/* A hart ID that does not fit into a register is useless */
		if ((unsigned long)id != id)
			continue;

		hart = malloc(sizeof(*hart));
		if (!hart)
			test_abort("out of memory for the harts");

		hart->next	= NULL;
		hart->hartid	= id;
		hart->stack_top = 0;
		hart->bitness	= parse_isa_base(fdt, cpu);
		hart->started	= 0;
		hart->sdtrig	= parse_sdtrig(fdt, cpu);

		*tail = hart;
		tail  = &hart->next;
		(*count)++;
	}

	return head;
}

void harts_free(struct hart *list)
{
	struct hart *next;

	for (; list; list = next) {
		next = list->next;
		free(list);
	}
}
