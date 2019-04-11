/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#ifndef __FDT_H__
#define __FDT_H__

#include <sbi/sbi_types.h>

struct fdt_node {
	char *data;
	const struct fdt_node *parent;
	const char *name;
	int depth;
	int address_cells;
	int size_cells;
};

struct fdt_prop {
	const struct fdt_node *node;
	const char *name;
	void *value;
	u32 len;
};

/* Reverse byte-order of 32bit number */
u32 fdt_rev32(u32 v);

/* Length of a string */
ulong fdt_strlen(const char *str);

/* Compate two strings */
int fdt_strcmp(const char *a, const char *b);

/* Find index of matching string from a list of strings */
int fdt_prop_string_index(const struct fdt_prop *prop, const char *str);

/* Iterate over each property of matching node */
int fdt_match_node_prop(void *fdt,
			int (*match)(const struct fdt_node *node,
				     const struct fdt_prop *prop, void *priv),
			void *match_priv,
			void (*fn)(const struct fdt_node *node,
				   const struct fdt_prop *prop, void *priv),
			void *fn_priv);

/* Iterate over each property of compatible node */
int fdt_compat_node_prop(void *fdt, const char *compat,
			 void (*fn)(const struct fdt_node *node,
				    const struct fdt_prop *prop, void *priv),
			 void *fn_priv);

/* Iterate over each node and property */
int fdt_walk(void *fdt,
	     void (*fn)(const struct fdt_node *node,
			const struct fdt_prop *prop, void *priv),
	     void *fn_priv);

/* Get size of FDT */
u32 fdt_size(void *fdt);

#endif
