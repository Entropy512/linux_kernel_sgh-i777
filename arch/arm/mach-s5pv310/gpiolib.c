/* linux/arch/arm/mach-s5pv310/gpiolib.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S5PV310 - GPIOlib support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <plat/gpio-core.h>
#include <plat/gpio-cfg.h>
#include <plat/gpio-cfg-helpers.h>
#include <mach/map.h>

static struct s3c_gpio_cfg gpio_cfg = {
	.set_config	= s3c_gpio_setcfg_s3c64xx_4bit,
	.set_pull	= s3c_gpio_setpull_updown,
	.get_pull	= s3c_gpio_getpull_updown,
};

static struct s3c_gpio_cfg gpio_cfg_noint = {
	.set_config	= s3c_gpio_setcfg_s3c64xx_4bit,
	.set_pull	= s3c_gpio_setpull_updown,
	.get_pull	= s3c_gpio_getpull_updown,
};

static struct s3c_gpio_cfg gpio_cfg_extint = {
	.set_config	= s3c_gpio_setcfg_s3c64xx_4bit,
	.set_pull	= s3c_gpio_setpull_updown,
	.get_pull	= s3c_gpio_getpull_updown,
};

static int s5pv310_gpio2int(struct gpio_chip *chip, unsigned pin)
{
	int ret = 0;
	int base = chip->base;

	switch (base) {
		case S5PV310_GPX0(0):
			ret = IRQ_EINT(0) + pin;
			break;
		case S5PV310_GPX1(0):
			ret = IRQ_EINT(8) + pin;
			break;
		case S5PV310_GPX2(0):
			ret = IRQ_EINT(16) + pin;
			break;
		case S5PV310_GPX3(0):
			ret = IRQ_EINT(24) + pin;
			break;
	}

	return ret;
}

/* GPIO bank's base address given the index of the bank in the
 * list of all gpio banks.
 */
#define S5PV310_BANK_BASE1(bank_nr)	(S5P_VA_GPIO + ((bank_nr) * 0x20))
#define S5PV310_BANK_BASE2(bank_nr)	(S5PV310_VA_GPIO2 + ((bank_nr) * 0x20))
#define S5PV310_BANK_BASE3(bank_nr)	(S5PV310_VA_GPIO3 + ((bank_nr) * 0x20))

/*
 * Following are the gpio banks in v310.
 *
 * The 'config' member when left to NULL, is initialized to the default
 * structure gpio_cfg in the init function below.
 *
 * The 'base' member is also initialized in the init function below.
 * Note: The initialization of 'base' member of s3c_gpio_chip structure
 * uses the above macro and depends on the banks being listed in order here.
 */
static struct s3c_gpio_chip s5pv310_gpio_part1_4bit[] = {
	{
		.chip	= {
			.base	= S5PV310_GPA0(0),
			.ngpio	= S5PV310_GPIO_A0_NR,
			.label	= "GPA0",
		},
	}, {
		.chip	= {
			.base	= S5PV310_GPA1(0),
			.ngpio	= S5PV310_GPIO_A1_NR,
			.label	= "GPA1",
		},
	}, {
		.chip	= {
			.base	= S5PV310_GPB(0),
			.ngpio	= S5PV310_GPIO_B_NR,
			.label	= "GPB",
		},
	}, {
		.chip	= {
			.base	= S5PV310_GPC0(0),
			.ngpio	= S5PV310_GPIO_C0_NR,
			.label	= "GPC0",
		},
	}, {
		.chip	= {
			.base	= S5PV310_GPC1(0),
			.ngpio	= S5PV310_GPIO_C1_NR,
			.label	= "GPC1",
		},
	}, {
		.chip	= {
			.base	= S5PV310_GPD0(0),
			.ngpio	= S5PV310_GPIO_D0_NR,
			.label	= "GPD0",
		},
	}, {
		.chip	= {
			.base	= S5PV310_GPD1(0),
			.ngpio	= S5PV310_GPIO_D1_NR,
			.label	= "GPD1",
		},
	}, {
		.chip	= {
			.base	= S5PV310_GPE0(0),
			.ngpio	= S5PV310_GPIO_E0_NR,
			.label	= "GPE0",
		},
	}, {
		.chip	= {
			.base	= S5PV310_GPE1(0),
			.ngpio	= S5PV310_GPIO_E1_NR,
			.label	= "GPE1",
		},
	}, {
		.chip	= {
			.base	= S5PV310_GPE2(0),
			.ngpio	= S5PV310_GPIO_E2_NR,
			.label	= "GPE2",
		},
	}, {
		.chip	= {
			.base	= S5PV310_GPE3(0),
			.ngpio	= S5PV310_GPIO_E3_NR,
			.label	= "GPE3",
		},
	}, {
		.chip	= {
			.base	= S5PV310_GPE4(0),
			.ngpio	= S5PV310_GPIO_E4_NR,
			.label	= "GPE4",
		},
	}, {
		.chip	= {
			.base	= S5PV310_GPF0(0),
			.ngpio	= S5PV310_GPIO_F0_NR,
			.label	= "GPF0",
		},
	}, {
		.chip	= {
			.base	= S5PV310_GPF1(0),
			.ngpio	= S5PV310_GPIO_F1_NR,
			.label	= "GPF1",
		},
	}, {
		.chip	= {
			.base	= S5PV310_GPF2(0),
			.ngpio	= S5PV310_GPIO_F2_NR,
			.label	= "GPF2",
		},
	}, {
		.chip	= {
			.base	= S5PV310_GPF3(0),
			.ngpio	= S5PV310_GPIO_F3_NR,
			.label	= "GPF3",
		},
	},
};

