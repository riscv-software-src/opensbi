/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <plat/string.h>
#include <plat/tinyfdt.h>

#define FDT_MAGIC 0xd00dfeed
#define FDT_VERSION 17

struct fdt_header {
	u32 magic;
	u32 totalsize;
	u32 off_dt_struct;
	u32 off_dt_strings;
	u32 off_mem_rsvmap;
	u32 version;
	u32 last_comp_version; /* <= 17 */
	u32 boot_cpuid_phys;
	u32 size_dt_strings;
	u32 size_dt_struct;
} __attribute__((packed));

#define FDT_BEGIN_NODE 1
#define FDT_END_NODE 2
#define FDT_PROP 3
#define FDT_NOP 4
#define FDT_END 9

u32 fdt_rev32(u32 v)
{
	return ((v & 0x000000FF) << 24) | ((v & 0x0000FF00) << 8) |
	       ((v & 0x00FF0000) >> 8) | ((v & 0xFF000000) >> 24);
}

int fdt_prop_string_index(const struct fdt_prop *prop, const char *str)
{
	int i;
	ulong l = 0;
	const char *p, *end;

	p   = prop->value;
	end = p + prop->len;

	for (i = 0; p < end; i++, p += l) {
		l = strlen(p) + 1;
		if (p + l > end)
			return -1;
		if (strcmp(str, p) == 0)
			return i; /* Found it; return index */
	}

	return -1;
}

struct recursive_iter_info {
	void (*fn)(const struct fdt_node *node, const struct fdt_prop *prop,
		   void *priv);
	void *fn_priv;
	const char *str;
};

#define DATA32(ptr) fdt_rev32(*((u32 *)ptr))

static void recursive_iter(char **data, struct recursive_iter_info *info,
			   const struct fdt_node *parent)
{
	struct fdt_node node;
	struct fdt_prop prop;

	if (DATA32(*data) != FDT_BEGIN_NODE)
		return;

	node.data = *data;

	(*data) += sizeof(u32);

	node.parent = parent;
	node.name   = *data;

	*data += strlen(*data) + 1;
	while ((ulong)(*data) % sizeof(u32) != 0)
		(*data)++;

	node.depth = (parent) ? (parent->depth + 1) : 1;

	/* Default cell counts, as per the FDT spec */
	node.address_cells = 2;
	node.size_cells	   = 1;

	info->fn(&node, NULL, info->fn_priv);

	while (DATA32(*data) != FDT_END_NODE) {
		switch (DATA32(*data)) {
		case FDT_PROP:
			prop.node = &node;
			*data += sizeof(u32);
			prop.len = DATA32(*data);
			*data += sizeof(u32);
			prop.name = &info->str[DATA32(*data)];
			*data += sizeof(u32);
			prop.value = *data;
			*data += prop.len;
			while ((ulong)(*data) % sizeof(u32) != 0)
				(*data)++;
			info->fn(&node, &prop, info->fn_priv);
			break;
		case FDT_NOP:
			*data += sizeof(u32);
			break;
		case FDT_BEGIN_NODE:
			recursive_iter(data, info, &node);
			break;
		default:
			return;
		};
	}

	*data += sizeof(u32);
}

struct match_iter_info {
	int (*match)(const struct fdt_node *node, const struct fdt_prop *prop,
		     void *priv);
	void *match_priv;
	void (*fn)(const struct fdt_node *node, const struct fdt_prop *prop,
		   void *priv);
	void *fn_priv;
	const char *str;
};

static void match_iter(const struct fdt_node *node, const struct fdt_prop *prop,
		       void *priv)
{
	char *data;
	struct match_iter_info *minfo = priv;
	struct fdt_prop nprop;

	/* Do nothing if node+prop dont match */
	if (!minfo->match(node, prop, minfo->match_priv))
		return;

	/* Call function for node */
	if (minfo->fn)
		minfo->fn(node, NULL, minfo->fn_priv);

	/* Convert node to character stream */
	data = node->data;
	data += sizeof(u32);

	/* Skip node name */
	data += strlen(data) + 1;
	while ((ulong)(data) % sizeof(u32) != 0)
		data++;

	/* Find node property and its value */
	while (DATA32(data) == FDT_PROP) {
		nprop.node = node;
		data += sizeof(u32);
		nprop.len = DATA32(data);
		data += sizeof(u32);
		nprop.name = &minfo->str[DATA32(data)];
		data += sizeof(u32);
		nprop.value = data;
		data += nprop.len;
		while ((ulong)(data) % sizeof(u32) != 0)
			(data)++;
		/* Call function for every property */
		if (minfo->fn)
			minfo->fn(node, &nprop, minfo->fn_priv);
	}
}

int fdt_match_node_prop(void *fdt,
			int (*match)(const struct fdt_node *node,
				     const struct fdt_prop *prop, void *priv),
			void *match_priv,
			void (*fn)(const struct fdt_node *node,
				   const struct fdt_prop *prop, void *priv),
			void *fn_priv)
{
	char *data;
	u32 string_offset, data_offset;
	struct fdt_header *header;
	struct match_iter_info minfo;
	struct recursive_iter_info rinfo;

	if (!fdt || !match)
		return -1;

	header = fdt;
	if (fdt_rev32(header->magic) != FDT_MAGIC ||
	    fdt_rev32(header->last_comp_version) > FDT_VERSION)
		return -1;
	string_offset = fdt_rev32(header->off_dt_strings);
	data_offset   = fdt_rev32(header->off_dt_struct);

	minfo.match	 = match;
	minfo.match_priv = match_priv;
	minfo.fn	 = fn;
	minfo.fn_priv	 = fn_priv;
	minfo.str	 = (const char *)(fdt + string_offset);

	rinfo.fn      = match_iter;
	rinfo.fn_priv = &minfo;
	rinfo.str     = minfo.str;

	data = (char *)(fdt + data_offset);
	recursive_iter(&data, &rinfo, NULL);

	return 0;
}

struct match_compat_info {
	const char *compat;
};

static int match_compat(const struct fdt_node *node,
			const struct fdt_prop *prop, void *priv)
{
	struct match_compat_info *cinfo = priv;

	if (!prop)
		return 0;

	if (strcmp(prop->name, "compatible"))
		return 0;

	if (fdt_prop_string_index(prop, cinfo->compat) < 0)
		return 0;

	return 1;
}

int fdt_compat_node_prop(void *fdt, const char *compat,
			 void (*fn)(const struct fdt_node *node,
				    const struct fdt_prop *prop, void *priv),
			 void *fn_priv)
{
	struct match_compat_info cinfo = { .compat = compat };

	return fdt_match_node_prop(fdt, match_compat, &cinfo, fn, fn_priv);
}

static int match_walk(const struct fdt_node *node, const struct fdt_prop *prop,
		      void *priv)
{
	if (!prop)
		return 1;

	return 0;
}

int fdt_walk(void *fdt,
	     void (*fn)(const struct fdt_node *node,
			const struct fdt_prop *prop, void *priv),
	     void *fn_priv)
{
	return fdt_match_node_prop(fdt, match_walk, NULL, fn, fn_priv);
}

u32 fdt_size(void *fdt)
{
	struct fdt_header *header;

	if (!fdt)
		return 0;

	header = fdt;
	if (fdt_rev32(header->magic) != FDT_MAGIC ||
	    fdt_rev32(header->last_comp_version) > FDT_VERSION)
		return 0;

	return fdt_rev32(header->totalsize);
}
