/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Anup Patel <apatel@ventanamicro.com>
 */

#include <libfdt.h>
#include <sbi/riscv_asm.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_byteorder.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_heap.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/regmap/fdt_regmap.h>

enum syscon_regmap_endian {
	SYSCON_ENDIAN_NATIVE = 0,
	SYSCON_ENDIAN_LITTLE,
	SYSCON_ENDIAN_BIG,
	SYSCON_ENDIAN_MAX
};

struct syscon_regmap {
	u32 reg_io_width;
	enum syscon_regmap_endian reg_endian;
	unsigned long addr;
	struct regmap rmap;
};

#define to_syscon_regmap(__rmap)	\
	container_of((__rmap), struct syscon_regmap, rmap)

static int regmap_syscon_read_8(struct regmap *rmap, unsigned int reg,
				unsigned int *val)
{
	struct syscon_regmap *srm = to_syscon_regmap(rmap);

	*val = readb((volatile void *)(srm->addr + reg));
	return 0;
}

static int regmap_syscon_write_8(struct regmap *rmap, unsigned int reg,
				 unsigned int val)
{
	struct syscon_regmap *srm = to_syscon_regmap(rmap);

	writeb(val, (volatile void *)(srm->addr + reg));
	return 0;
}

static int regmap_syscon_read_16(struct regmap *rmap, unsigned int reg,
				unsigned int *val)
{
	struct syscon_regmap *srm = to_syscon_regmap(rmap);

	*val = readw((volatile void *)(srm->addr + reg));
	return 0;
}

static int regmap_syscon_write_16(struct regmap *rmap, unsigned int reg,
				 unsigned int val)
{
	struct syscon_regmap *srm = to_syscon_regmap(rmap);

	writew(val, (volatile void *)(srm->addr + reg));
	return 0;
}

static int regmap_syscon_read_32(struct regmap *rmap, unsigned int reg,
				unsigned int *val)
{
	struct syscon_regmap *srm = to_syscon_regmap(rmap);

	*val = readl((volatile void *)(srm->addr + reg));
	return 0;
}

static int regmap_syscon_write_32(struct regmap *rmap, unsigned int reg,
				 unsigned int val)
{
	struct syscon_regmap *srm = to_syscon_regmap(rmap);

	writel(val, (volatile void *)(srm->addr + reg));
	return 0;
}

static int regmap_syscon_read_le16(struct regmap *rmap, unsigned int reg,
				   unsigned int *val)
{
	struct syscon_regmap *srm = to_syscon_regmap(rmap);

	*val = le16_to_cpu(readw((volatile void *)(srm->addr + reg)));
	return 0;
}

static int regmap_syscon_write_le16(struct regmap *rmap, unsigned int reg,
				    unsigned int val)
{
	struct syscon_regmap *srm = to_syscon_regmap(rmap);

	writew(cpu_to_le16(val), (volatile void *)(srm->addr + reg));
	return 0;
}

static int regmap_syscon_read_le32(struct regmap *rmap, unsigned int reg,
				   unsigned int *val)
{
	struct syscon_regmap *srm = to_syscon_regmap(rmap);

	*val = le32_to_cpu(readl((volatile void *)(srm->addr + reg)));
	return 0;
}

static int regmap_syscon_write_le32(struct regmap *rmap, unsigned int reg,
				    unsigned int val)
{
	struct syscon_regmap *srm = to_syscon_regmap(rmap);

	writel(cpu_to_le32(val), (volatile void *)(srm->addr + reg));
	return 0;
}

static int regmap_syscon_read_be16(struct regmap *rmap, unsigned int reg,
				   unsigned int *val)
{
	struct syscon_regmap *srm = to_syscon_regmap(rmap);

	*val = be16_to_cpu(readl((volatile void *)(srm->addr + reg)));
	return 0;
}

static int regmap_syscon_write_be16(struct regmap *rmap, unsigned int reg,
				    unsigned int val)
{
	struct syscon_regmap *srm = to_syscon_regmap(rmap);

	writel(cpu_to_be16(val), (volatile void *)(srm->addr + reg));
	return 0;
}

static int regmap_syscon_read_be32(struct regmap *rmap, unsigned int reg,
				   unsigned int *val)
{
	struct syscon_regmap *srm = to_syscon_regmap(rmap);

	*val = be32_to_cpu(readl((volatile void *)(srm->addr + reg)));
	return 0;
}

