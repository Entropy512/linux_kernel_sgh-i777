/*
 * linux/arch/arm/plat-samsung/irq-gpio.c
 *
 * Copyright (C) 2009-2010 Samsung Electronics Co.Ltd
 * Author: Kyungmin Park <kyungmin.park@samsung.com>
 * Author: Joonyoung Shim <jy0922.shim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/io.h>

#include <plat/gpio-core.h>

#define GPIO_BASE(chip)		(((unsigned long)(chip)->base) & ~(SZ_4K - 1))

#define CON_OFFSET			0x700
#define MASK_OFFSET			0x900
#define PEND_OFFSET			0xA00
#define REG_OFFSET(x)			((x) << 2)

#define SAMSUNG_IRQ_GPIO_LEVEL_LOW	0x0
#define SAMSUNG_IRQ_GPIO_LEVEL_HIGH	0x1
#define SAMSUNG_IRQ_GPIO_EDGE_FALLING	0x2
#define SAMSUNG_IRQ_GPIO_EDGE_RISING	0x3
#define SAMSUNG_IRQ_GPIO_EDGE_BOTH	0x4

#define IRQ_GPIO_GROUP(x)		(IRQ_GPIO_BASE + ((x) * 8))

static inline int samsung_irq_gpio_get_bit(unsigned int irq, int group)
{
	return irq - (IRQ_GPIO_GROUP(group));
}

static void samsung_irq_gpio_ack(unsigned int irq)
{
	struct s3c_gpio_chip *chip = get_irq_chip_data(irq);
	int group, n, offset, value;

	group = chip->group;
	n = samsung_irq_gpio_get_bit(irq, group);

	offset = (group < 16) ? REG_OFFSET(group) : REG_OFFSET(group - 16);

	value = __raw_readl(GPIO_BASE(chip) + PEND_OFFSET + offset);
	value |= BIT(n);
	__raw_writel(value, GPIO_BASE(chip) + PEND_OFFSET + offset);
}

static void samsung_irq_gpio_mask(unsigned int irq)
{
	struct s3c_gpio_chip *chip = get_irq_chip_data(irq);
	int group, n, offset, value;

	group = chip->group;
	n = samsung_irq_gpio_get_bit(irq, group);

	offset = (group < 16) ? REG_OFFSET(group) : REG_OFFSET(group - 16);

	value = __raw_readl(GPIO_BASE(chip) + MASK_OFFSET + offset);
	value |= BIT(n);
	__raw_writel(value, GPIO_BASE(chip) + MASK_OFFSET + offset);
}

static void samsung_irq_gpio_mask_ack(unsigned int irq)
{
	samsung_irq_gpio_mask(irq);
	samsung_irq_gpio_ack(irq);
}

static void samsung_irq_gpio_unmask(unsigned int irq)
{
	struct s3c_gpio_chip *chip = get_irq_chip_data(irq);
	int group, n, offset, value;

	group = chip->group;
	n = samsung_irq_gpio_get_bit(irq, group);

	offset = (group < 16) ? REG_OFFSET(group) : REG_OFFSET(group - 16);

	value = __raw_readl(GPIO_BASE(chip) + MASK_OFFSET + offset);
	value &= ~BIT(n);
	__raw_writel(value, GPIO_BASE(chip) + MASK_OFFSET + offset);
}

static int samsung_irq_gpio_set_type(unsigned int irq, unsigned int type)
{
	struct s3c_gpio_chip *chip = get_irq_chip_data(irq);
	int group, bit, offset, value, eint_con;
	struct irq_desc *desc = irq_to_desc(irq);

	group = chip->group;
	bit = samsung_irq_gpio_get_bit(irq, group);

	switch (type) {
	case IRQ_TYPE_EDGE_RISING:
		eint_con = SAMSUNG_IRQ_GPIO_EDGE_RISING;
		break;
	case IRQ_TYPE_EDGE_FALLING:
		eint_con = SAMSUNG_IRQ_GPIO_EDGE_FALLING;
		break;
	case IRQ_TYPE_EDGE_BOTH:
		eint_con = SAMSUNG_IRQ_GPIO_EDGE_BOTH;
		break;
	case IRQ_TYPE_LEVEL_HIGH:
		eint_con = SAMSUNG_IRQ_GPIO_LEVEL_HIGH;
		break;
	case IRQ_TYPE_LEVEL_LOW:
		eint_con = SAMSUNG_IRQ_GPIO_LEVEL_LOW;
		break;
	case IRQ_TYPE_NONE:
		printk(KERN_WARNING "No irq type\n");
	default:
		return -EINVAL;
	}

	offset = (group < 16) ? REG_OFFSET(group) : REG_OFFSET(group - 16);
	bit = bit << 2;			/* 4 bits offset */

	value = __raw_readl(GPIO_BASE(chip) + CON_OFFSET + offset);
	value &= ~(0xf << bit);
	value |= (eint_con << bit);
	__raw_writel(value, GPIO_BASE(chip) + CON_OFFSET + offset);

	if (type & IRQ_TYPE_EDGE_BOTH)
		desc->handle_irq = handle_edge_irq;
	else
		desc->handle_irq = handle_level_irq;

	return 0;
}

