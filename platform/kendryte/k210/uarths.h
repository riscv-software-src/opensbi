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

/*
 * Universal Asynchronous Receiver/Transmitter (UART)
 * The UART peripheral supports the following features:
 *
 * - 8-N-1 and 8-N-2 formats: 8 data bits, no parity bit, 1 start
 *   bit, 1 or 2 stop bits
 *
 * - 8-entry transmit and receive FIFO buffers with programmable
 *   watermark interrupts
 *
 * - 16× Rx oversampling with 2/3 majority voting per bit
 *
 * The UART peripheral does not support hardware flow control or
 * other modem control signals, or synchronous serial data
 * tranfesrs.
 *
 * UART RAM Layout
 * | Address   | Name     | Description                     |
 * |-----------|----------|---------------------------------|
 * | 0x000     | txdata   | Transmit data register          |
 * | 0x004     | rxdata   | Receive data register           |
 * | 0x008     | txctrl   | Transmit control register       |
 * | 0x00C     | rxctrl   | Receive control register        |
 * | 0x010     | ie       | UART interrupt enable           |
 * | 0x014     | ip       | UART Interrupt pending          |
 * | 0x018     | div      | Baud rate divisor               |
 */

#ifndef _K210_UARTHS_H_
#define _K210_UARTHS_H_

#include <sbi/sbi_types.h>

/* clang-format off */

/* Base register address */
#define UARTHS_BASE_ADDR	(0x38000000U)

/* Register address offsets */
#define UARTHS_REG_TXFIFO	0x00
#define UARTHS_REG_RXFIFO	0x04
#define UARTHS_REG_TXCTRL	0x08
#define UARTHS_REG_RXCTRL	0x0c
#define UARTHS_REG_IE		0x10
#define UARTHS_REG_IP		0x14
#define UARTHS_REG_DIV		0x18

/* TXCTRL register */
#define UARTHS_TXEN		0x01
#define UARTHS_TXWM(x)		(((x) & 0xffff) << 16)

/* RXCTRL register */
#define UARTHS_RXEN		0x01
#define UARTHS_RXWM(x)		(((x) & 0xffff) << 16)

/* IP register */
#define UARTHS_IP_TXWM		0x01
#define UARTHS_IP_RXWM		0x02

/* clang-format on */

struct uarths_txdata {
	/* Bits [7:0] is data */
	u32 data : 8;
	/* Bits [30:8] is 0 */
	u32 zero : 23;
	/* Bit 31 is full status */
	u32 full : 1;
} __attribute__((packed, aligned(4)));

struct uarths_rxdata {
	/* Bits [7:0] is data */
	u32 data : 8;
	/* Bits [30:8] is 0 */
	u32 zero : 23;
	/* Bit 31 is empty status */
	u32 empty : 1;
} __attribute__((packed, aligned(4)));

struct uarths_txctrl {
	/* Bit 0 is txen, controls whether the Tx channel is active. */
	u32 txen : 1;
	/* Bit 1 is nstop, 0 for one stop bit and 1 for two stop bits */
	u32 nstop : 1;
	/* Bits [15:2] is reserved */
	u32 resv0 : 14;
	/* Bits [18:16] is threshold of interrupt triggers */
	u32 txcnt : 3;
	/* Bits [31:19] is reserved */
	u32 resv1 : 13;
} __attribute__((packed, aligned(4)));

struct uarths_rxctrl {
	/* Bit 0 is txen, controls whether the Tx channel is active. */
	u32 rxen : 1;
	/* Bits [15:1] is reserved */
	u32 resv0 : 15;
	/* Bits [18:16] is threshold of interrupt triggers */
	u32 rxcnt : 3;
	/* Bits [31:19] is reserved */
	u32 resv1 : 13;
} __attribute__((packed, aligned(4)));

struct uarths_ip {
	/* Bit 0 is txwm, raised less than txcnt */
	u32 txwm : 1;
	/* Bit 1 is txwm, raised greater than rxcnt */
	u32 rxwm : 1;
	/* Bits [31:2] is 0 */
	u32 zero : 30;
} __attribute__((packed, aligned(4)));

struct uarths_ie {
	/* Bit 0 is txwm, raised less than txcnt */
	u32 txwm : 1;
	/* Bit 1 is txwm, raised greater than rxcnt */
	u32 rxwm : 1;
	/* Bits [31:2] is 0 */
	u32 zero : 30;
} __attribute__((packed, aligned(4)));

struct uarths_div {
	/* Bits [31:2] is baud rate divisor register */
	u32 div : 16;
	/* Bits [31:16] is 0 */
	u32 zero : 16;
} __attribute__((packed, aligned(4)));

struct uarths {
	/* Address offset 0x00 */
	struct uarths_txdata txdata;
	/* Address offset 0x04 */
	struct uarths_rxdata rxdata;
	/* Address offset 0x08 */
	struct uarths_txctrl txctrl;
	/* Address offset 0x0c */
	struct uarths_rxctrl rxctrl;
	/* Address offset 0x10 */
	struct uarths_ie ie;
	/* Address offset 0x14 */
	struct uarths_ip ip;
	/* Address offset 0x18 */
	struct uarths_div div;
} __attribute__((packed, aligned(4)));

enum uarths_stopbit { UARTHS_STOP_1, UARTHS_STOP_2 };

void uarths_init(u32 baud_rate, enum uarths_stopbit stopbit);
void uarths_putc(char c);
int uarths_getc(void);

#endif /* _K210_UARTHS_H_ */
