/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 Bo Gan <ganboing@gmail.com>
 *
 */

#include <platform_override.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_system.h>
#include <sbi/sbi_math.h>
#include <sbi/sbi_hart_pmp.h>
#include <sbi/sbi_hart_protection.h>
#include <eswin/eic770x.h>
#include <eswin/hfp.h>

static struct sbi_hart_protection eswin_eic7700_pmp_protection;

static int eic770x_system_reset_check(u32 type, u32 reason)
{
	switch (type) {
	case SBI_SRST_RESET_TYPE_COLD_REBOOT:
	case SBI_SRST_RESET_TYPE_WARM_REBOOT:
		return 1;
	default:
		return 0;
	}
}

static void eic770x_system_reset(u32 type, u32 reason)
{
	switch (type) {
	case SBI_SRST_RESET_TYPE_COLD_REBOOT:
	case SBI_SRST_RESET_TYPE_WARM_REBOOT:
		sbi_printf("%s: resetting...\n", __func__);
		writel(EIC770X_SYSCRG_RST_VAL, (void *)EIC770X_SYSCRG_RST);
	}

	sbi_hart_hang();
}

static struct sbi_system_reset_device *board_reset = NULL;
static struct sbi_system_reset_device eic770x_reset = {
	.name = "eic770x_reset",
	.system_reset_check = eic770x_system_reset_check,
	.system_reset = eic770x_system_reset,
};

#define add_root_mem_chk(...) do { \
	rc = sbi_domain_root_add_memrange(__VA_ARGS__); \
	if (rc) \
		return rc; \
} while (0)

/**
 * EIC7700 special arrangement of PMP entries:
 *
 * We have to use extra PMPs to block data cacheable regions that
 * that doesn't belong to the current hart's die in order to prevent
 * speculative accesses or HW prefetcher from generating bus error:
 *
 * 	bus error of cause event: 9, accrued: 0x220,
 *	physical address: 0x24ffffffa0
 *
 * The data cacheable regions (per datasheet) include:
 *
 *   - [0x1a000000,    0x1a400000) -- Die 0 L3 zero device
 *   - [0x3a000000,    0x3a400000) -- Die 1 L3 zero device
 *   - [0x80000000, 0x80_00000000) -- memory port
 *
 * To make the blocker effective for M mode too, the extra PMPs need
 * LOCK bit to be set, and once set, we can't change them later.
 * We also have to to use 1 extra PMP to protect OpenSBI in uncached
 * memory. EIC770X maps main memory (DRAM) twice -- one in memory
 * port (cached), the other in system port (uncached). P550 doesn't
 * support Svpbmt, so EIC770X use the uncached window to handle DMA
 * that are cache incoherent -- pretty much all peripherals
 *
 * Final PMP configuration:
 *
 * From die 0 point of view, block
 *   -         [0x3a000000,    0x3a400000) -- Die 1 L3 zero device
 *   -      [0x10_00000000, 0x80_00000000) -- Die 1 cached mem + holes
 *
 * Root domain Harts:
 *  PMP[0]: [   0x80000000,    0x80080000) ____ Firmware in cached mem
 *  PMP[1]: [0xc0_00000000, 0xc0_00080000) ____ Firmware in uncached
 *  PMP[2]: [   0x3a000000,    0x3a400000) L___ Die 1 L3 zero device
 *  PMP[3]: [    0x2000000      0x2010000) ____ CLINT
 *  PMP[4]: [          0x0, 0x10_00000000) _RWX P550/System/Die 0 cached mem
 *  PMP[5]: <Free>
 *  PMP[6]: [          0x0, 0x80_00000000) L___ P550/System/Memory Port
 *  PMP[7]: [     0x0, 0xffffffffffffffff] _RWX Everything
 *
 * From die 1 point of view, block
 *   -         [0x1a000000,    0x1a400000) -- Die 0 L3 zero device
 *   -         [0x80000000, 0x20_00000000) -- Die 0 cached mem + holes
 *   -      [0x30_00000000, 0x80_00000000) -- other holes in Memory port
 *
 * Root domain Harts:
 *  PMP[0]: [0x20_00000000, 0x20_00080000) ____ Firmware in cached mem
 *  PMP[1]: [0xe0_00000000, 0xe0_00080000) ____ Firmware in uncached
 *  PMP[2]: [   0x1a000000,    0x1a400000) L___ Die 0 L3 zero dev
 *  PMP[3]: [   0x22000000     0x22010000) ____ CLINT
 *  PMP[4]: [          0x0,    0x80000000) _RWX Die 0/1 P550 internal
 *  PMP[5]: [0x20_00000000, 0x30_00000000) _RWX Die 1 cached memory
 *  PMP[6]: [          0x0, 0x80_00000000) L___ P550/System/Memory Port
 *  PMP[7]: [     0x0, 0xffffffffffffffff] _RWX Everything
 *
 * EIC770X memory port map:
 * P550 Internal
 *   ├─ 0x0000_0000 - 0x2000_0000 die 0 internal
 *   └─ 0x2000_0000 - 0x4000_0000 die 1 internal
 * System Port 0
 *   ├─ 0x4000_0000 - 0x6000_0000 die 0 low MMIO
 *   └─ 0x6000_0000 - 0x8000_0000 die 1 low MMIO
 * Memory Port
 *   ├─    0x8000_0000 - 0x10_8000_0000 die 0 memory (cached)
 *   ├─ 0x20_0000_0000 - 0x30_0000_0000 die 1 memory (cached)
 *   └─ 0x40_0000_0000 - 0x60_0000_0000 interleaved memory (cached)
 * System Port 1
 *   ├─ 0x80_0000_0000 - 0xa0_0000_0000 die 0 high MMIO
 *   ├─ 0xa0_0000_0000 - 0xc0_0000_0000 die 1 high MMIO
 *   ├─ 0xc0_0000_0000 - 0xd0_0000_0000 die 0 memory (uncached)
 *   ├─ 0xe0_0000_0000 - 0xf0_0000_0000 die 1 memory (uncached)
 *   ├─ 0x100_0000_0000 - 0x120_0000_0000 interleaved memory (uncached)
 *   └─ ...
 *
 * In early_init, add memory regions such that lib/ code has the knowledge
 * of blocked ranges. When the driver code inserts new regions, lib/ code
 * can optimize away unnecessary ones. Next, in final_init, we program the
 * PMPs to a default state that'll keep ourselves functional (CLINT/...
 * accessible). Later, in pmp_configure, do the actual configuration of
 * PMP, using domain memory regions and permissions.
 */

