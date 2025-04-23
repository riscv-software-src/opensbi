// SPDX-License-Identifier: BSD-2-Clause
/*
 * fdt_helper.h - Flat Device Tree parsing helper routines
 * Implement helper routines to parse FDT nodes on top of
 * libfdt for OpenSBI usage
 *
 * Copyright (C) 2020 Bin Meng <bmeng.cn@gmail.com>
 */

#ifndef __FDT_HELPER_H__
#define __FDT_HELPER_H__

#include <sbi/sbi_types.h>
#include <sbi/sbi_domain.h>

struct fdt_match {
	const char *compatible;
	const void *data;
};

#define FDT_MAX_PHANDLE_ARGS 16
struct fdt_phandle_args {
	int node_offset;
	int args_count;
	u32 args[FDT_MAX_PHANDLE_ARGS];
};

struct platform_uart_data {
	unsigned long addr;
	unsigned long freq;
	unsigned long baud;
	unsigned long reg_shift;
	unsigned long reg_io_width;
	unsigned long reg_offset;
};

int fdt_parse_phandle_with_args(const void *fdt, int nodeoff,
				const char *prop, const char *cells_prop,
				int index, struct fdt_phandle_args *out_args);

int fdt_get_node_addr_size(const void *fdt, int node, int index,
			   uint64_t *addr, uint64_t *size);

int fdt_get_node_addr_size_by_name(const void *fdt, int node, const char *name,
				   uint64_t *addr, uint64_t *size);

bool fdt_node_is_enabled(const void *fdt, int nodeoff);

int fdt_parse_hart_id(const void *fdt, int cpu_offset, u32 *hartid);

int fdt_parse_max_enabled_hart_id(const void *fdt, u32 *max_hartid);

int fdt_parse_cbom_block_size(const void *fdt, int cpu_offset, unsigned long  *cbom_block_size);

int fdt_parse_timebase_frequency(const void *fdt, unsigned long *freq);

int fdt_parse_isa_extensions(const void *fdt, unsigned int hartid,
			     unsigned long *extensions);

int fdt_parse_gaisler_uart_node(const void *fdt, int nodeoffset,
				struct platform_uart_data *uart);

int fdt_parse_renesas_scif_node(const void *fdt, int nodeoffset,
				struct platform_uart_data *uart);

int fdt_parse_shakti_uart_node(const void *fdt, int nodeoffset,
			       struct platform_uart_data *uart);

int fdt_parse_sifive_uart_node(const void *fdt, int nodeoffset,
			       struct platform_uart_data *uart);

int fdt_parse_uart_node(const void *fdt, int nodeoffset,
			struct platform_uart_data *uart);

int fdt_parse_uart8250(const void *fdt, struct platform_uart_data *uart,
		       const char *compatible);

int fdt_parse_xlnx_uartlite_node(const void *fdt, int nodeoffset,
			       struct platform_uart_data *uart);

struct aplic_data;

int fdt_parse_aplic_node(const void *fdt, int nodeoff, struct aplic_data *aplic);

struct imsic_data;

bool fdt_check_imsic_mlevel(const void *fdt);

int fdt_parse_imsic_node(const void *fdt, int nodeoff, struct imsic_data *imsic);

struct plic_data;

int fdt_parse_plic_node(const void *fdt, int nodeoffset, struct plic_data *plic);

int fdt_parse_plic(const void *fdt, struct plic_data *plic, const char *compat);

int fdt_parse_aclint_node(const void *fdt, int nodeoffset,
			  bool for_timer, bool allow_regname,
			  unsigned long *out_addr1, unsigned long *out_size1,
			  unsigned long *out_addr2, unsigned long *out_size2,
			  u32 *out_first_hartid, u32 *out_hart_count);

int fdt_parse_plmt_node(const void *fdt, int nodeoffset, unsigned long *plmt_base,
			unsigned long *plmt_size, u32 *hart_count);

int fdt_parse_plicsw_node(const void *fdt, int nodeoffset, unsigned long *plicsw_base,
			  unsigned long *size, u32 *hart_count);

int fdt_parse_compat_addr(const void *fdt, uint64_t *addr,
			  const char *compatible);

static inline const void *fdt_get_address(void)
{
	return (const void *)root.next_arg1;
}

static inline void *fdt_get_address_rw(void)
{
	return (void *)root.next_arg1;
}

#endif /* __FDT_HELPER_H__ */