static struct s3c_gpio_chip s5pv310_gpio_part2_4bit[] = {
	{
		.chip	= {
			.base	= S5PV310_GPJ0(0),
			.ngpio	= S5PV310_GPIO_J0_NR,
			.label	= "GPJ0",
		},
	}, {
		.chip	= {
			.base	= S5PV310_GPJ1(0),
			.ngpio	= S5PV310_GPIO_J1_NR,
			.label	= "GPJ1",
		},
	}, {
		.chip	= {
			.base	= S5PV310_GPK0(0),
			.ngpio	= S5PV310_GPIO_K0_NR,
			.label	= "GPK0",
		},
	}, {
		.chip	= {
			.base	= S5PV310_GPK1(0),
			.ngpio	= S5PV310_GPIO_K1_NR,
			.label	= "GPK1",
		},
	}, {
		.chip	= {
			.base	= S5PV310_GPK2(0),
			.ngpio	= S5PV310_GPIO_K2_NR,
			.label	= "GPK2",
		},
	}, {
		.chip	= {
			.base	= S5PV310_GPK3(0),
			.ngpio	= S5PV310_GPIO_K3_NR,
			.label	= "GPK3",
		},
	}, {
		.chip	= {
			.base	= S5PV310_GPL0(0),
			.ngpio	= S5PV310_GPIO_L0_NR,
			.label	= "GPL0",
		},
	}, {
		.chip	= {
			.base	= S5PV310_GPL1(0),
			.ngpio	= S5PV310_GPIO_L1_NR,
			.label	= "GPL1",
		},
	}, {
			.chip	= {
			.base	= S5PV310_GPL2(0),
			.ngpio	= S5PV310_GPIO_L2_NR,
			.label	= "GPL2",
		},
	}, {
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV310_MP00(0),
			.ngpio	= S5PV310_GPIO_MP00_NR,
			.label	= "MP00",
		},
	}, {
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV310_MP01(0),
			.ngpio	= S5PV310_GPIO_MP01_NR,
			.label	= "MP01",
		},
	}, {
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV310_MP02(0),
			.ngpio	= S5PV310_GPIO_MP02_NR,
			.label	= "MP02",
		},
	}, {
		.base	= (S5PV310_VA_GPIO2 + 0xC00),
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV310_GPX0(0),
			.ngpio	= S5PV310_GPIO_X0_NR,
			.label	= "GPX0",
			.to_irq	= s5pv310_gpio2int,
		},
	}, {
		.base	= (S5PV310_VA_GPIO2 + 0xC20),
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV310_GPX1(0),
			.ngpio	= S5PV310_GPIO_X1_NR,
			.label	= "GPX1",
			.to_irq	= s5pv310_gpio2int,
		},
	}, {
		.base	= (S5PV310_VA_GPIO2 + 0xC40),
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV310_GPX2(0),
			.ngpio	= S5PV310_GPIO_X2_NR,
			.label	= "GPX2",
			.to_irq	= s5pv310_gpio2int,
		},
	}, {
		.base	= (S5PV310_VA_GPIO2 + 0xC60),
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV310_GPX3(0),
			.ngpio	= S5PV310_GPIO_X3_NR,
			.label	= "GPX3",
			.to_irq	= s5pv310_gpio2int,
		},
	}, {
		.base	= (S5PV310_VA_GPIO2 + 0x120),
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV310_GPY0(0),
			.ngpio	= S5PV310_GPIO_Y0_NR,
			.label	= "GPY0",
		},
	}, {
		.base	= (S5PV310_VA_GPIO2 + 0x140),
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV310_GPY1(0),
			.ngpio	= S5PV310_GPIO_Y1_NR,
			.label	= "GPY1",
		},
	}, {
		.base	= (S5PV310_VA_GPIO2 + 0x160),
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV310_GPY2(0),
			.ngpio	= S5PV310_GPIO_Y2_NR,
			.label	= "GPY2",
		},
	}, {
		.base	= (S5PV310_VA_GPIO2 + 0x180),
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV310_GPY3(0),
			.ngpio	= S5PV310_GPIO_Y3_NR,
			.label	= "GPY3",
		},
	}, {
		.base	= (S5PV310_VA_GPIO2 + 0x1A0),
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV310_GPY4(0),
			.ngpio	= S5PV310_GPIO_Y4_NR,
			.label	= "GPY4",
		},
	}, {
		.base	= (S5PV310_VA_GPIO2 + 0x1C0),
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV310_GPY5(0),
			.ngpio	= S5PV310_GPIO_Y5_NR,
			.label	= "GPY5",
		},
	}, {
		.base	= (S5PV310_VA_GPIO2 + 0x1E0),
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV310_GPY6(0),
			.ngpio	= S5PV310_GPIO_Y6_NR,
			.label	= "GPY6",
		},
	},
};

