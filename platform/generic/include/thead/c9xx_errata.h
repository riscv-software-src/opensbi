
#ifndef __RISCV_THEAD_C9XX_ERRATA_H____
#define __RISCV_THEAD_C9XX_ERRATA_H____

/**
 * T-HEAD board with this quirk need to execute sfence.vma to flush
 * stale entrie avoid incorrect memory access.
 */
#define THEAD_QUIRK_ERRATA_TLB_FLUSH		BIT(0)
#define THEAD_QUIRK_ERRATA_THEAD_PMU		BIT(1)

void thead_register_tlb_flush_trap_handler(void);

#endif // __RISCV_THEAD_C9XX_ERRATA_H____
