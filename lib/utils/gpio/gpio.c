/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2021 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <sbi/sbi_error.h>
#include <sbi_utils/gpio/gpio.h>

#define GPIO_CHIP_MAX		16

static struct gpio_chip *gc_array[GPIO_CHIP_MAX];

struct gpio_chip *gpio_chip_find(unsigned int id)
{
	unsigned int i;
	struct gpio_chip *ret = NULL;

	for (i = 0; i < GPIO_CHIP_MAX; i++) {
		if (gc_array[i] && gc_array[i]->id == id) {
			ret = gc_array[i];
			break;
		}
	}

	return ret;
}

int gpio_chip_add(struct gpio_chip *gc)
{
	int i, ret = SBI_ENOSPC;

	if (!gc)
		return SBI_EINVAL;
	if (gpio_chip_find(gc->id))
		return SBI_EALREADY;

	for (i = 0; i < GPIO_CHIP_MAX; i++) {
		if (!gc_array[i]) {
			gc_array[i] = gc;
			ret = 0;
			break;
		}
	}

	return ret;
}

void gpio_chip_remove(struct gpio_chip *gc)
{
	int i;

	if (!gc)
		return;

	for (i = 0; i < GPIO_CHIP_MAX; i++) {
		if (gc_array[i] == gc) {
			gc_array[i] = NULL;
			break;
		}
	}
}

int gpio_get_direction(struct gpio_pin *gp)
{
	if (!gp || !gp->chip || (gp->chip->ngpio <= gp->offset))
		return SBI_EINVAL;
	if (!gp->chip->get_direction)
		return SBI_ENOSYS;

	return gp->chip->get_direction(gp);
}

int gpio_direction_input(struct gpio_pin *gp)
{
	if (!gp || !gp->chip || (gp->chip->ngpio <= gp->offset))
		return SBI_EINVAL;
	if (!gp->chip->direction_input)
		return SBI_ENOSYS;

	return gp->chip->direction_input(gp);
}

int gpio_direction_output(struct gpio_pin *gp, int value)
{
	if (!gp || !gp->chip || (gp->chip->ngpio <= gp->offset))
		return SBI_EINVAL;
	if (!gp->chip->direction_output)
		return SBI_ENOSYS;

	return gp->chip->direction_output(gp, value);
}

int gpio_get(struct gpio_pin *gp)
{
	if (!gp || !gp->chip || (gp->chip->ngpio <= gp->offset))
		return SBI_EINVAL;
	if (!gp->chip->get)
		return SBI_ENOSYS;

	return gp->chip->get(gp);
}

int gpio_set(struct gpio_pin *gp, int value)
{
	if (!gp || !gp->chip || (gp->chip->ngpio <= gp->offset))
		return SBI_EINVAL;
	if (!gp->chip->set)
		return SBI_ENOSYS;

	gp->chip->set(gp, value);
	return 0;
}