static struct s3c_gpio_chip s5pv310_gpio_part3_4bit[] = {
	{
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV310_GPZ(0),
			.ngpio	= S5PV310_GPIO_Z_NR,
			.label	= "GPZ",
		},
	},
};

/* S5PV310 machine dependent GPIO help function */
int s3c_gpio_slp_cfgpin(unsigned int pin, unsigned int config)
{
	struct s3c_gpio_chip *chip = s3c_gpiolib_getchip(pin);
	void __iomem *reg;
	unsigned long flags;
	int offset;
	u32 con;
	int shift;

	if (!chip)
		return -EINVAL;

	if ((pin >= S5PV310_GPX0(0)) && (pin <= S5PV310_GPX3(7)))
		return -EINVAL;

	if (config > S3C_GPIO_SLP_PREV)
		return -EINVAL;

	reg = chip->base + 0x10;

	offset = pin - chip->chip.base;
	shift = offset * 2;

	local_irq_save(flags);

	con = __raw_readl(reg);
	con &= ~(3 << shift);
	con |= config << shift;
	__raw_writel(con, reg);

	local_irq_restore(flags);
	return 0;
}

s3c_gpio_pull_t s3c_gpio_get_slp_cfgpin(unsigned int pin)
{
	struct s3c_gpio_chip *chip = s3c_gpiolib_getchip(pin);
	void __iomem *reg;
	unsigned long flags;
	int offset;
	u32 con;
	int shift;

	if (!chip)
		return -EINVAL;

	if ((pin >= S5PV310_GPX0(0)) && (pin <= S5PV310_GPX3(7)))
		return -EINVAL;

	reg = chip->base + 0x10;

	offset = pin - chip->chip.base;
	shift = offset * 2;

	local_irq_save(flags);

	con = __raw_readl(reg);
	con >>= shift;
	con &= 0x3;

	local_irq_restore(flags);

	return (__force s3c_gpio_pull_t)con;
}

