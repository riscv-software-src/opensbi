/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2021 SiFive
 * Copyright (c) 2021 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Green Wan <green.wan@sifive.com>
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <libfdt.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_system.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/gpio/fdt_gpio.h>
#include <sbi_utils/reset/fdt_reset.h>

struct gpio_reset {
	struct gpio_pin pin;
	u32 active_delay;
	u32 inactive_delay;
};

static struct gpio_reset poweroff = {
	.active_delay = 100,
	.inactive_delay = 100
};

static struct gpio_reset restart = {
	.active_delay = 100,
	.inactive_delay = 100
};

/* Custom mdelay function until we have a generic mdelay() API */
static void gpio_mdelay(unsigned long msecs)
{
	volatile int i;
	while (msecs--)
		for (i = 0; i < 100000; i++) ;
}

static int gpio_system_reset_check(u32 type, u32 reason)
{
	switch (type) {
	case SBI_SRST_RESET_TYPE_SHUTDOWN:
	case SBI_SRST_RESET_TYPE_COLD_REBOOT:
	case SBI_SRST_RESET_TYPE_WARM_REBOOT:
		return 1;
	}

	return 0;
}

static void gpio_system_reset(u32 type, u32 reason)
{
	struct gpio_reset *reset = NULL;

	switch (type) {
	case SBI_SRST_RESET_TYPE_SHUTDOWN:
		reset = &poweroff;
		break;
	case SBI_SRST_RESET_TYPE_COLD_REBOOT:
	case SBI_SRST_RESET_TYPE_WARM_REBOOT:
		reset = &restart;
		break;
	}

	if (reset) {
		if (!reset->pin.chip) {
			sbi_printf("%s: gpio pin not available\n", __func__);
			goto skip_reset;
		}

		/* drive it active, also inactive->active edge */
		gpio_direction_output(&reset->pin, 1);
		gpio_mdelay(reset->active_delay);

		/* drive inactive, also active->inactive edge */
		gpio_set(&reset->pin, 0);
		gpio_mdelay(reset->inactive_delay);

		/* drive it active, also inactive->active edge */
		gpio_set(&reset->pin, 1);

skip_reset:
		/* hang !!! */
		sbi_hart_hang();
	}
}

static struct sbi_system_reset_device gpio_reset = {
	.name = "gpio-reset",
	.system_reset_check = gpio_system_reset_check,
	.system_reset = gpio_system_reset
};

static int gpio_reset_init(void *fdt, int nodeoff,
			   const struct fdt_match *match)
{
	int rc, len;
	const fdt32_t *val;
	bool is_restart = (ulong)match->data;
	const char *dir_prop = (is_restart) ? "open-source" : "input";
	struct gpio_reset *reset = (is_restart) ? &restart : &poweroff;

	rc = fdt_gpio_pin_get(fdt, nodeoff, 0, &reset->pin);
	if (rc)
		return rc;

	if (fdt_getprop(fdt, nodeoff, dir_prop, &len)) {
		rc = gpio_direction_input(&reset->pin);
		if (rc)
			return rc;
	}

	val = fdt_getprop(fdt, nodeoff, "active-delay-ms", &len);
	if (len > 0)
		reset->active_delay = fdt32_to_cpu(*val);

	val = fdt_getprop(fdt, nodeoff, "inactive-delay-ms", &len);
	if (len > 0)
		reset->inactive_delay = fdt32_to_cpu(*val);

	sbi_system_reset_set_device(&gpio_reset);

	return 0;
}

static const struct fdt_match gpio_reset_match[] = {
	{ .compatible = "gpio-poweroff", .data = (void *)FALSE },
	{ .compatible = "gpio-restart", .data = (void *)TRUE },
	{ },
};

struct fdt_reset fdt_reset_gpio = {
	.match_table = gpio_reset_match,
	.init = gpio_reset_init,
};