static int regmap_syscon_write_be32(struct regmap *rmap, unsigned int reg,
				    unsigned int val)
{
	struct syscon_regmap *srm = to_syscon_regmap(rmap);

	writel(cpu_to_be32(val), (volatile void *)(srm->addr + reg));
	return 0;
}

static int regmap_syscon_init(const void *fdt, int nodeoff,
			      const struct fdt_match *match)
{
	struct syscon_regmap *srm;
	uint64_t addr, size;
	const fdt32_t *val;
	int rc, len;

	srm = sbi_zalloc(sizeof(*srm));
	if (!srm)
		return SBI_ENOMEM;

	val = fdt_getprop(fdt, nodeoff, "reg-io-width", &len);
	srm->reg_io_width = val ? fdt32_to_cpu(*val) : 4;

	if (fdt_getprop(fdt, nodeoff, "native-endian", &len))
		srm->reg_endian = SYSCON_ENDIAN_NATIVE;
	else if (fdt_getprop(fdt, nodeoff, "little-endian", &len))
		srm->reg_endian = SYSCON_ENDIAN_LITTLE;
	else if (fdt_getprop(fdt, nodeoff, "big-endian", &len))
		srm->reg_endian = SYSCON_ENDIAN_BIG;
	else
		srm->reg_endian = SYSCON_ENDIAN_NATIVE;

	rc = fdt_get_node_addr_size(fdt, nodeoff, 0, &addr, &size);
	if (rc)
		goto fail_free_syscon;
	srm->addr = addr;

	srm->rmap.id = nodeoff;
	srm->rmap.reg_shift = 0;
	srm->rmap.reg_stride = srm->reg_io_width * 8;
	srm->rmap.reg_base = 0;
	srm->rmap.reg_max = size / srm->reg_io_width;
	switch (srm->reg_io_width) {
	case 1:
		srm->rmap.reg_read = regmap_syscon_read_8;
		srm->rmap.reg_write = regmap_syscon_write_8;
		break;
	case 2:
		switch (srm->reg_endian) {
		case SYSCON_ENDIAN_NATIVE:
			srm->rmap.reg_read = regmap_syscon_read_16;
			srm->rmap.reg_write = regmap_syscon_write_16;
			break;
		case SYSCON_ENDIAN_LITTLE:
			srm->rmap.reg_read = regmap_syscon_read_le16;
			srm->rmap.reg_write = regmap_syscon_write_le16;
			break;
		case SYSCON_ENDIAN_BIG:
			srm->rmap.reg_read = regmap_syscon_read_be16;
			srm->rmap.reg_write = regmap_syscon_write_be16;
			break;
		default:
			rc = SBI_EINVAL;
			goto fail_free_syscon;
		}
		break;
	case 4:
		switch (srm->reg_endian) {
		case SYSCON_ENDIAN_NATIVE:
			srm->rmap.reg_read = regmap_syscon_read_32;
			srm->rmap.reg_write = regmap_syscon_write_32;
			break;
		case SYSCON_ENDIAN_LITTLE:
			srm->rmap.reg_read = regmap_syscon_read_le32;
			srm->rmap.reg_write = regmap_syscon_write_le32;
			break;
		case SYSCON_ENDIAN_BIG:
			srm->rmap.reg_read = regmap_syscon_read_be32;
			srm->rmap.reg_write = regmap_syscon_write_be32;
			break;
		default:
			rc = SBI_EINVAL;
			goto fail_free_syscon;
		}
		break;
	default:
		rc = SBI_EINVAL;
		goto fail_free_syscon;
	}

	rc = sbi_domain_root_add_memrange(addr, size, PAGE_SIZE,
				(SBI_DOMAIN_MEMREGION_MMIO |
				 SBI_DOMAIN_MEMREGION_SHARED_SURW_MRW));
	if (rc)
		goto fail_free_syscon;

	rc = regmap_add(&srm->rmap);
	if (rc)
		goto fail_free_syscon;

	return 0;

fail_free_syscon:
	sbi_free(srm);
	return rc;
}

static const struct fdt_match regmap_syscon_match[] = {
	{ .compatible = "syscon" },
	{ },
};

const struct fdt_driver fdt_regmap_syscon = {
	.match_table = regmap_syscon_match,
	.init = regmap_syscon_init,
};
