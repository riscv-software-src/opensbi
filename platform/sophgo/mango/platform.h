/*
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef _SG2040_PLATFORM_H_
#define _SG2040_PLATFORM_H_

#define SG2040_HART_COUNT	64
#define SG2040_HART_PER_CHIP	64

#define SG2040_CHIP_ADDR_SPACE		(1UL << 39)
#define SG2040_CHIP_ADDR_BASE(n)	((n) * SG2040_CHIP_ADDR_SPACE)
#define SG2040_CHIP_HARTID_BASE(n)	((n) * SG2040_HART_PER_CHIP)

#define SBI_SOPHGO_FEATURES	\
	 (SBI_PLATFORM_HAS_MFAULTS_DELEGATION \
	 )

#define SBI_EXT_VENDOR_SG2040_SET_PMU            0x09000001
#define SBI_EXT_VENDOR_SG2040_BOOT_OTHER_CORE    0x09000003

#define SG2040_PLIC_BASE		0x7090000000UL
#define SG2040_MSIP_BASE		0x7094000000UL
#define SG2040_MTIMECMP_BASE		(0x70ac000000UL + 0x4000UL)

#define SG2040_PLIC_DELEG_OFFSET     0x001ffffc
#define SG2040_PLIC_DELEG_ENABLE     0x1

#define SG2040_UART0_ADDRBASE        0x7040000000
#ifdef MANGO_PLATFORM_PLD
#define SG2040_UART0_FREQ            153600
#define SG2040_CONSOLE_BDRATE        9600
#else
#define SG2040_UART0_FREQ            500000000
#define SG2040_CONSOLE_BDRATE        115200
#endif

#define MANGO_DEVICE_LOCK_REGISTER0	0x7030010140UL
#define MANGO_DEVICE_LOCK_REGISTER1	0x7030010144UL
#define MANGO_PA_BASE			0x0UL
#define MANGO_IO_BASE			0x7000000000UL
#define MANGO_HW_LOCK_BASE		MANGO_DEVICE_LOCK_REGISTER0
#define MANGO_CONSOLE_LOCK		MANGO_DEVICE_LOCK_REGISTER1

#define MANGO_CORES_PER_CLUSTER		4
#define MANGO_CLUSTER_ID(_vhartid)	(_vhartid / MANGO_CORES_PER_CLUSTER)
#define MANGO_CORE_ID(_vhartid)		(_vhartid % MANGO_CORES_PER_CLUSTER)

#define MANGO_MTIMECMP_SIZE_PER_CLUSTER	(64 * 1024)

#undef __SG_QEMU__

#include <sbi/sbi_types.h>

struct sg2040_regs_struct {
	u64 pmpaddr0;
	u64 pmpaddr1;
	u64 pmpaddr2;
	u64 pmpaddr3;
	u64 pmpaddr4;
	u64 pmpaddr5;
	u64 pmpaddr6;
	u64 pmpaddr7;
	u64 pmpcfg0;
	u64 msmpr;
	u64 mcor;
	u64 mhcr;
	u64 mccr2;
	u64 mhint;
	u64 mhint2;
	u64 mxstatus;
	u64 plic_base_addr;
	u64 msip_base_addr;
	u64 mtimecmp_base_addr;
};

struct pmp {
	long start;
	long end;
};

#endif /* _SG2040_PLATFORM_H_ */
