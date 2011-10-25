/*
 * s5m8751-core.c  --  S5M8751 Advanced PMIC with AUDIO DAC
 *
 * Copyright 2009 Samsung Electronics.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/irq.h>
#include <linux/mfd/core.h>
#include <linux/slab.h>

#include <mach/gpio.h>

#include <linux/mfd/s5m8751/core.h>
#include <linux/mfd/s5m8751/pdata.h>

#define SLEEPB_ENABLE		1
#define SLEEPB_DISABLE		0

static DEFINE_MUTEX(io_mutex);

int s5m8751_clear_bits(struct s5m8751 *s5m8751, uint8_t reg, uint8_t mask)
{
	uint8_t reg_val;
	int ret = 0;

	ret = s5m8751_reg_read(s5m8751, reg, &reg_val);
	if (ret)
		return ret;

	reg_val &= ~mask;
	ret = s5m8751_reg_write(s5m8751, reg, reg_val);

	return ret;
}
EXPORT_SYMBOL_GPL(s5m8751_clear_bits);

int s5m8751_set_bits(struct s5m8751 *s5m8751, uint8_t reg, uint8_t mask)
{
	uint8_t reg_val;
	int ret = 0;

	ret = s5m8751_reg_read(s5m8751, reg, &reg_val);
	if (ret)
		return ret;

	reg_val |= mask;
	ret = s5m8751_reg_write(s5m8751, reg, reg_val);

	return ret;
}
EXPORT_SYMBOL_GPL(s5m8751_set_bits);

int s5m8751_reg_read(struct s5m8751 *s5m8751, uint8_t reg, uint8_t *val)
{
	int ret = 0;

	mutex_lock(&io_mutex);
	ret = s5m8751->read_dev(s5m8751, reg, val);
	if (ret < 0)
		dev_err(s5m8751->dev, "failed reading from 0x%02x\n", reg);

	mutex_unlock(&io_mutex);
	return ret;
}
EXPORT_SYMBOL_GPL(s5m8751_reg_read);

int s5m8751_reg_write(struct s5m8751 *s5m8751, uint8_t reg, uint8_t val)
{
	int ret = 0;

	mutex_lock(&io_mutex);
	ret = s5m8751->write_dev(s5m8751, reg, val);
	if (ret < 0)
		dev_err(s5m8751->dev, "failed writing 0x%02x\n", reg);

	mutex_unlock(&io_mutex);
	return ret;
}
EXPORT_SYMBOL_GPL(s5m8751_reg_write);

int s5m8751_block_read(struct s5m8751 *s5m8751, uint8_t reg, int len,
							uint8_t *val)
{
	int ret = 0;

	mutex_lock(&io_mutex);
	ret = s5m8751->read_block_dev(s5m8751, reg, len, val);
	if (ret < 0)
		dev_err(s5m8751->dev, "failed reading from 0x%02x\n", reg);

	mutex_unlock(&io_mutex);
	return ret;
}
EXPORT_SYMBOL_GPL(s5m8751_block_read);

int s5m8751_block_write(struct s5m8751 *s5m8751, uint8_t reg, int len,
							uint8_t *val)
{
	int ret = 0;

	mutex_lock(&io_mutex);
	ret = s5m8751->write_block_dev(s5m8751, reg, len, val);
	if (ret < 0)
		dev_err(s5m8751->dev, "failed writings to 0x%02x\n", reg);

	mutex_unlock(&io_mutex);
	return ret;
}
EXPORT_SYMBOL_GPL(s5m8751_block_write);

static struct resource s5m8751_ldo1_resource[] = {
	{
		.start = S5M8751_LDO1_VSET,
		.end = S5M8751_LDO1_VSET,
		.flags = IORESOURCE_IO,
	},
};

static struct resource s5m8751_ldo2_resource[] = {
	{
		.start = S5M8751_LDO2_VSET,
		.end = S5M8751_LDO2_VSET,
		.flags = IORESOURCE_IO,
	},
};

static struct resource s5m8751_ldo3_resource[] = {
	{
		.start = S5M8751_LDO3_VSET,
		.end = S5M8751_LDO3_VSET,
		.flags = IORESOURCE_IO,
	},
};

static struct resource s5m8751_ldo4_resource[] = {
	{
		.start = S5M8751_LDO4_VSET,
		.end = S5M8751_LDO4_VSET,
		.flags = IORESOURCE_IO,
	},
};

static struct resource s5m8751_ldo5_resource[] = {
	{
		.start = S5M8751_LDO5_VSET,
		.end = S5M8751_LDO5_VSET,
		.flags = IORESOURCE_IO,
	},
};

static struct resource s5m8751_ldo6_resource[] = {
	{
		.start = S5M8751_LDO6_VSET,
		.end = S5M8751_LDO6_VSET,
		.flags = IORESOURCE_IO,
	},
};

static struct resource s5m8751_buck1_1_resource[] = {
	{
		.start = S5M8751_BUCK1_V1_SET,
		.end = S5M8751_BUCK1_V2_SET,
		.flags = IORESOURCE_IO,
	},
};

static struct resource s5m8751_buck1_2_resource[] = {
	{
		.start = S5M8751_BUCK1_V1_SET,
		.end = S5M8751_BUCK1_V2_SET,
		.flags = IORESOURCE_IO,
	},
};

static struct resource s5m8751_buck2_1_resource[] = {
	{
		.start = S5M8751_BUCK2_V1_SET,
		.end = S5M8751_BUCK2_V2_SET,
		.flags = IORESOURCE_IO,
	},
};

static struct resource s5m8751_buck2_2_resource[] = {
	{
		.start = S5M8751_BUCK2_V1_SET,
		.end = S5M8751_BUCK2_V2_SET,
		.flags = IORESOURCE_IO,
	},
};

static struct mfd_cell regulator_devs[] = {
	{
		.name = "s5m8751-regulator",
		.id = 0,
		.num_resources = ARRAY_SIZE(s5m8751_ldo1_resource),
		.resources = s5m8751_ldo1_resource,
	},
	{
		.name = "s5m8751-regulator",
		.id = 1,
		.num_resources = ARRAY_SIZE(s5m8751_ldo2_resource),
		.resources = s5m8751_ldo2_resource,
	},
	{
		.name = "s5m8751-regulator",
		.id = 2,
		.num_resources = ARRAY_SIZE(s5m8751_ldo3_resource),
		.resources = s5m8751_ldo3_resource,
	},
	{
		.name = "s5m8751-regulator",
		.id = 3,
		.num_resources = ARRAY_SIZE(s5m8751_ldo4_resource),
		.resources = s5m8751_ldo4_resource,
	},
	{
		.name = "s5m8751-regulator",
		.id = 4,
		.num_resources = ARRAY_SIZE(s5m8751_ldo5_resource),
		.resources = s5m8751_ldo5_resource,
	},
	{
		.name = "s5m8751-regulator",
		.id = 5,
		.num_resources = ARRAY_SIZE(s5m8751_ldo6_resource),
		.resources = s5m8751_ldo6_resource,
	},
	{
		.name = "s5m8751-regulator",
		.id = 6,
		.num_resources = ARRAY_SIZE(s5m8751_buck1_1_resource),
		.resources = s5m8751_buck1_1_resource,
	},
	{
		.name = "s5m8751-regulator",
		.id = 7,
		.num_resources = ARRAY_SIZE(s5m8751_buck1_2_resource),
		.resources = s5m8751_buck1_2_resource,
	},
	{
		.name = "s5m8751-regulator",
		.id = 8,
		.num_resources = ARRAY_SIZE(s5m8751_buck2_1_resource),
		.resources = s5m8751_buck2_1_resource,
	},
	{
		.name = "s5m8751-regulator",
		.id = 9,
		.num_resources = ARRAY_SIZE(s5m8751_buck2_2_resource),
		.resources = s5m8751_buck2_2_resource,
	},
};

static struct resource power_supply_resources[] = {
	{
		.name = "s5m8751-power",
		.start = S5M8751_IRQB_EVENT2,
		.end = S5M8751_IRQB_MASK2,
		.flags = IORESOURCE_IO,
	},
};

static struct mfd_cell backlight_devs[] = {
	{
		.name = "s5m8751-backlight",
	},
};

static struct mfd_cell power_devs[] = {
	{
		.name = "s5m8751-power",
		.num_resources = 1,
		.resources = &power_supply_resources[0],
		.id = -1,
	},
};

static struct mfd_cell audio_devs[] = {
	{
		.name = "s5m8751-codec",
		.id = -1,
	}
};

struct s5m8751_irq_data {
	int reg;
	int mask_reg;
	int enable;
	int offs;
	int flags;
};

static struct s5m8751_irq_data s5m8751_irqs[] = {
	[S5M8751_IRQ_PWRKEY1B] = {
		.reg		= S5M8751_IRQB_EVENT1,
		.mask_reg	= S5M8751_IRQB_MASK1,
		.offs		= 1 << 3,
	},
	[S5M8751_IRQ_PWRKEY2B] = {
		.reg		= S5M8751_IRQB_EVENT1,
		.mask_reg	= S5M8751_IRQB_MASK1,
		.offs		= 1 << 2,
	},
	[S5M8751_IRQ_PWRKEY3] = {
		.reg		= S5M8751_IRQB_EVENT1,
		.mask_reg	= S5M8751_IRQB_MASK1,
		.offs		= 1 << 1,
	},
	[S5M8751_IRQ_VCHG_DETECTION] = {
		.reg		= S5M8751_IRQB_EVENT2,
		.mask_reg	= S5M8751_IRQB_MASK2,
		.offs		= 1 << 4,
	},
	[S5M8751_IRQ_VCHG_REMOVAL] = {
		.reg		= S5M8751_IRQB_EVENT2,
		.mask_reg	= S5M8751_IRQB_MASK2,
		.offs		= 1 << 3,
	},
	[S5M8751_IRQ_VCHG_TIMEOUT] = {
		.reg		= S5M8751_IRQB_EVENT2,
		.mask_reg	= S5M8751_IRQB_MASK2,
		.offs		= 1 << 2,
	},
};

static inline struct s5m8751_irq_data *irq_to_s5m8751(struct s5m8751 *s5m8751,
							int irq)
{
	return &s5m8751_irqs[irq - s5m8751->irq_base];
}

static irqreturn_t s5m8751_irq(int irq, void *data)
{
	struct s5m8751 *s5m8751 = data;
	struct s5m8751_irq_data *irq_data;

	int read_reg = -1;
	uint8_t value;
	int i;

	for (i = 0; i < ARRAY_SIZE(s5m8751_irqs); i++) {
		irq_data = &s5m8751_irqs[i];
		if (read_reg != irq_data->reg) {
			read_reg = irq_data->reg;
			s5m8751_reg_read(s5m8751, irq_data->reg, &value);
		}
		if (value & irq_data->enable)
			handle_nested_irq(s5m8751->irq_base + i);
	}

	s5m8751_reg_write(s5m8751, S5M8751_IRQB_EVENT1, 0x00);
	s5m8751_reg_write(s5m8751, S5M8751_IRQB_EVENT2, 0x00);

	return IRQ_HANDLED;
}

static void s5m8751_irq_lock(unsigned int irq)
{
	struct s5m8751 *s5m8751 = get_irq_chip_data(irq);
	mutex_lock(&s5m8751->irq_lock);
}

static void s5m8751_irq_sync_unlock(unsigned int irq)
{
	struct s5m8751 *s5m8751 = get_irq_chip_data(irq);
	struct s5m8751_irq_data *irq_data;
	static uint8_t cache_pwr = 0xFF;
	static uint8_t cache_chg = 0xFF;
	static uint8_t irq_pwr, irq_chg;
	int i;

	irq_pwr = cache_pwr;
	irq_chg = cache_chg;
	for (i = 0; i < ARRAY_SIZE(s5m8751_irqs); i++) {
		irq_data = &s5m8751_irqs[i];
		switch (irq_data->mask_reg) {
		case S5M8751_IRQB_MASK1:
			irq_pwr &= ~irq_data->enable;
			break;
		case S5M8751_IRQB_MASK2:
			irq_chg &= ~irq_data->enable;
			break;
		default:
			dev_err(s5m8751->dev, "Unknown IRQ\n");
			break;
		}
	}

	if (cache_pwr != irq_pwr) {
		cache_pwr = irq_pwr;
		s5m8751_reg_write(s5m8751, S5M8751_IRQB_MASK1, irq_pwr);
	}

	if (cache_chg != irq_chg) {
		cache_chg = irq_chg;
		s5m8751_reg_write(s5m8751, S5M8751_IRQB_MASK2, irq_chg);
	}

	mutex_unlock(&s5m8751->irq_lock);
}

static void s5m8751_irq_enable(unsigned int irq)
{
	struct s5m8751 *s5m8751 = get_irq_chip_data(irq);
	s5m8751_irqs[irq - s5m8751->irq_base].enable =
		s5m8751_irqs[irq - s5m8751->irq_base].offs;
}

static void s5m8751_irq_disable(unsigned int irq)
{
	struct s5m8751 *s5m8751 = get_irq_chip_data(irq);
	s5m8751_irqs[irq - s5m8751->irq_base].enable = 0;
}

static struct irq_chip s5m8751_irq_chip = {
	.name		= "s5m8751",
	.bus_lock	= s5m8751_irq_lock,
	.bus_sync_unlock = s5m8751_irq_sync_unlock,
	.enable		= s5m8751_irq_enable,
	.disable	= s5m8751_irq_disable,
};

static int s5m8751_irq_init(struct s5m8751 *s5m8751, int irq,
				struct s5m8751_pdata *pdata)
{
	unsigned long flags = IRQF_TRIGGER_FALLING | IRQF_ONESHOT;
	struct irq_desc *desc;
	int i, ret, __irq;

	if (!pdata || !pdata->irq_base) {
		dev_warn(s5m8751->dev, "No interrupt support on IRQ base\n");
		return -EINVAL;
	}

	s5m8751_reg_write(s5m8751, S5M8751_IRQB_EVENT1, 0x00);
	s5m8751_reg_write(s5m8751, S5M8751_IRQB_EVENT2, 0x00);

	s5m8751_reg_write(s5m8751, S5M8751_IRQB_MASK1, 0xFF);
	s5m8751_reg_write(s5m8751, S5M8751_IRQB_MASK2, 0xFF);

	mutex_init(&s5m8751->irq_lock);
	s5m8751->core_irq = irq;
	s5m8751->irq_base = pdata->irq_base;
	desc = irq_to_desc(s5m8751->core_irq);

	for (i = 0; i < ARRAY_SIZE(s5m8751_irqs); i++) {
		__irq = i + s5m8751->irq_base;
		set_irq_chip_data(__irq, s5m8751);
		set_irq_chip_and_handler(__irq, &s5m8751_irq_chip,
					 handle_edge_irq);
		set_irq_nested_thread(__irq, 1);
#ifdef CONFIG_ARM
		set_irq_flags(__irq, IRQF_VALID);
#else
		set_irq_noprobe(__irq);
#endif
	}
	if (!irq) {
		dev_warn(s5m8751->dev, "No interrupt support on core IRQ\n");
		return -EINVAL;
	}

	ret = request_threaded_irq(irq, NULL, s5m8751_irq, flags,
				"s5m8751", s5m8751);

	if (ret) {
		dev_err(s5m8751->dev, "Failed to request core IRQ: %d\n", ret);
		s5m8751->core_irq = 0;
	}

	return 0;
}

static void s5m8751_irq_exit(struct s5m8751 *s5m8751)
{
	if (s5m8751->core_irq)
		free_irq(s5m8751->core_irq, s5m8751);
}

static ssize_t s5m8751_store_pshold(struct device *dev,
	       struct device_attribute *attr, const char *buf, size_t count)
{
	int rc;
	unsigned long value;

	struct s5m8751 *s5m8751 = dev_get_drvdata(dev);
	struct s5m8751_pdata *s5m8751_pdata = s5m8751->dev->platform_data;

	rc = strict_strtoul(buf, 0, &value);
	if (rc)
		return rc;

	rc = -ENXIO;

	if (!value) {
		gpio_set_value(s5m8751_pdata->gpio_pshold, 0);
		rc = count;
	} else
		rc = -EINVAL;

	return rc;
}

static ssize_t s5m8751_show_pshold(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct s5m8751 *s5m8751 = dev_get_drvdata(dev);
	struct s5m8751_pdata *s5m8751_pdata = s5m8751->dev->platform_data;
	int value;

	value = gpio_get_value(s5m8751_pdata->gpio_pshold);

	return sprintf(buf, "%d\n", value);
}

static DEVICE_ATTR(pshold, 0644, s5m8751_show_pshold, s5m8751_store_pshold);

static int s5m8751_sleepb_set(struct s5m8751 *s5m8751, int status)
{
	if (status)
		s5m8751_set_bits(s5m8751, S5M8751_ONOFF1,
					S5M8751_SLEEPB_PIN_ENABLE);
	else
		s5m8751_clear_bits(s5m8751, S5M8751_ONOFF1,
					S5M8751_SLEEPB_PIN_ENABLE);
	return 0;
}

int s5m8751_device_init(struct s5m8751 *s5m8751, int irq,
						struct s5m8751_pdata *pdata)
{
	int ret = 0;

	if (irq)
		ret = s5m8751_irq_init(s5m8751, irq, pdata);
	if (ret < 0)
		goto err;

	dev_set_drvdata(s5m8751->dev, s5m8751);
	s5m8751_sleepb_set(s5m8751, S5M8751_SLEEPB_ENABLE);

	if (pdata->gpio_pshold)
		ret = device_create_file(s5m8751->dev, &dev_attr_pshold);

	if (ret < 0) {
		dev_err(s5m8751->dev,
				"Failed to add s5m8751 sysfs: %d\n", ret);
		goto err;
	}

	if (pdata && pdata->regulator) {
		ret = mfd_add_devices(s5m8751->dev, -1, regulator_devs,
					ARRAY_SIZE(regulator_devs), NULL,
					0);
		if (ret < 0)
			dev_err(s5m8751->dev,
				"Failed to add regulator driver: %d\n", ret);
	}

	if (pdata && pdata->backlight) {
		ret = mfd_add_devices(s5m8751->dev, -1, backlight_devs,
					ARRAY_SIZE(backlight_devs), NULL,
					0);
		if (ret < 0)
			dev_err(s5m8751->dev,
				"Failed to add backlight driver: %d\n",	ret);
	}

	if (pdata && pdata->power) {
		ret = mfd_add_devices(s5m8751->dev, -1, power_devs,
					ARRAY_SIZE(power_devs), NULL,
					0);
		if (ret < 0)
			dev_err(s5m8751->dev,
				"Failed to add power driver: %d\n", ret);
	}

	if (pdata && pdata->audio) {
		ret = mfd_add_devices(s5m8751->dev, -1, audio_devs,
					ARRAY_SIZE(audio_devs), NULL,
					0);
		if (ret < 0)
			dev_err(s5m8751->dev,
				"Failed to add audio driver: %d\n", ret);
	}

	return 0;

err:
	return ret;
}
EXPORT_SYMBOL_GPL(s5m8751_device_init);

void s5m8751_device_exit(struct s5m8751 *s5m8751)
{
	dev_info(s5m8751->dev, "Unloaded S5M8751 device driver\n");

	mfd_remove_devices(s5m8751->dev);
	if (s5m8751->irq_base)
		s5m8751_irq_exit(s5m8751);

	kfree(s5m8751);
}
EXPORT_SYMBOL_GPL(s5m8751_device_exit);

/* Module Information */
MODULE_DESCRIPTION("S5M8751 Multi-function IC");
MODULE_AUTHOR("Jongbin, Won <jongbin.won@samsung.com>");
MODULE_LICENSE("GPL");