static struct irq_chip samsung_irq_gpio = {
	.name		= "GPIO",
	.ack		= samsung_irq_gpio_ack,
	.mask		= samsung_irq_gpio_mask,
	.mask_ack	= samsung_irq_gpio_mask_ack,
	.unmask		= samsung_irq_gpio_unmask,
	.set_type	= samsung_irq_gpio_set_type,
};

static int samsung_irq_gpio_to_irq(struct gpio_chip *gpio, unsigned int offset)
{
	struct s3c_gpio_chip *chip = to_s3c_gpio(gpio);

	return IRQ_GPIO_GROUP(chip->group) + offset;
}

void __init samsung_irq_gpio_add(struct s3c_gpio_chip *chip)
{
	int irq, i;

	chip->chip.to_irq = samsung_irq_gpio_to_irq;

	for (i = 0; i < chip->chip.ngpio; i++) {
		irq = IRQ_GPIO_GROUP(chip->group) + i;
		set_irq_chip(irq, &samsung_irq_gpio);
		set_irq_chip_data(irq, chip);
		set_irq_handler(irq, handle_level_irq);
		set_irq_flags(irq, IRQF_VALID | IRQF_PROBE);
	}
}

/*
 * Note that this 'irq' is the real repesentative irq for GPIO
 *
 * To reduce the register access, use the valid group cache.
 * first check the valid groups. if failed, scan all groups fully.
 */
static void samsung_irq_gpio_handler(unsigned int irq, struct irq_desc *desc)
{
	struct samsung_irq_gpio *gpio;
	int group, n, offset;
	int start, end, pend, mask, handled = 0;
	struct irq_chip *chip = get_irq_chip(irq);

	gpio = get_irq_data(irq);
	start = gpio->start;
	end = gpio->nr_groups;

	/* primary controller ack'ing */
	if (chip->ack)
		chip->ack(irq);

	/* Check the valid group first */
	for (group = 0; group <= end; group++) {
		if (!test_bit(group, &gpio->valid_groups))
			continue;

		offset = REG_OFFSET(group);	/* 4 bytes offset */
		pend = __raw_readl(gpio->base + PEND_OFFSET + offset);
		if (!pend)
			continue;

		mask = __raw_readl(gpio->base + MASK_OFFSET + offset);
		pend &= ~mask;

		while (pend) {
			n = fls(pend) - 1;
			generic_handle_irq(IRQ_GPIO_GROUP(start + group) + n);
			pend &= ~BIT(n);
		}

		handled = 1;
	}

	if (handled)
		goto out;

	/* Okay we can't find a proper handler. Scan fully */
	for (group = 0; group <= end; group++) {
		offset = REG_OFFSET(group);	/* 4 bytes offset */
		pend = __raw_readl(gpio->base + PEND_OFFSET + offset);
		if (!pend)
			continue;

		mask = __raw_readl(gpio->base + MASK_OFFSET + offset);
		pend &= ~mask;

		while (pend) {
			n = fls(pend) - 1;
			generic_handle_irq(IRQ_GPIO_GROUP(start + group) + n);
			pend &= ~BIT(n);
		}

		/* It found the valid group */
		set_bit(group, &gpio->valid_groups);
	}

out:
	/* primary controller unmasking */
	chip->unmask(irq);
}

void __init samsung_irq_gpio_register(struct samsung_irq_gpio_info *gpios, int num)
{
	struct samsung_irq_gpio_info *gpio = gpios;
	struct samsung_irq_gpio *sig;
	int i;

	for (i = 0; i < num; i++, gpio++) {
		sig = kmemdup(&gpio->sig, sizeof(gpio->sig), GFP_KERNEL);
		if (!sig)
			continue;

		sig->valid_groups = 0;
		/* Use the default irq gpio handler, if no handler is defined */
		if (!gpio->handler)
			gpio->handler = samsung_irq_gpio_handler;

		set_irq_chained_handler(gpio->irq, gpio->handler);
		set_irq_data(gpio->irq, sig);
	}
}
