/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Andes Technology Corporation
 *
 * Authors:
 *   Zong Li <zong@andestech.com>
 *   Nylon Chen <nylon7@andestech.com>
 *   Yu Chien Peter Lin <peterlin@andestech.com>
 */

#include <libfdt.h>
#include <sbi/riscv_asm.h>
#include <sbi/riscv_encoding.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_const.h>
#include <sbi/sbi_hartmask.h>
#include <sbi/sbi_ipi.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_string.h>
#include <sbi/sbi_trap.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/fdt/fdt_fixup.h>
#include <sbi_utils/irqchip/fdt_irqchip.h>
#include <sbi_utils/reset/fdt_reset.h>
#include <sbi_utils/serial/fdt_serial.h>
#include <sbi_utils/timer/fdt_timer.h>
#include <sbi_utils/ipi/fdt_ipi.h>
#include "platform.h"
#include "cache.h"

struct sbi_platform platform;
unsigned long fw_platform_init(unsigned long arg0, unsigned long arg1,
				unsigned long arg2, unsigned long arg3,
				unsigned long arg4)
{
	const char *model;
	void *fdt = (void *)arg1;
	u32 hartid, hart_count = 0;
	int rc, root_offset, cpus_offset, cpu_offset, len;

	root_offset = fdt_path_offset(fdt, "/");
	if (root_offset < 0)
		goto fail;

	model = fdt_getprop(fdt, root_offset, "model", &len);
	if (model)
		sbi_strncpy(platform.name, model, sizeof(platform.name) - 1);

	cpus_offset = fdt_path_offset(fdt, "/cpus");
	if (cpus_offset < 0)
		goto fail;

	fdt_for_each_subnode(cpu_offset, fdt, cpus_offset) {
		rc = fdt_parse_hart_id(fdt, cpu_offset, &hartid);
		if (rc)
			continue;

		if (SBI_HARTMASK_MAX_BITS <= hartid)
			continue;

		hart_count++;
	}

	platform.hart_count = hart_count;

	/* Return original FDT pointer */
	return arg1;

fail:
	while (1)
		wfi();
}

/* Platform final initialization. */
static int ae350_final_init(bool cold_boot)
{
	void *fdt;

	if (!cold_boot)
		return 0;

	fdt_reset_init();

	fdt = fdt_get_address();
	fdt_fixups(fdt);

	return 0;
}

/* Vendor-Specific SBI handler */
static int ae350_vendor_ext_provider(long extid, long funcid,
	const struct sbi_trap_regs *regs, unsigned long *out_value,
	struct sbi_trap_info *out_trap)
{
	int ret = 0;
	switch (funcid) {
	case SBI_EXT_ANDES_GET_MCACHE_CTL_STATUS:
		*out_value = csr_read(CSR_MCACHECTL);
		break;
	case SBI_EXT_ANDES_GET_MMISC_CTL_STATUS:
		*out_value = csr_read(CSR_MMISCCTL);
		break;
	case SBI_EXT_ANDES_SET_MCACHE_CTL:
		ret = mcall_set_mcache_ctl(regs->a0);
		break;
	case SBI_EXT_ANDES_SET_MMISC_CTL:
		ret = mcall_set_mmisc_ctl(regs->a0);
		break;
	case SBI_EXT_ANDES_ICACHE_OP:
		ret = mcall_icache_op(regs->a0);
		break;
	case SBI_EXT_ANDES_DCACHE_OP:
		ret = mcall_dcache_op(regs->a0);
		break;
	case SBI_EXT_ANDES_L1CACHE_I_PREFETCH:
		ret = mcall_l1_cache_i_prefetch_op(regs->a0);
		break;
	case SBI_EXT_ANDES_L1CACHE_D_PREFETCH:
		ret = mcall_l1_cache_d_prefetch_op(regs->a0);
		break;
	case SBI_EXT_ANDES_NON_BLOCKING_LOAD_STORE:
		ret = mcall_non_blocking_load_store(regs->a0);
		break;
	case SBI_EXT_ANDES_WRITE_AROUND:
		ret = mcall_write_around(regs->a0);
		break;
	default:
		sbi_printf("Unsupported vendor sbi call : %ld\n", funcid);
		asm volatile("ebreak");
	}
	return ret;
}

/* Platform descriptor. */
const struct sbi_platform_operations platform_ops = {
	.final_init = ae350_final_init,

	.console_init = fdt_serial_init,

	.irqchip_init = fdt_irqchip_init,

	.ipi_init     = fdt_ipi_init,

	.timer_init	   = fdt_timer_init,

	.vendor_ext_provider = ae350_vendor_ext_provider
};

struct sbi_platform platform = {
	.opensbi_version = OPENSBI_VERSION,
	.platform_version =
		SBI_PLATFORM_VERSION(CONFIG_PLATFORM_ANDES_AE350_MAJOR_VER,
				     CONFIG_PLATFORM_ANDES_AE350_MINOR_VER),
	.name = CONFIG_PLATFORM_ANDES_AE350_NAME,
	.features = SBI_PLATFORM_DEFAULT_FEATURES,
	.hart_count = SBI_HARTMASK_MAX_BITS,
	.hart_stack_size = SBI_PLATFORM_DEFAULT_HART_STACK_SIZE,
	.platform_ops_addr = (unsigned long)&platform_ops
};
