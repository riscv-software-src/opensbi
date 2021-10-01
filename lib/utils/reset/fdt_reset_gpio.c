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
#include <sbi/sbi_timer.h>
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

static struct gpio_reset *gpio_get_reset_settings(u32 type)
{
	struct gpio_reset *reset;

	switch (type) {
	case SBI_SRST_RESET_TYPE_SHUTDOWN:
		reset = &poweroff;
		break;
	case SBI_SRST_RESET_TYPE_COLD_REBOOT:
	case SBI_SRST_RESET_TYPE_WARM_REBOOT:
		reset = &restart;
		break;
	default:
		reset = NULL;
	}

	if (reset && !reset->pin.chip)
		reset = NULL;

	return reset;
}

static int gpio_system_reset_check(u32 type, u32 reason)
{
	return !!gpio_get_reset_settings(type);
}

static void gpio_system_reset(u32 type, u32 reason)
{
	struct gpio_reset *reset = gpio_get_reset_settings(type);

	if (reset) {
		/* drive it active, also inactive->active edge */
		gpio_direction_output(&reset->pin, 1);
		sbi_timer_mdelay(reset->active_delay);

		/* drive inactive, also active->inactive edge */
		gpio_set(&reset->pin, 0);
		sbi_timer_mdelay(reset->inactive_delay);

		/* drive it active, also inactive->active edge */
		gpio_set(&reset->pin, 1);
	}
	/* hang !!! */
	sbi_hart_hang();
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

	sbi_system_reset_add_device(&gpio_reset);

	return 0;
}

static const struct fdt_match gpio_poweroff_match[] = {
	{ .compatible = "gpio-poweroff", .data = (void *)FALSE },
	{ },
};

struct fdt_reset fdt_poweroff_gpio = {
	.match_table = gpio_poweroff_match,
	.init = gpio_reset_init,
};

static const struct fdt_match gpio_reset_match[] = {
	{ .compatible = "gpio-restart", .data = (void *)TRUE },
	{ },
};

struct fdt_reset fdt_reset_gpio = {
	.match_table = gpio_reset_match,
	.init = gpio_reset_init,
};