int s3c_gpio_slp_setpull_updown(unsigned int pin, unsigned int config)
{
	struct s3c_gpio_chip *chip = s3c_gpiolib_getchip(pin);
	void __iomem *reg;
	unsigned long flags;
	int offset;
	u32 con;
	int shift;

	if (!chip)
		return -EINVAL;

	if ((pin >= S5PV310_GPX0(0)) && (pin <= S5PV310_GPX3(7)))
		return -EINVAL;

	if (config > S3C_GPIO_PULL_UP)
		return -EINVAL;

	reg = chip->base + 0x14;

	offset = pin - chip->chip.base;
	shift = offset * 2;

	local_irq_save(flags);

	con = __raw_readl(reg);
	con &= ~(3 << shift);
	con |= config << shift;
	__raw_writel(con, reg);

	local_irq_restore(flags);

	return 0;
}

static int s5pc210_extint_to_irq(struct gpio_chip *gpio, unsigned int offset)
{
	struct s3c_gpio_chip *chip = to_s3c_gpio(gpio);
	int irq;

	irq = (chip->group * 8) + offset;
	return IRQ_EINT(irq);
}

static __init int s5pv310_gpiolib_init(void)
{
#ifdef CONFIG_SAMSUNG_IRQ_GPIO
	struct samsung_irq_gpio_info gpio[2];
#endif
	struct s3c_gpio_chip *chip;
	int i;
	int nr_chips;
	int group = 0, extint = 0;

	chip = s5pv310_gpio_part1_4bit;
	nr_chips = ARRAY_SIZE(s5pv310_gpio_part1_4bit);

	for (i = 0; i < nr_chips; i++, chip++) {
		if (chip->config == NULL) {
			chip->config = &gpio_cfg;
			/* Assign the GPIO interrupt group */
			chip->group = group++;
			samsung_irq_gpio_add(chip);
		}
		if (chip->base == NULL)
			chip->base = S5PV310_BANK_BASE1(i);
	}

	samsung_gpiolib_add_4bit_chips(s5pv310_gpio_part1_4bit, nr_chips);

	chip = s5pv310_gpio_part2_4bit;
	nr_chips = ARRAY_SIZE(s5pv310_gpio_part2_4bit);

	for (i = 0; i < nr_chips; i++, chip++) {
		if (chip->config == NULL) {
			chip->config = &gpio_cfg;
			/* Assign the GPIO interrupt group */
			chip->group = group++;
			samsung_irq_gpio_add(chip);
		}
		if (chip->config == &gpio_cfg_extint) {
			/* Assign the External GPIO interrupt group */
			chip->group = extint++;
			chip->chip.to_irq = s5pc210_extint_to_irq;
		}
		if (chip->base == NULL)
				chip->base = S5PV310_BANK_BASE2(i);
	}

	samsung_gpiolib_add_4bit_chips(s5pv310_gpio_part2_4bit, nr_chips);

	chip = s5pv310_gpio_part3_4bit;
	nr_chips = ARRAY_SIZE(s5pv310_gpio_part3_4bit);

	for (i = 0; i < nr_chips; i++, chip++) {
		if (chip->config == NULL) {
			chip->config = &gpio_cfg;
			/* Assign the GPIO interrupt group */
			chip->group = group++;
			samsung_irq_gpio_add(chip);
		}
		if (chip->base == NULL)
				chip->base = S5PV310_BANK_BASE3(i);
	}

	samsung_gpiolib_add_4bit_chips(s5pv310_gpio_part3_4bit, nr_chips);

#ifdef CONFIG_SAMSUNG_IRQ_GPIO
	/* Register two GPIO IRQ */
	gpio[0].irq = IRQ_GPIO_XA;
	gpio[0].sig.start = 0;
	gpio[0].sig.nr_groups = IRQ_GPIO1_NR_GROUPS;
	gpio[0].sig.base = S5P_VA_GPIO;
	gpio[0].handler = NULL;
	gpio[1].irq = IRQ_GPIO_XB;
	gpio[1].sig.start = IRQ_GPIO1_NR_GROUPS;
	gpio[1].sig.nr_groups = IRQ_GPIO2_NR_GROUPS;
	gpio[1].sig.base = S5PV310_VA_GPIO2;
	gpio[1].handler = NULL;
#endif
	samsung_irq_gpio_register(gpio, ARRAY_SIZE(gpio));

	return 0;
}
core_initcall(s5pv310_gpiolib_init);
