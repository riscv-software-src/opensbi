/*
 * SPDX-License-Identifier:    Apache-2.0
 *
 * Copyright 2018 Canaan Inc.
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <sbi/riscv_encoding.h>
#include "sysctl.h"

volatile sysctl_t *const sysctl = (volatile sysctl_t *)SYSCTL_BASE_ADDR;

#define SYSCTRL_CLOCK_FREQ_IN0 (26000000UL)

static u32 sysctl_pll0_get_freq(void)
{
	u32 freq_in, nr, nf, od;

	freq_in = SYSCTRL_CLOCK_FREQ_IN0;
	nr	= sysctl->pll0.clkr0 + 1;
	nf	= sysctl->pll0.clkf0 + 1;
	od	= sysctl->pll0.clkod0 + 1;

	/*
	 * Get final PLL output freq
	 * FOUT = FIN / NR * NF / OD
	 *      = (FIN * NF) / (NR * OD)
	 */
	return ((u64)freq_in * (u64)nf) / ((u64)nr * (u64)od);
}

u32 sysctl_get_cpu_freq(void)
{
	int clock_source;

	clock_source = (int)sysctl->clk_sel0.aclk_sel;
	switch (clock_source) {
	case 0:
		return SYSCTRL_CLOCK_FREQ_IN0;
	case 1:
		return sysctl_pll0_get_freq() /
		       (2ULL << (int)sysctl->clk_sel0.aclk_divider_sel);
	default:
		return 0;
	}
}
