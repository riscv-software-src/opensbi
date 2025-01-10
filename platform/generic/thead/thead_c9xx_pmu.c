/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Authors:
 *   Inochi Amaoto <inochiama@outlook.com>
 *   Haijiao Liu <haijiao.liu@sophgo.com>
 *
 */

#include <thead/c9xx_encoding.h>
#include <thead/c9xx_pmu.h>
#include <sbi/riscv_asm.h>
#include <sbi/sbi_bitops.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_pmu.h>

static void thead_c9xx_pmu_ctr_enable_irq(uint32_t ctr_idx)
{
	if (ctr_idx >= SBI_PMU_HW_CTR_MAX)
		return;

	/**
	 * Clear out the OF bit so that next interrupt can be enabled.
	 * This should be done before starting interrupt to avoid unexcepted
	 * overflow interrupt.
	 */
	csr_clear(THEAD_C9XX_CSR_MCOUNTEROF, BIT(ctr_idx));

	/**
	 * This register is described in C9xx document as the control register
	 * for enabling writes to the superuser state counter. However, if the
	 * corresponding bit is not set to 1, scounterof will always read as 0
	 * when the counter register overflows.
	 */
	csr_set(THEAD_C9XX_CSR_MCOUNTERWEN, BIT(ctr_idx));

	/**
	 * SSCOFPMF uses the OF bit for enabling/disabling the interrupt,
	 * while the C9XX has designated enable bits.
	 * So enable per-counter interrupt on C9xx here.
	 */
	csr_set(THEAD_C9XX_CSR_MCOUNTERINTEN, BIT(ctr_idx));
}

static void thead_c9xx_pmu_ctr_disable_irq(uint32_t ctr_idx)
{
	/**
	 * There is no need to clear the bit of mcounterwen, it will expire
	 * after setting the csr mcountinhibit.
	 */
	csr_clear(THEAD_C9XX_CSR_MCOUNTERINTEN, BIT(ctr_idx));
}

static int thead_c9xx_pmu_irq_bit(void)
{
	return THEAD_C9XX_IRQ_PMU_OVF;
}

static const struct sbi_pmu_device thead_c9xx_pmu_device = {
	.name = "thead,c900-pmu",
	.hw_counter_enable_irq = thead_c9xx_pmu_ctr_enable_irq,
	.hw_counter_disable_irq = thead_c9xx_pmu_ctr_disable_irq,
	.hw_counter_irq_bit = thead_c9xx_pmu_irq_bit,
};

void thead_c9xx_register_pmu_device(void)
{
	sbi_pmu_set_device(&thead_c9xx_pmu_device);
}
