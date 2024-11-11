/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2021 SiFive
 *
 * Authors:
 *   Green Wan <green.wan@sifive.com>
 */

#include <sbi/riscv_io.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_heap.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/gpio/fdt_gpio.h>

#define SIFIVE_GPIO_PINS_MIN	1
#define SIFIVE_GPIO_PINS_MAX	32
#define SIFIVE_GPIO_PINS_DEF	16

#define SIFIVE_GPIO_OUTEN	0x8
#define SIFIVE_GPIO_OUTVAL	0xc
#define SIFIVE_GPIO_BIT(b)	(1U << (b))

struct sifive_gpio_chip {
	unsigned long addr;
	struct gpio_chip chip;
};

static int sifive_gpio_direction_output(struct gpio_pin *gp, int value)
{
	unsigned int v;
	struct sifive_gpio_chip *chip =
		container_of(gp->chip, struct sifive_gpio_chip, chip);

	v = readl((volatile void *)(chip->addr + SIFIVE_GPIO_OUTEN));
	v |= SIFIVE_GPIO_BIT(gp->offset);
	writel(v, (volatile void *)(chip->addr + SIFIVE_GPIO_OUTEN));

	v = readl((volatile void *)(chip->addr + SIFIVE_GPIO_OUTVAL));
	if (!value)
		v &= ~SIFIVE_GPIO_BIT(gp->offset);
	else
		v |= SIFIVE_GPIO_BIT(gp->offset);
	writel(v, (volatile void *)(chip->addr + SIFIVE_GPIO_OUTVAL));

	return 0;
}

static void sifive_gpio_set(struct gpio_pin *gp, int value)
{
	unsigned int v;
	struct sifive_gpio_chip *chip =
		container_of(gp->chip, struct sifive_gpio_chip, chip);

	v = readl((volatile void *)(chip->addr + SIFIVE_GPIO_OUTVAL));
	if (!value)
		v &= ~SIFIVE_GPIO_BIT(gp->offset);
	else
		v |= SIFIVE_GPIO_BIT(gp->offset);
	writel(v, (volatile void *)(chip->addr + SIFIVE_GPIO_OUTVAL));
}

const struct fdt_gpio fdt_gpio_sifive;

static int sifive_gpio_init(const void *fdt, int nodeoff,
			    const struct fdt_match *match)
{
	int rc;
	struct sifive_gpio_chip *chip;
	uint64_t addr;

	chip = sbi_zalloc(sizeof(*chip));
	if (!chip)
		return SBI_ENOMEM;

	rc = fdt_get_node_addr_size(fdt, nodeoff, 0, &addr, NULL);
	if (rc) {
		sbi_free(chip);
		return rc;
	}

	chip->addr = addr;
	chip->chip.driver = &fdt_gpio_sifive;
	chip->chip.id = nodeoff;
	chip->chip.ngpio = SIFIVE_GPIO_PINS_DEF;
	chip->chip.direction_output = sifive_gpio_direction_output;
	chip->chip.set = sifive_gpio_set;
	rc = gpio_chip_add(&chip->chip);
	if (rc) {
		sbi_free(chip);
		return rc;
	}

	return 0;
}

static const struct fdt_match sifive_gpio_match[] = {
	{ .compatible = "sifive,gpio0" },
	{ },
};

const struct fdt_gpio fdt_gpio_sifive = {
	.driver = {
		.match_table = sifive_gpio_match,
		.init = sifive_gpio_init,
	},
	.xlate = fdt_gpio_simple_xlate,
};
