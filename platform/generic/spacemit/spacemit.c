/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 SpacemiT
 * Authors:
 *   Xianbin Zhu <xianbin.zhu@linux.spacemit.com>
 *   Troy Mitchell <troy.mitchell@linux.spacemit.com>
 */

#include <platform_override.h>
#include <sbi/riscv_io.h>
#include <spacemit/common.h>

void cci_enable_snoop_dvm_reqs(const int *cci_map, unsigned int master_id)
{
	int slave_if_id = cci_map[master_id];

	/*
	 * Enable Snoops and DVM messages, no need for Read/Modify/Write as
	 * rest of bits are write ignore
	 */
	writel(CCI_550_SNOOP_CTRL_ENABLE_DVMS | CCI_550_SNOOP_CTRL_ENABLE_SNOOPS,
	       (void *)(unsigned long)CCI_550_PLATFORM_CCI_ADDR +
	       CCI_550_SLAVE_IFACE_OFFSET(slave_if_id) + CCI_550_SNOOP_CTRL);

	/*
	 * Wait for the completion of the write to the Snoop Control Register
	 * before testing the change_pending bit
	 */
	mb();

	/* Wait for the dust to settle down */
	while (readl((void *)(unsigned long)CCI_550_PLATFORM_CCI_ADDR + CCI_550_STATUS) &
	       CCI_550_STATUS_CHANGE_PENDING)
		;
}