static int eswin_eic7700_early_init(bool cold_boot)
{
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();
	int rc;

	if (!cold_boot)
		return generic_early_init(cold_boot);

	if (board_reset)
		sbi_system_reset_add_device(board_reset);
	sbi_system_reset_add_device(&eic770x_reset);

	/* Enable bus blocker */
	writel(1, (void*)EIC770X_TL64D2D_OUT);
	writel(1, (void*)EIC770X_TL256D2D_OUT);
	writel(1, (void*)EIC770X_TL256D2D_IN);
	asm volatile ("fence o, rw");

	/* Block firmware in uncached memory */
	add_root_mem_chk(EIC770X_TO_UNCACHED(
			 scratch->fw_start),
			 1UL << log2roundup(scratch->fw_size),
			 1UL << log2roundup(scratch->fw_size),
			(SBI_DOMAIN_MEMREGION_M_READABLE |
			 SBI_DOMAIN_MEMREGION_M_WRITABLE |
			 SBI_DOMAIN_MEMREGION_M_EXECUTABLE |
			 SBI_DOMAIN_MEMREGION_MMIO |
			 SBI_DOMAIN_MEMREGION_FW));

	/* Allow SURW of P550 + System Port */
	add_root_mem_chk(0,
			 EIC770X_MEMPORT_BASE,
			 EIC770X_MEMPORT_BASE,
			(SBI_DOMAIN_MEMREGION_MMIO |
			 SBI_DOMAIN_MEMREGION_SHARED_SURW_MRW));

	if (current_hart_die()) {
		/* Allow SURWX of die 1 cached memory */
		add_root_mem_chk(EIC770X_D1_MEM_BASE,
				 EIC770X_D1_MEM_SIZE,
				 EIC770X_D1_MEM_SIZE,
				(SBI_DOMAIN_MEMREGION_M_READABLE |
				 SBI_DOMAIN_MEMREGION_M_WRITABLE |
				 SBI_DOMAIN_MEMREGION_SU_RWX));
	} else {
		/* Allow SURWX of P550 + System Port + die 0 cached memory */
		add_root_mem_chk(0,
				 EIC770X_D0_MEM_LIMIT,
				 EIC770X_D0_MEM_LIMIT,
				(SBI_DOMAIN_MEMREGION_M_READABLE |
				 SBI_DOMAIN_MEMREGION_M_WRITABLE |
				 SBI_DOMAIN_MEMREGION_SU_RWX));
	}

	/* Block P550 + System Port 0 + Memory Port (enforced) */
	add_root_mem_chk(0,
			 EIC770X_MEMPORT_LIMIT,
			 EIC770X_MEMPORT_LIMIT,
			(SBI_DOMAIN_MEMREGION_MMIO |
			 SBI_DOMAIN_MEMREGION_ENF_PERMISSIONS));

	rc = sbi_hart_protection_register(&eswin_eic7700_pmp_protection);
	if (rc)
		return rc;

	return generic_early_init(cold_boot);
}

