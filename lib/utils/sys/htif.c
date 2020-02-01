/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2010-2020, The Regents of the University of California
 * (Regents).  All Rights Reserved.
 */

#include <sbi/riscv_locks.h>
#include <sbi_utils/sys/htif.h>

#if __riscv_xlen == 64
# define TOHOST_CMD(dev, cmd, payload) \
  (((uint64_t)(dev) << 56) | ((uint64_t)(cmd) << 48) | (uint64_t)(payload))
#else
# define TOHOST_CMD(dev, cmd, payload) ({ \
  if ((dev) || (cmd)) __builtin_trap(); \
  (payload); })
#endif
#define FROMHOST_DEV(fromhost_value) ((uint64_t)(fromhost_value) >> 56)
#define FROMHOST_CMD(fromhost_value) ((uint64_t)(fromhost_value) << 8 >> 56)
#define FROMHOST_DATA(fromhost_value) ((uint64_t)(fromhost_value) << 16 >> 16)

#define PK_SYS_write 64

volatile uint64_t tohost __attribute__((section(".htif")));
volatile uint64_t fromhost __attribute__((section(".htif")));
static int htif_console_buf;
static spinlock_t htif_lock = SPIN_LOCK_INITIALIZER;

static void __check_fromhost()
{
	uint64_t fh = fromhost;
	if (!fh)
		return;
	fromhost = 0;

	// this should be from the console
	if (FROMHOST_DEV(fh) != 1) __builtin_trap();
	switch (FROMHOST_CMD(fh)) {
		case 0:
			htif_console_buf = 1 + (uint8_t)FROMHOST_DATA(fh);
			break;
		case 1:
			break;
		default:
			__builtin_trap();
	}
}

static void __set_tohost(uint64_t dev, uint64_t cmd, uint64_t data)
{
	while (tohost)
		__check_fromhost();
	tohost = TOHOST_CMD(dev, cmd, data);
}

#if __riscv_xlen == 32
static void do_tohost_fromhost(uint64_t dev, uint64_t cmd, uint64_t data)
{
	spin_lock(&htif_lock);
	__set_tohost(0, cmd, data);

	while (1) {
		uint64_t fh = fromhost;
		if (fh) {
			if (FROMHOST_DEV(fh) == 0 && FROMHOST_CMD(fh) == cmd) {
				fromhost = 0;
				break;
			}
			__check_fromhost();
		}
	}
	spin_unlock(&htif_lock);
}

void htif_putc(char ch)
{
	// HTIF devices are not supported on RV32, so proxy a write system call
	volatile uint64_t magic_mem[8];
	magic_mem[0] = PK_SYS_write;
	magic_mem[1] = 1;
	magic_mem[2] = (uint64_t)(uintptr_t)&ch;
	magic_mem[3] = 1;
	do_tohost_fromhost(0, 0, (uint64_t)(uintptr_t)magic_mem);
}
#else
void htif_putc(char ch)
{
	spin_lock(&htif_lock);
	__set_tohost(1, 1, ch);
	spin_unlock(&htif_lock);
}
#endif

int htif_getc(void)
{
#if __riscv_xlen == 32
	// HTIF devices are not supported on RV32
	return -1;
#endif

	spin_lock(&htif_lock);
	__check_fromhost();
	int ch = htif_console_buf;
	if (ch >= 0) {
		htif_console_buf = -1;
		__set_tohost(1, 0, 0);
	}
	spin_unlock(&htif_lock);

	return ch - 1;
}

int htif_system_down(u32 type)
{
	while (1) {
		fromhost = 0;
		tohost = 1;
	}
}
