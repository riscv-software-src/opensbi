/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 SiFive Inc.
 */

#ifndef __FDT_HSM_SIFIVE_TMC0_H__
#define __FDT_HSM_SIFIVE_TMC0_H__

int sifive_tmc0_set_wakemask_enareq(u32 hartid);
void sifive_tmc0_set_wakemask_disreq(u32 hartid);
bool sifive_tmc0_is_pg(u32 hartid);

#endif
