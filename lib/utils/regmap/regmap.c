/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Anup Patel <apatel@ventanamicro.com>
 */

#include <sbi/sbi_error.h>
#include <sbi_utils/regmap/regmap.h>

static SBI_LIST_HEAD(regmap_list);

struct regmap *regmap_find(unsigned int id)
{
	struct sbi_dlist *pos;

	sbi_list_for_each(pos, &(regmap_list)) {
		struct regmap *rmap = to_regmap(pos);

		if (rmap->id == id)
			return rmap;
	}

	return NULL;
}

int regmap_add(struct regmap *rmap)
{
	if (!rmap)
		return SBI_EINVAL;
	if (regmap_find(rmap->id))
		return SBI_EALREADY;

	sbi_list_add(&(rmap->node), &(regmap_list));

	return 0;
}

void regmap_remove(struct regmap *rmap)
{
	if (!rmap)
		return;

	sbi_list_del(&(rmap->node));
}

static bool regmap_reg_valid(struct regmap *rmap, unsigned int reg)
{
	if ((reg >= rmap->reg_max) ||
	    (reg & (rmap->reg_stride - 1)))
		return false;
	return true;
}

static unsigned int regmap_reg_addr(struct regmap *rmap, unsigned int reg)
{
	reg += rmap->reg_base;

	if (rmap->reg_shift > 0)
		reg >>= rmap->reg_shift;
	else if (rmap->reg_shift < 0)
		reg <<= -(rmap->reg_shift);

	return reg;
}

int regmap_read(struct regmap *rmap, unsigned int reg, unsigned int *val)
{
	if (!rmap || !regmap_reg_valid(rmap, reg))
		return SBI_EINVAL;
	if (!rmap->reg_read)
		return SBI_ENOSYS;

	return rmap->reg_read(rmap, regmap_reg_addr(rmap, reg), val);
}

int regmap_write(struct regmap *rmap, unsigned int reg, unsigned int val)
{
	if (!rmap || !regmap_reg_valid(rmap, reg))
		return SBI_EINVAL;
	if (!rmap->reg_write)
		return SBI_ENOSYS;

	return rmap->reg_write(rmap, regmap_reg_addr(rmap, reg), val);
}

int regmap_update_bits(struct regmap *rmap, unsigned int reg,
		       unsigned int mask, unsigned int val)
{
	int rc;
	unsigned int reg_val;

	if (!rmap || !regmap_reg_valid(rmap, reg))
		return SBI_EINVAL;

	if (rmap->reg_update_bits) {
		return rmap->reg_update_bits(rmap, regmap_reg_addr(rmap, reg),
					     mask, val);
	} else if (rmap->reg_read && rmap->reg_write) {
		reg = regmap_reg_addr(rmap, reg);

		rc = rmap->reg_read(rmap, reg, &reg_val);
		if (rc)
			return rc;

		reg_val &= ~mask;
		reg_val |= val & mask;
		return rmap->reg_write(rmap, reg, reg_val);
	}

	return SBI_ENOSYS;
}
