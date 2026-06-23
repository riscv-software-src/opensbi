#ifndef __RISCV_SPACEMIT_K1_H__
#define __RISCV_SPACEMIT_K1_H__

#define CSR_MSETUP				0x7c0
#define CSR_MHCR				0x7c1
#define CSR_MRAOP				0x7c2
#define CSR_MHINT				0x7c5
#define CSR_ML2SETUP				0x7f0

/* decache enable */
#define MSETUP_DE				BIT(0)
/* icache enable */
#define MSETUP_IE				BIT(1)
/* branch prediction enable */
#define MSETUP_BPE				BIT(4)
/* prefetch functionality enable */
#define MSETUP_PFE				BIT(5)
/* misaligned memory access enable */
#define MSETUP_MME				BIT(6)
/* ECC enable */
#define MSETUP_ECCE				BIT(16)

/* icache invalidation */
#define MRAOP_ICACHE_INVALID			GENMASK(1, 0)

#define PMU_AP_BASE				0xd4282800

#define PMU_AP_CORE0_WAKEUP			(PMU_AP_BASE + 0x12c)
#define PMU_AP_CORE1_WAKEUP			(PMU_AP_BASE + 0x130)
#define PMU_AP_CORE2_WAKEUP			(PMU_AP_BASE + 0x134)
#define PMU_AP_CORE3_WAKEUP			(PMU_AP_BASE + 0x138)
#define PMU_AP_CORE4_WAKEUP			(PMU_AP_BASE + 0x324)
#define PMU_AP_CORE5_WAKEUP			(PMU_AP_BASE + 0x328)
#define PMU_AP_CORE6_WAKEUP			(PMU_AP_BASE + 0x32c)
#define PMU_AP_CORE7_WAKEUP			(PMU_AP_BASE + 0x330)

#define PMU_AP_CORE0_IDLE_CFG			(PMU_AP_BASE + 0x124)
#define PMU_AP_CORE1_IDLE_CFG			(PMU_AP_BASE + 0x128)
#define PMU_AP_CORE2_IDLE_CFG			(PMU_AP_BASE + 0x160)
#define PMU_AP_CORE3_IDLE_CFG			(PMU_AP_BASE + 0x164)
#define PMU_AP_CORE4_IDLE_CFG			(PMU_AP_BASE + 0x304)
#define PMU_AP_CORE5_IDLE_CFG			(PMU_AP_BASE + 0x308)
#define PMU_AP_CORE6_IDLE_CFG			(PMU_AP_BASE + 0x30c)
#define PMU_AP_CORE7_IDLE_CFG			(PMU_AP_BASE + 0x310)

/* power down */
#define PMU_AP_IDLE_PWRDWN			BIT(0)
/* sram power down */
#define PMU_AP_IDLE_SRAM_PWRDWN			BIT(1)
/* enable wake up the memory controller */
#define PMU_AP_IDLE_WAKE_MCE			BIT(3)
/* disable memory controller software req */
#define PMU_AP_IDLE_MC_SW_REQ			BIT(4)

#define PMU_AP_IDLE_PWRDOWN_MASK		(PMU_AP_IDLE_PWRDWN | PMU_AP_IDLE_SRAM_PWRDWN | \
						 PMU_AP_IDLE_WAKE_MCE | PMU_AP_IDLE_MC_SW_REQ)
/* cci */
#define C0_RVBADDR_LO_ADDR			0xd4282db0
#define C0_RVBADDR_HI_ADDR			0xd4282db4
#define C1_RVBADDR_LO_ADDR			0xd4282eb0
#define C1_RVBADDR_HI_ADDR			0xd4282eb4

#define CCI_550_PLATFORM_CCI_ADDR		0xd8500000

/* relative to cci base */
#define CCI_550_STATUS				0x000c
/* status register bits */
#define CCI_550_STATUS_CHANGE_PENDING		BIT(0)

/* slave interface registers */
#define CCI_550_SLAVE_IFACE0_OFFSET		0x1000
#define CCI_550_SLAVE_IFACE_OFFSET(idx)		(CCI_550_SLAVE_IFACE0_OFFSET + ((0x1000) * (idx)))

/* relative to slave interface base */
#define CCI_550_SNOOP_CTRL			0x0000
/* snoop control register bits */
#define CCI_550_SNOOP_CTRL_ENABLE_SNOOPS	BIT(0)
#define CCI_550_SNOOP_CTRL_ENABLE_DVMS		BIT(1)

/* clusters and CPU mapping */
#define PLATFORM_MAX_CPUS			8
#define PLATFORM_MAX_CPUS_PER_CLUSTER		4
#define CPU_TO_CLUSTER(cpu)			((cpu) / PLATFORM_MAX_CPUS_PER_CLUSTER)

#define PLAT_CCI_CLUSTER0_IFACE_IX		0
#define PLAT_CCI_CLUSTER1_IFACE_IX		1

#endif
