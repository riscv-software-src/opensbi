/*
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef _C910_PLATFORM_H_
#define _C910_PLATFORM_H_

#define C910_HART_COUNT   16
#define C910_HART_STACK_SIZE   8192

#define SBI_THEAD_FEATURES	\
	(SBI_PLATFORM_HAS_SCOUNTEREN | \
	 SBI_PLATFORM_HAS_MCOUNTEREN | \
	 SBI_PLATFORM_HAS_MFAULTS_DELEGATION)

#define CSR_MCOR         0x7c2
#define CSR_MHCR         0x7c1
#define CSR_MCCR2        0x7c3
#define CSR_MHINT        0x7c5
#define CSR_MXSTATUS     0x7c0
#define CSR_PLIC_BASE    0xfc1
#define CSR_MRMR         0x7c6
#define CSR_MRVBR        0x7c7

#define SBI_EXT_VENDOR_C910_BOOT_OTHER_CORE    0x09000003

#define C910_PLIC_CLINT_OFFSET     0x04000000  /* 64M */
#define C910_PLIC_DELEG_OFFSET     0x001ffffc
#define C910_PLIC_DELEG_ENABLE     0x1

struct c910_regs_struct {
	u64 pmpaddr0;
	u64 pmpaddr1;
	u64 pmpaddr2;
	u64 pmpaddr3;
	u64 pmpaddr4;
	u64 pmpaddr5;
	u64 pmpaddr6;
	u64 pmpaddr7;
	u64 pmpcfg0;
	u64 mcor;
	u64 mhcr;
	u64 mccr2;
	u64 mhint;
	u64 mxstatus;
	u64 plic_base_addr;
	u64 clint_base_addr;
};

#endif /* _C910_PLATFORM_H_ */
