/*
 * s5m8751-battery.c  --  S5M8751 Power-Audio IC ALSA Soc Audio driver
 *
 * Copyright 2009 Samsung Electronics.
 *
 * Author: Jongbin Won <jongbin.won@samsung.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/module.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/slab.h>
#include <asm/irq.h>
#include <mach/gpio.h>

#include <linux/mfd/s5m8751/core.h>
#include <linux/mfd/s5m8751/power.h>
#include <linux/mfd/s5m8751/pdata.h>

struct s5m8751_power {
	struct s5m8751 *s5m8751;
	struct power_supply wall;
	struct power_supply usb;
	struct power_supply battery;
	unsigned int gpio_hadp_lusb;
};

static int s5m8751_power_check_online(struct s5m8751 *s5m8751, int supply,
					union power_supply_propval *val)
{
	struct s5m8751_pdata *s5m8751_pdata = s5m8751->dev->platform_data;
	struct s5m8751_power_pdata *pdata = s5m8751_pdata->power;
	unsigned int gpio = pdata->gpio_hadp_lusb;
	int value;

	value = gpio_get_value(gpio);

	if (!(value ^ supply))
		val->intval = 1;
	else
		val->intval = 0;

	return 0;
}

static int s5m8751_wall_get_prop(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct s5m8751_power *s5m8751_power =
					dev_get_drvdata(psy->dev->parent);
	struct s5m8751 *s5m8751 = s5m8751_power->s5m8751;
	int ret;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		ret = s5m8751_power_check_online(s5m8751, S5M8751_PWR_WALL,
									val);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static enum power_supply_property s5m8751_wall_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static int s5m8751_usb_get_prop(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct s5m8751_power *s5m8751_power =
					dev_get_drvdata(psy->dev->parent);
	struct s5m8751 *s5m8751 = s5m8751_power->s5m8751;
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		ret = s5m8751_power_check_online(s5m8751, S5M8751_PWR_USB,
									val);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static enum power_supply_property s5m8751_usb_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

int s5m8751_config_battery(struct s5m8751 *s5m8751)
{
	struct s5m8751_pdata *s5m8751_pdata = s5m8751->dev->platform_data;
	struct s5m8751_power_pdata *pdata;
	uint8_t reg_val, constant_voltage, fast_chg_current;

	if (!s5m8751_pdata || !s5m8751_pdata->power) {
		dev_warn(s5m8751->dev,
			"No charger platform data, charger not configured.\n");
		return -EINVAL;
	}
	pdata = s5m8751_pdata->power;

	if ((pdata->constant_voltage < 4100) ||
					 (pdata->constant_voltage > 4275)) {
		dev_err(s5m8751->dev, "Constant voltage is out of range!\n");
		return -EINVAL;
	}
	constant_voltage = S5M8751_CHG_CONT_VOLTAGE(pdata->constant_voltage);
	s5m8751_reg_read(s5m8751, S5M8751_CHG_IV_SET, &reg_val);
	reg_val &= ~S5M8751_CONSTANT_VOLTAGE_MASK;
	s5m8751_reg_write(s5m8751, S5M8751_CHG_IV_SET, reg_val
							 | constant_voltage);

	if ((pdata->fast_chg_current < 50) ||
					(pdata->fast_chg_current > 800)) {
		dev_err(s5m8751->dev,
				"Fast charging current is out of range!\n");
		return -EINVAL;
	}
	fast_chg_current = S5M8751_FAST_CHG_CURRENT(pdata->fast_chg_current);
	s5m8751_reg_read(s5m8751, S5M8751_CHG_IV_SET, &reg_val);
	reg_val &= ~S5M8751_FAST_CHG_CURRENT_MASK;
	s5m8751_reg_write(s5m8751, S5M8751_CHG_IV_SET, reg_val
							 | fast_chg_current);
	return 0;
}

static int s5m8751_bat_check_status(struct s5m8751 *s5m8751, int *status)
{
	int ret;
	uint8_t val;

	ret = s5m8751_reg_read(s5m8751, S5M8751_STATUS, &val);
	if (ret < 0)
		return ret;

	switch (val & S5M8751_CHG_STATUS_MASK) {
	case S5M8751_CHG_STATE_OFF:
		*status = POWER_SUPPLY_STATUS_NOT_CHARGING;
		break;
	case S5M8751_CHG_STATE_ON:
		*status = POWER_SUPPLY_STATUS_CHARGING;
		break;
	default:
		*status = POWER_SUPPLY_STATUS_UNKNOWN;
	}

	return 0;
}

static int s5m8751_bat_check_type(struct s5m8751 *s5m8751, int *type)
{
	int ret;
	uint8_t val;

	ret = s5m8751_reg_read(s5m8751, S5M8751_STATUS, &val);
	if (ret < 0)
		return ret;

	if (!(val & S5M8751_CHG_STATUS_MASK)) {
		*type = POWER_SUPPLY_CHARGE_TYPE_NONE;
		return 0;
	}

	ret = s5m8751_reg_read(s5m8751, S5M8751_CHG_TEST_R, &val);
	if (ret < 0)
		return ret;

	switch (val & S5M8751_CHG_TYPE_MASK) {
	case S5M8751_CHG_STATE_TRICKLE:
		*type = POWER_SUPPLY_CHARGE_TYPE_TRICKLE;
		break;
	case S5M8751_CHG_STATE_FAST:
		*type = POWER_SUPPLY_CHARGE_TYPE_FAST;
		break;
	}

	return 0;
}

static int s5m8751_bat_get_prop(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct s5m8751_power *s5m8751_power =
					dev_get_drvdata(psy->dev->parent);
	struct s5m8751 *s5m8751 = s5m8751_power->s5m8751;
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		ret = s5m8751_bat_check_status(s5m8751, &val->intval);
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		ret = s5m8751_bat_check_type(s5m8751, &val->intval);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static enum power_supply_property s5m8751_bat_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_CHARGE_TYPE,
};

static irqreturn_t s5m8751_charger_handler(int irq, void *data)
{
	struct s5m8751_power *s5m8751_power = data;
	struct s5m8751 *s5m8751 = s5m8751_power->s5m8751;

	switch (irq - s5m8751->irq_base) {
	case S5M8751_IRQ_VCHG_DETECTION:
		dev_info(s5m8751->dev, "Charger source inserted\n");
		break;
	case S5M8751_IRQ_VCHG_REMOVAL:
		dev_info(s5m8751->dev, "Charger source removal\n");
		break;
	case S5M8751_IRQ_VCHG_TIMEOUT:
		dev_info(s5m8751->dev, "Charger timeout\n");
		break;
	default:
		dev_info(s5m8751->dev, "Unknown IRQ source\n");
		break;
	}

	s5m8751_reg_write(s5m8751, S5M8751_IRQB_EVENT1, 0x00);
	s5m8751_reg_write(s5m8751, S5M8751_IRQB_EVENT2, 0x00);

	return IRQ_HANDLED;
}

#define REQUEST_IRQ(_irq, _name)					\
do {									\
	ret = request_threaded_irq(s5m8751->irq_base + _irq, NULL,	\
				    s5m8751_charger_handler,		\
				    IRQF_ONESHOT, _name, power);	\
	if (ret)							\
		dev_err(s5m8751->dev, "Fail to request IRQ #%d: %d\n",	\
			_irq, ret);					\
} while (0)

static __devinit int s5m8751_init_charger(struct s5m8751 *s5m8751,
						struct s5m8751_power *power)
{
	int ret;

	REQUEST_IRQ(S5M8751_IRQ_VCHG_DETECTION, "chg detection");
	REQUEST_IRQ(S5M8751_IRQ_VCHG_REMOVAL, "chg removal");
	REQUEST_IRQ(S5M8751_IRQ_VCHG_TIMEOUT, "chg timeout");

	return 0;
};

static __devexit int s5m8751_exit_charger(struct s5m8751_power *power)
{
	struct s5m8751 *s5m8751 = power->s5m8751;
	int irq;

	irq = s5m8751->irq_base + S5M8751_IRQ_VCHG_DETECTION;

	for (; irq <= s5m8751->irq_base + S5M8751_IRQ_VCHG_TIMEOUT; irq++)
		free_irq(irq, power);

	return 0;
}

static __devinit int s5m8751_power_probe(struct platform_device *pdev)
{
	struct s5m8751 *s5m8751 = dev_get_drvdata(pdev->dev.parent);
	struct s5m8751_pdata *s5m8751_pdata = s5m8751->dev->platform_data;
	struct s5m8751_power_pdata *pdata = s5m8751_pdata->power;
	struct s5m8751_power *power;
	struct power_supply *usb;
	struct power_supply *battery;
	struct power_supply *wall;
	int ret;

	power = kzalloc(sizeof(struct s5m8751_power), GFP_KERNEL);
	if (power == NULL)
		return -ENOMEM;

	power->s5m8751 = s5m8751;
	power->gpio_hadp_lusb = pdata->gpio_hadp_lusb;
	platform_set_drvdata(pdev, power);

	usb = &power->usb;
	battery = &power->battery;
	wall = &power->wall;

	s5m8751_config_battery(s5m8751);

	wall->name = "s5m8751-wall";
	wall->type = POWER_SUPPLY_TYPE_MAINS;
	wall->properties = s5m8751_wall_props;
	wall->num_properties = ARRAY_SIZE(s5m8751_wall_props);
	wall->get_property = s5m8751_wall_get_prop;
	ret = power_supply_register(&pdev->dev, wall);
	if (ret)
		goto err_kmalloc;

	battery->name = "s5m8751-battery";
	battery->properties = s5m8751_bat_props;
	battery->num_properties = ARRAY_SIZE(s5m8751_bat_props);
	battery->get_property = s5m8751_bat_get_prop;
	ret = power_supply_register(&pdev->dev, battery);
	if (ret)
		goto err_wall;

	usb->name = "s5m8751-usb",
	usb->type = POWER_SUPPLY_TYPE_USB;
	usb->properties = s5m8751_usb_props;
	usb->num_properties = ARRAY_SIZE(s5m8751_usb_props);
	usb->get_property = s5m8751_usb_get_prop;
	ret = power_supply_register(&pdev->dev, usb);
	if (ret)
		goto err_battery;

	s5m8751_init_charger(s5m8751, power);
	return ret;

err_battery:
	power_supply_unregister(battery);
err_wall:
	power_supply_unregister(wall);
err_kmalloc:
	kfree(power);
	return ret;
}

static __devexit int s5m8751_power_remove(struct platform_device *pdev)
{
	struct s5m8751_power *s5m8751_power = platform_get_drvdata(pdev);

	power_supply_unregister(&s5m8751_power->battery);
	power_supply_unregister(&s5m8751_power->wall);
	power_supply_unregister(&s5m8751_power->usb);
	s5m8751_exit_charger(s5m8751_power);
	return 0;
}

static struct platform_driver s5m8751_power_driver = {
	.driver	= {
		.name	= "s5m8751-power",
		.owner	= THIS_MODULE,
	},
	.probe = s5m8751_power_probe,
	.remove = __devexit_p(s5m8751_power_remove),
};

static int __init s5m8751_power_init(void)
{
	return platform_driver_register(&s5m8751_power_driver);
}
module_init(s5m8751_power_init);

static void __exit s5m8751_power_exit(void)
{
	platform_driver_unregister(&s5m8751_power_driver);
}
module_exit(s5m8751_power_exit);

MODULE_DESCRIPTION("Power supply driver for S5M8751");
MODULE_AUTHOR("Jongbin,Won <jognbin.won@samsung.com>");
MODULE_LICENSE("GPL");
