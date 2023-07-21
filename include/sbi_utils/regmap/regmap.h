/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Anup Patel <apatel@ventanamicro.com>
 */

#ifndef __REGMAP_H__
#define __REGMAP_H__

#include <sbi/sbi_types.h>
#include <sbi/sbi_list.h>

/** Representation of a regmap instance */
struct regmap {
	/** Uniquie ID of the regmap instance assigned by the driver */
	unsigned int id;

	/** Configuration of regmap registers */
	int reg_shift;
	int reg_stride;
	unsigned int reg_base;
	unsigned int reg_max;

	/** Read a regmap register */
	int (*reg_read)(struct regmap *rmap, unsigned int reg,
			unsigned int *val);

	/** Write a regmap register */
	int (*reg_write)(struct regmap *rmap, unsigned int reg,
			 unsigned int val);

	/** Read-modify-write a regmap register */
	int (*reg_update_bits)(struct regmap *rmap, unsigned int reg,
			       unsigned int mask, unsigned int val);

	/** List */
	struct sbi_dlist node;
};

static inline struct regmap *to_regmap(struct sbi_dlist *node)
{
	return container_of(node, struct regmap, node);
}

/** Find a registered regmap instance */
struct regmap *regmap_find(unsigned int id);

/** Register a regmap instance */
int regmap_add(struct regmap *rmap);

/** Un-register a regmap instance */
void regmap_remove(struct regmap *rmap);

/** Read a register in a regmap instance */
int regmap_read(struct regmap *rmap, unsigned int reg, unsigned int *val);

/** Write a register in a regmap instance */
int regmap_write(struct regmap *rmap, unsigned int reg, unsigned int val);

/** Read-modify-write a register in a regmap instance */
int regmap_update_bits(struct regmap *rmap, unsigned int reg,
		       unsigned int mask, unsigned int val);

#endif