#define PMP_FW_START 0
#define PMP_FW_COUNT 2
#define PMP_RESERVED_A 2
#define PMP_FREE_A_START 3
#define PMP_FREE_A_COUNT 3
#define PMP_RESERVED_B 6
#define PMP_FREE_B_START 7
#define PMP_FREE_B_COUNT 1

static int eswin_eic7700_final_init(bool cold_boot)
{
	/**
	 * For both dies after final_init:
	 *
	 *  PMP[0]:   Protect OpenSBI in cached memory
	 *  PMP[1]:   Protect OpenSBI in uncached memory
	 *  PMP[2]:   Block remote die P550 L3 Zero Device
	 *  PMP[3-5]: <Free ranges A>
	 *  PMP[5]:   Temporary enable P550 + System Port
	 *  PMP[6]:   Block all P550 + System + Memory Port
	 *  PMP[7-7]: <Free ranges B>
	 */
	struct sbi_domain_memregion *reg;
	unsigned int pmp_idx = PMP_FW_START,
		     pmp_max = PMP_FW_START + PMP_FW_COUNT;
	int rc;


	/**
	 * Do generic_final_init stuff first, because it touchs FDT.
	 * After final_init, we'll block entire memory port with the
	 * LOCK bit set, which means we can't access memory outside
	 * of [fw_start, fw_start + fw_size). The FDT could very well
	 * reside outside of firmware region. Later, pmp_configure()
	 * may unblock it with some preceding entries for root domain
	 * harts. It may not unblock it, however, for non-root harts.
	 */
	rc = generic_final_init(cold_boot);
	if (rc)
		return rc;

	/* Process firmware regions */
	sbi_domain_for_each_memregion(&root, reg) {
		if (!SBI_DOMAIN_MEMREGION_IS_FIRMWARE(reg->flags))
			continue;

		if (pmp_idx >= pmp_max) {
			sbi_printf("%s: insufficient FW PMP entries\n",
				   __func__);
			return SBI_EFAIL;
		}
		pmp_set(pmp_idx++, sbi_domain_get_oldpmp_flags(reg),
			reg->base, reg->order);
	}

	pmp_set(PMP_RESERVED_A, PMP_L, EIC770X_L3_ZERO_REMOTE,
			   log2roundup(EIC770X_L3_ZERO_SIZE));
	/**
	 * Enable P550 internal + System Port, so OpenSBI can access
	 * CLINT/PLIC/UART. Might be overwritten in pmp_configure.
	 */
	pmp_set(PMP_FREE_A_START + PMP_FREE_A_COUNT - 1, 0, 0,
		log2roundup(EIC770X_MEMPORT_BASE));

	pmp_set(PMP_RESERVED_B, PMP_L, 0,
		log2roundup(EIC770X_MEMPORT_LIMIT));
	/**
	 * These must come after the setup of PMP, as we are about to
	 * enable speculation and HW prefetcher bits
	 */
	csr_write(EIC770X_CSR_FEAT0, CONFIG_ESWIN_EIC770X_FEAT0_CFG);
	csr_write(EIC770X_CSR_FEAT1, CONFIG_ESWIN_EIC770X_FEAT1_CFG);
	csr_write(EIC770X_CSR_L1_HWPF, CONFIG_ESWIN_EIC770X_L1_HWPF_CFG);
	csr_write(EIC770X_CSR_L2_HWPF, CONFIG_ESWIN_EIC770X_L2_HWPF_CFG);

	return 0;
}

