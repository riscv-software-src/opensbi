/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Tests of the hart list that is built from the device tree.
 */

#include <libfdt.h>
#include <sbi/sbi_console.h>
#include "check.h"
#include "hart.h"
#include "heap.h"
#include "sbitest.h"

#define TREE_SIZE 2048

/* Cells of a reg property, in the byte order of the device tree */
static void cells(fdt32_t *out, unsigned int a, unsigned int b)
{
	out[0] = cpu_to_fdt32(a);
	out[1] = cpu_to_fdt32(b);
}

/* One hart node. Missing members are left out of the node. */
static int add_cpu(void *fdt, const char *name, const char *compatible,
		   int compatible_len, const fdt32_t *reg, int reg_len,
		   const char *isa_base)
{
	int err = fdt_begin_node(fdt, name);

	if (err)
		return err;
	if (compatible)
		err = fdt_property(fdt, "compatible", compatible,
				   compatible_len);
	if (!err && reg)
		err = fdt_property(fdt, "reg", reg, reg_len);
	if (!err && isa_base)
		err = fdt_property_string(fdt, "riscv,isa-base", isa_base);
	if (err)
		return err;

	return fdt_end_node(fdt);
}

/* Start a tree with /cpus, address_cells is the #address-cells of /cpus */
static int tree_begin(void *fdt, int address_cells)
{
	int err = fdt_create(fdt, TREE_SIZE);

	if (!err)
		err = fdt_finish_reservemap(fdt);
	if (!err)
		err = fdt_begin_node(fdt, "");
	if (!err)
		err = fdt_begin_node(fdt, "cpus");
	if (!err)
		err = fdt_property_u32(fdt, "#address-cells", address_cells);
	if (!err)
		err = fdt_property_u32(fdt, "#size-cells", 0);

	return err;
}

static int tree_end(void *fdt)
{
	int err = fdt_end_node(fdt);

	if (!err)
		err = fdt_end_node(fdt);
	if (!err)
		err = fdt_finish(fdt);

	return err;
}

/* Children of /cpus that are harts and others that are not */
static unsigned int test_mixed_tree(void)
{
	fdt32_t id0[1], id1[1], id2[1], id3[1], id4[1], id5[1];
	static const char riscv[]	 = "riscv";
	static const char vendor_riscv[] = "vendor,core\0riscv";
	static const char riscv_prefix[] = "riscv-core";
	static const char cache[]	 = "cache";
	unsigned int failed = 0, count = 0;
	struct hart *list, *hart;
	void *fdt = malloc(TREE_SIZE);
	int err;

	CHECK(fdt);
	if (!fdt)
		return failed;

	id0[0] = cpu_to_fdt32(0);
	id1[0] = cpu_to_fdt32(1);
	id2[0] = cpu_to_fdt32(2);
	id3[0] = cpu_to_fdt32(3);
	id4[0] = cpu_to_fdt32(4);
	id5[0] = cpu_to_fdt32(5);

	err = tree_begin(fdt, 1);
	/* The harts, 64 bit and 32 bit, with and without isa-base */
	if (!err)
		err = add_cpu(fdt, "cpu@0", riscv, sizeof(riscv), id0,
			      sizeof(id0), "rv64i");
	if (!err)
		err = add_cpu(fdt, "cpu@1", riscv, sizeof(riscv), id1,
			      sizeof(id1), "rv32i");
	if (!err)
		err = add_cpu(fdt, "cpu@2", riscv, sizeof(riscv), id2,
			      sizeof(id2), NULL);
	/* Not a hart: a cache has no riscv compatible */
	if (!err)
		err = add_cpu(fdt, "l2-cache", cache, sizeof(cache), id5,
			      sizeof(id5), NULL);
	/* "riscv" is one of two strings, so it is a hart */
	if (!err)
		err = add_cpu(fdt, "cpu@3", vendor_riscv, sizeof(vendor_riscv),
			      id3, sizeof(id3), "rv64i");
	/* Not a hart: riscv-core is not the string riscv */
	if (!err)
		err = add_cpu(fdt, "cpu@4", riscv_prefix, sizeof(riscv_prefix),
			      id4, sizeof(id4), "rv64i");
	/* Not usable: no reg, no hart ID */
	if (!err)
		err = add_cpu(fdt, "cpu@5", riscv, sizeof(riscv), NULL, 0,
			      "rv64i");
	if (!err)
		err = tree_end(fdt);
	CHECK(!err);

	list = harts_parse(fdt, &count);
	CHECK(count == 4);

	/* The list is in the order of the nodes */
	hart = list;
	CHECK(hart && hart->hartid == 0 && hart->bitness == 64);
	hart = hart ? hart->next : NULL;
	CHECK(hart && hart->hartid == 1 && hart->bitness == 32);
	hart = hart ? hart->next : NULL;
	CHECK(hart && hart->hartid == 2 && hart->bitness == 0);
	hart = hart ? hart->next : NULL;
	CHECK(hart && hart->hartid == 3 && hart->bitness == 64);
	hart = hart ? hart->next : NULL;
	CHECK(!hart);

	/* Only harts of the own bitness can run this payload */
	CHECK(list && list->next && hart_can_run(list) == (OWN_BITNESS == 64));
	CHECK(list && list->next &&
	      hart_can_run(list->next) == (OWN_BITNESS == 32));
	CHECK(list && list->next && list->next->next &&
	      !hart_can_run(list->next->next));

	harts_free(list);
	free(fdt);

	return failed;
}

