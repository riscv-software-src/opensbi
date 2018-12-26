/*
 * Copyright (c) 2018 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef __SBI_PLATFORM_H__
#define __SBI_PLATFORM_H__

#include <sbi/sbi_scratch.h>

enum sbi_platform_features {
	SBI_PLATFORM_HAS_MMIO_TIMER_VALUE	= (1 << 0),
	SBI_PLATFORM_HAS_HART_HOTPLUG		= (1 << 1),
	SBI_PLATFORM_HAS_PMP			= (1 << 2),
	SBI_PLATFORM_HAS_SCOUNTEREN		= (1 << 3),
	SBI_PLATFORM_HAS_MCOUNTEREN		= (1 << 4),
	SBI_PLATFORM_HAS_MFAULTS_DELEGATION	= (1 << 5),
};

#define SBI_PLATFORM_DEFAULT_FEATURES	\
	(SBI_PLATFORM_HAS_MMIO_TIMER_VALUE | \
	 SBI_PLATFORM_HAS_PMP | \
	 SBI_PLATFORM_HAS_SCOUNTEREN | \
	 SBI_PLATFORM_HAS_MCOUNTEREN | \
	 SBI_PLATFORM_HAS_MFAULTS_DELEGATION)

struct sbi_platform {
	char name[64];
	u64 features;
	u32 hart_count;
	u32 hart_stack_size;
	u64 disabled_hart_mask;
	int (*early_init)(u32 hartid, bool cold_boot);
	int (*final_init)(u32 hartid, bool cold_boot);
	u32 (*pmp_region_count)(u32 hartid);
	int (*pmp_region_info)(u32 hartid, u32 index,
			       ulong *prot, ulong *addr, ulong *log2size);
	void (*console_putc)(char ch);
	char (*console_getc)(void);
	int (*console_init)(void);
	int (*irqchip_init)(u32 hartid, bool cold_boot);
	void (*ipi_inject)(u32 target_hart, u32 source_hart);
	void (*ipi_sync)(u32 target_hart, u32 source_hart);
	void (*ipi_clear)(u32 target_hart);
	int (*ipi_init)(u32 hartid, bool cold_boot);
	u64 (*timer_value)(void);
	void (*timer_event_stop)(u32 target_hart);
	void (*timer_event_start)(u32 target_hart, u64 next_event);
	int (*timer_init)(u32 hartid, bool cold_boot);
	int (*system_reboot)(u32 type);
	int (*system_shutdown)(u32 type);
} __attribute__((packed));

#define sbi_platform_ptr(__s)	\
	((struct sbi_platform *)((__s)->platform_addr))
#define sbi_platform_thishart_ptr()	\
	((struct sbi_platform *)(sbi_scratch_thishart_ptr()->platform_addr))
#define sbi_platform_has_mmio_timer_value(__p)	\
	((__p)->features & SBI_PLATFORM_HAS_MMIO_TIMER_VALUE)
#define sbi_platform_has_hart_hotplug(__p)	\
	((__p)->features & SBI_PLATFORM_HAS_HART_HOTPLUG)
#define sbi_platform_has_pmp(__p)	\
	((__p)->features & SBI_PLATFORM_HAS_PMP)
#define sbi_platform_has_scounteren(__p)	\
	((__p)->features & SBI_PLATFORM_HAS_SCOUNTEREN)
#define sbi_platform_has_mcounteren(__p)	\
	((__p)->features & SBI_PLATFORM_HAS_MCOUNTEREN)
#define sbi_platform_has_mfaults_delegation(__p)	\
	((__p)->features & SBI_PLATFORM_HAS_MFAULTS_DELEGATION)

static inline const char *sbi_platform_name(struct sbi_platform *plat)
{
	if (plat)
		return plat->name;
	return NULL;
}

static inline bool sbi_platform_hart_disabled(struct sbi_platform *plat, u32 hartid)
{
	if (plat && (plat->disabled_hart_mask & (1 << hartid)))
		return 1;
	else
		return 0;
}
static inline u32 sbi_platform_hart_count(struct sbi_platform *plat)
{
	if (plat)
		return plat->hart_count;
	return 0;
}

static inline u32 sbi_platform_hart_stack_size(struct sbi_platform *plat)
{
	if (plat)
		return plat->hart_stack_size;
	return 0;
}

static inline int sbi_platform_early_init(struct sbi_platform *plat,
					  u32 hartid, bool cold_boot)
{
	if (plat && plat->early_init)
		return plat->early_init(hartid, cold_boot);
	return 0;
}

static inline int sbi_platform_final_init(struct sbi_platform *plat,
					  u32 hartid, bool cold_boot)
{
	if (plat && plat->final_init)
		return plat->final_init(hartid, cold_boot);
	return 0;
}

static inline u32 sbi_platform_pmp_region_count(struct sbi_platform *plat,
						u32 hartid)
{
	if (plat && plat->pmp_region_count)
		return plat->pmp_region_count(hartid);
	return 0;
}

static inline int sbi_platform_pmp_region_info(struct sbi_platform *plat,
					       u32 hartid, u32 index,
					       ulong *prot, ulong *addr,
					       ulong *log2size)
{
	if (plat && plat->pmp_region_info)
		return plat->pmp_region_info(hartid, index,
					     prot, addr, log2size);
	return 0;
}

static inline void sbi_platform_console_putc(struct sbi_platform *plat,
					     char ch)
{
	if (plat && plat->console_putc)
		plat->console_putc(ch);
}

static inline char sbi_platform_console_getc(struct sbi_platform *plat)
{
	if (plat && plat->console_getc)
		return plat->console_getc();
	return 0;
}

static inline int sbi_platform_console_init(struct sbi_platform *plat)
{
	if (plat && plat->console_init)
		return plat->console_init();
	return 0;
}

static inline int sbi_platform_irqchip_init(struct sbi_platform *plat,
					    u32 hartid, bool cold_boot)
{
	if (plat && plat->irqchip_init)
		return plat->irqchip_init(hartid, cold_boot);
	return 0;
}

static inline void sbi_platform_ipi_inject(struct sbi_platform *plat,
					   u32 target_hart, u32 source_hart)
{
	if (plat && plat->ipi_inject)
		plat->ipi_inject(target_hart, source_hart);
}

static inline void sbi_platform_ipi_sync(struct sbi_platform *plat,
					 u32 target_hart, u32 source_hart)
{
	if (plat && plat->ipi_sync)
		plat->ipi_sync(target_hart, source_hart);
}

static inline void sbi_platform_ipi_clear(struct sbi_platform *plat,
					  u32 target_hart)
{
	if (plat && plat->ipi_clear)
		plat->ipi_clear(target_hart);
}

static inline int sbi_platform_ipi_init(struct sbi_platform *plat,
					u32 hartid, bool cold_boot)
{
	if (plat && plat->ipi_init)
		return plat->ipi_init(hartid, cold_boot);
	return 0;
}

static inline u64 sbi_platform_timer_value(struct sbi_platform *plat)
{
	if (plat && plat->timer_value)
		return plat->timer_value();
	return 0;
}

static inline void sbi_platform_timer_event_stop(struct sbi_platform *plat,
						 u32 target_hart)
{
	if (plat && plat->timer_event_stop)
		plat->timer_event_stop(target_hart);
}

static inline void sbi_platform_timer_event_start(struct sbi_platform *plat,
						  u32 target_hart,
						  u64 next_event)
{
	if (plat && plat->timer_event_start)
		plat->timer_event_start(target_hart, next_event);
}

static inline int sbi_platform_timer_init(struct sbi_platform *plat,
					  u32 hartid, bool cold_boot)
{
	if (plat && plat->timer_init)
		return plat->timer_init(hartid, cold_boot);
	return 0;
}

static inline int sbi_platform_system_reboot(struct sbi_platform *plat,
					     u32 type)
{
	if (plat && plat->system_reboot)
		return plat->system_reboot(type);
	return 0;
}

static inline int sbi_platform_system_shutdown(struct sbi_platform *plat,
					       u32 type)
{
	if (plat && plat->system_shutdown)
		return plat->system_shutdown(type);
	return 0;
}

#endif