static int eswin_eic7700_pmp_configure(struct sbi_scratch *scratch)
{
	struct sbi_domain *dom = sbi_domain_thishart_ptr();
	struct sbi_domain_memregion *reg, *prev = NULL;
	unsigned int pmp_idx, pmp_max;
	unsigned int i, j;

	/* Process the first free range A [3-5] */
	pmp_idx = PMP_FREE_A_START,
	pmp_max = PMP_FREE_A_START + PMP_FREE_A_COUNT;

	sbi_domain_for_each_memregion_idx(dom, reg, i) {
		if (SBI_DOMAIN_MEMREGION_IS_FIRMWARE(reg->flags))
			continue;

		/**
		 * This must be the one blocking P550 + System Port +
		 * Memory Port we setup in early_init, or a superset
		 * of it. If seen, break, and program the rest in
		 * free range B.
		 */
		if (reg->base == 0 &&
		    reg->order >= log2roundup(EIC770X_MEMPORT_LIMIT))
			break;

		/**
		 * Relaxation:
		 * Treat a previous region with SURW as SURWX if the
		 * current has SURWX, and current region with MMIO
		 * if previous has MMIO, and see if it can be merged.
		 * This saves 1 PMP entry on die 0/
		 */
		if (prev && sbi_domain_memregion_is_subset(prev, reg) &&
		    (reg->flags | SBI_DOMAIN_MEMREGION_MMIO) ==
		    (prev->flags | SBI_DOMAIN_MEMREGION_SU_EXECUTABLE))
			pmp_idx--;

		if (pmp_idx >= pmp_max)
			goto no_more_pmp;

		pmp_set(pmp_idx++, sbi_domain_get_oldpmp_flags(reg),
			reg->base, reg->order);
		prev = reg;
	}
	/* Disable the rest */
	while (pmp_idx < pmp_max)
		pmp_disable(pmp_idx++);

	/* Process the second free range B [7-7] */
	pmp_idx = PMP_FREE_B_START,
	pmp_max = PMP_FREE_B_START + PMP_FREE_B_COUNT;

	sbi_domain_for_each_memregion_idx(dom, reg, j) {
		if (i >= j)
			continue;

		if (pmp_idx >= pmp_max)
			goto no_more_pmp;

		pmp_set(pmp_idx++, sbi_domain_get_oldpmp_flags(reg),
			reg->base, reg->order);
	}
	/* Disable the rest */
	while (pmp_idx < pmp_max)
		pmp_disable(pmp_idx++);

	sbi_hart_pmp_fence();
	return 0;
no_more_pmp:
	sbi_printf("%s: insufficient PMP entries\n", __func__);
	return SBI_EFAIL;
}

static void eswin_eic7700_pmp_unconfigure(struct sbi_scratch *scratch)
{
	/* Enable P550 internal + System Port */
	pmp_set(PMP_FREE_A_START + PMP_FREE_A_COUNT - 1, 0, 0,
		log2roundup(EIC770X_MEMPORT_BASE));

	for (unsigned int i = 0; i < PMP_FREE_A_COUNT - 1; i++)
		pmp_disable(i + PMP_FREE_A_START);

	for (unsigned int i = 0; i < PMP_FREE_B_COUNT; i++)
		pmp_disable(i + PMP_FREE_B_START);
}

static struct sbi_hart_protection eswin_eic7700_pmp_protection = {
	.name = "eic7700_pmp",
	.rating = -1UL,
	.configure = eswin_eic7700_pmp_configure,
	.unconfigure = eswin_eic7700_pmp_unconfigure,
};

static bool eswin_eic7700_single_fw_region(void)
{
	return true;
}

static int eswin_eic7700_platform_init(const void *fdt, int nodeoff,
					const struct fdt_match *match)
{
	const struct eic770x_board_override *board_override = match->data;

	generic_platform_ops.early_init = eswin_eic7700_early_init;
	generic_platform_ops.final_init = eswin_eic7700_final_init;
	generic_platform_ops.single_fw_region = eswin_eic7700_single_fw_region;

	if (board_override)
		board_reset = board_override->reset_dev;
	return 0;
}

static const struct fdt_match eswin_eic7700_match[] = {
	{ .compatible = "sifive,hifive-premier-p550", .data = &hfp_override },
	{ .compatible = "eswin,eic7700" },
	{ },
};

const struct fdt_driver eswin_eic7700 = {
	.match_table = eswin_eic7700_match,
	.init = eswin_eic7700_platform_init,
};