/* Hart IDs that are two cells wide */
static unsigned int test_two_cells(void)
{
	fdt32_t wide[2], narrow[2];
	static const char riscv[] = "riscv";
	unsigned int failed = 0, count = 0;
	struct hart *list;
	void *fdt = malloc(TREE_SIZE);
	int err;

	CHECK(fdt);
	if (!fdt)
		return failed;

	cells(wide, 1, 2);
	cells(narrow, 0, 7);

	err = tree_begin(fdt, 2);
	if (!err)
		err = add_cpu(fdt, "cpu@0", riscv, sizeof(riscv), narrow,
			      sizeof(narrow), "rv64i");
	if (!err)
		err = add_cpu(fdt, "cpu@1", riscv, sizeof(riscv), wide,
			      sizeof(wide), "rv64i");
	if (!err)
		err = tree_end(fdt);
	CHECK(!err);

	list = harts_parse(fdt, &count);
	CHECK(list && list->hartid == 7);
	/* An ID above 32 bit is only usable if a register can hold it */
	if (sizeof(unsigned long) == 8) {
		CHECK(count == 2);
		CHECK(list && list->next &&
		      list->next->hartid == 0x100000002UL);
	} else {
		CHECK(count == 1);
	}

	harts_free(list);
	free(fdt);

	return failed;
}

/* No /cpus, or a tree that is not a tree */
static unsigned int test_no_cpus(void)
{
	unsigned int failed = 0, count = 99;
	void *fdt = malloc(TREE_SIZE);
	int err;

	CHECK(fdt);
	if (!fdt)
		return failed;

	err = fdt_create(fdt, TREE_SIZE);
	if (!err)
		err = fdt_finish_reservemap(fdt);
	if (!err)
		err = fdt_begin_node(fdt, "");
	if (!err)
		err = fdt_end_node(fdt);
	if (!err)
		err = fdt_finish(fdt);
	CHECK(!err);

	CHECK(!harts_parse(fdt, &count));
	CHECK(count == 0);

	free(fdt);

	return failed;
}

/* The device tree that OpenSBI passed to the payload */
unsigned int test_fdt_harts(const void *fdt, unsigned long boot_hartid)
{
	unsigned int failed = 0;
	struct hart *hart, *other;
	unsigned int n = 0;

	failed += test_mixed_tree();
	failed += test_two_cells();
	failed += test_no_cpus();

	/* The real tree has the boot hart, and it has our bitness */
	hart = hart_find(boot_hartid);
	CHECK(hart);
	CHECK(hart && hart->bitness == OWN_BITNESS);
	CHECK(hart_count() > 0);

	/* Every ID occurs once */
	for (hart = hart_first(); hart; hart = hart->next) {
		n++;
		for (other = hart->next; other; other = other->next)
			CHECK(other->hartid != hart->hartid);
	}
	CHECK(n == hart_count());

	return failed;
}
