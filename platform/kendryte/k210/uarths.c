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

#include "sysctl.h"
#include "uarths.h"

static volatile struct uarths *const uarths =
	(volatile struct uarths *)UARTHS_BASE_ADDR;

void uarths_init(u32 baud_rate, enum uarths_stopbit stopbit)
{
	u32 freq = sysctl_get_cpu_freq();
	u16 div	 = freq / baud_rate - 1;

	/* Set UART registers */
	uarths->div.div	     = div;
	uarths->txctrl.nstop = stopbit;
	uarths->txctrl.txen  = 1;
	uarths->rxctrl.rxen  = 1;
	uarths->txctrl.txcnt = 0;
	uarths->rxctrl.rxcnt = 0;
	uarths->ip.txwm	     = 1;
	uarths->ip.rxwm	     = 0;
	uarths->ie.txwm	     = 1;
	uarths->ie.rxwm	     = 0;

	/* Clear input */
	if (!uarths->rxdata.empty)
		(void)uarths_getc();
}

void uarths_putc(char c)
{
	while (uarths->txdata.full)
		;

	uarths->txdata.data = (u8)c;
}

int uarths_getc(void)
{
	struct uarths_rxdata rx = uarths->rxdata;

	if (rx.empty)
		return -1;

	return rx.data;
}
