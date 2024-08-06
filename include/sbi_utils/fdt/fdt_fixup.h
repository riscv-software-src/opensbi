// SPDX-License-Identifier: BSD-2-Clause
/*
 * fdt_fixup.h - Flat Device Tree manipulation helper routines
 * Implement platform specific DT fixups on top of libfdt. 
 *
 * Copyright (C) 2020 Bin Meng <bmeng.cn@gmail.com>
 */

#ifndef __FDT_FIXUP_H__
#define __FDT_FIXUP_H__

#include <sbi/sbi_list.h>

struct sbi_cpu_idle_state {
	const char *name;
	uint32_t suspend_param;
	bool local_timer_stop;
	uint32_t entry_latency_us;
	uint32_t exit_latency_us;
	uint32_t min_residency_us;
	uint32_t wakeup_latency_us;
};

/**
 * Add CPU idle states to cpu nodes in the DT
 *
 * Add information about CPU idle states to the devicetree. This function
 * assumes that CPU idle states are not already present in the devicetree, and
 * that all CPU states are equally applicable to all CPUs.
 *
 * @param fdt: device tree blob
 * @param states: array of idle state descriptions, ending with empty element
 * @return zero on success and -ve on failure
 */
int fdt_add_cpu_idle_states(void *fdt, const struct sbi_cpu_idle_state *state);

/**
 * Fix up the CPU node in the device tree
 *
 * This routine updates the "status" property of a CPU node in the device tree
 * to "disabled" if that hart is in disabled state in OpenSBI.
 *
 * It is recommended that platform codes call this helper in their final_init()
 *
 * @param fdt: device tree blob
 */
void fdt_cpu_fixup(void *fdt);

/**
 * Fix up the APLIC nodes in the device tree
 *
 * This routine disables APLIC nodes which are not accessible to the next
 * booting stage based on currently assigned domain.
 *
 * It is recommended that platform codes call this helper in their final_init()
 *
 * @param fdt: device tree blob
 */
void fdt_aplic_fixup(void *fdt);

/**
 * Fix up the IMSIC nodes in the device tree
 *
 * This routine disables IMSIC nodes which are not accessible to the next
 * booting stage based on currently assigned domain.
 *
 * It is recommended that platform codes call this helper in their final_init()
 *
 * @param fdt: device tree blob
 */
void fdt_imsic_fixup(void *fdt);

/**
 * Fix up the PLIC node in the device tree
 *
 * This routine updates the "interrupt-extended" property of the PLIC node in
 * the device tree to hide the M-mode external interrupt from CPUs.
 *
 * It is recommended that platform codes call this helper in their final_init()
 *
 * @param fdt: device tree blob
 */
void fdt_plic_fixup(void *fdt);

/**
 * Fix up the reserved memory node in the device tree
 *
 * This routine inserts a child node of the reserved memory node in the device
 * tree that describes the protected memory region done by OpenSBI via PMP.
 *
 * It is recommended that platform codes call this helper in their final_init()
 *
 * @param fdt: device tree blob
 * @return zero on success and -ve on failure
 */
int fdt_reserved_memory_fixup(void *fdt);

/** Representation of a general fixup */
struct fdt_general_fixup {
	struct sbi_dlist head;
	const char *name;
	void (*do_fixup)(struct fdt_general_fixup *f, void *fdt);
};

/** Register a general fixup */
int fdt_register_general_fixup(struct fdt_general_fixup *fixup);

/** UnRegister a general fixup */
void fdt_unregister_general_fixup(struct fdt_general_fixup *fixup);

/**
 * General device tree fix-up
 *
 * This routine do all required device tree fix-ups for a typical platform.
 * It fixes up the PLIC node, IMSIC nodes, APLIC nodes, and the reserved
 * memory node in the device tree by calling the corresponding helper
 * routines to accomplish the task.
 *
 * It is recommended that platform codes call this helper in their final_init()
 *
 * @param fdt: device tree blob
 */
void fdt_fixups(void *fdt);

#endif /* __FDT_FIXUP_H__ */

