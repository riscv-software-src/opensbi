/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) Copyright (c) 2023 Andes Technology Corporation
 */

#ifndef _RISCV_ANDES_PMU_H
#define _RISCV_ANDES_PMU_H

#include <sbi/sbi_hart.h>

int andes_pmu_init(void);
int andes_pmu_extensions_init(struct sbi_hart_features *hfeatures);

#endif /* _RISCV_ANDES_PMU_H */
