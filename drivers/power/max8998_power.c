/*
 * Battery charger driver for Maxim MAX8998
 *
 * Copyright (C) 2009-2010 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/mfd/max8998.h>
#include <linux/mfd/max8998-private.h>
#include <linux/wakelock.h>

#define MAX8998_REG_STATUS2_BDET_MASK	(1 << 4)
#define MAX8998_REG_STATUS2_VCHGIN_MASK	(1 << 5)

struct max8998_power_info {
	struct max8998_dev	*chip;
	struct i2c_client	*i2c;

	struct power_supply	ac;
	struct power_supply	usb;
	struct power_supply	battery;
	int			irq_base;
	unsigned		ac_online:1;
	unsigned		usb_online:1;
	unsigned		bat_online:1;
	unsigned		chg_mode:2;
	unsigned		batt_detect:1;	/* detecing MB by ID pin */
	unsigned		topoff_threshold:2;
	unsigned		fast_charge:3;

	int (*set_charger) (int);

	struct wake_lock vbus_wake_lock;
};

static struct power_supply max8998_power_supplies[];

static int max8998_check_charger(struct max8998_power_info *info)
{
	int ret;
	u8 val;

	ret = max8998_read_reg(info->i2c, MAX8998_REG_STATUS2, &val);
	if (ret >= 0) {
		/*
		 * If battery detection is enabled, ID pin of battery is
		 * connected to MBDET pin of MAX8998. It could be used to
		 * detect battery presence.
		 * Otherwise, we have to assume that battery is always on.
		 */
		if (info->batt_detect)
			info->bat_online =
			    (val & MAX8998_REG_STATUS2_BDET_MASK) ? 0 : 1;
		else
			info->bat_online = 1;

		if (val & MAX8998_REG_STATUS2_VCHGIN_MASK) {
			info->ac_online = 1;
			info->usb_online = 1;
			wake_lock(&info->vbus_wake_lock);
		} else {
			info->ac_online = 0;
			info->usb_online = 0;
			wake_lock_timeout(&info->vbus_wake_lock, HZ / 2);
		}
	}

#if 0
	if (enable) {
		/* enable charger in platform */
		if (info->set_charger)
			info->set_charger(1);
		/* enable charger */
		max8998_set_bits(info->i2c, MAX8998_CHG_CNTL1, 1 << 7, 0);
	} else {
		/* disable charge */
		max8998_set_bits(info->i2c, MAX8998_CHG_CNTL1, 1 << 7, 1 << 7);
		if (info->set_charger)
			info->set_charger(0);
	}

	dev_dbg(chip->dev, "%s\n", (enable) ? "Enable charger"
		: "Disable charger");
#endif

	power_supply_changed(&max8998_power_supplies[0]);
	power_supply_changed(&max8998_power_supplies[1]);
	power_supply_changed(&max8998_power_supplies[2]);

	return 0;
}

static irqreturn_t max8998_charger_handler(int irq, void *data)
{
	struct max8998_power_info *info = (struct max8998_power_info *)data;
	struct max8998_dev *chip = info->chip;

	switch (irq - chip->irq_base) {
	case MAX8998_IRQ_DCINR:
	case MAX8998_IRQ_JIGR:
		dev_dbg(chip->dev, "USB or TA is inserted\n");
		break;
	case MAX8998_IRQ_DCINF:
	case MAX8998_IRQ_JIGF:
		dev_dbg(chip->dev, "USB or TA is removed\n");
		break;
	}

	max8998_check_charger(info);

	return IRQ_HANDLED;
}

static int max8998_ac_get_prop(struct power_supply *psy,
			       enum power_supply_property psp,
			       union power_supply_propval *val)
{
	struct max8998_power_info *info = dev_get_drvdata(psy->dev->parent);
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = info->ac_online;
		break;
	default:
		ret = -ENODEV;
		break;
	}

	return ret;
}

static enum power_supply_property max8998_ac_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static int max8998_usb_get_prop(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct max8998_power_info *info = dev_get_drvdata(psy->dev->parent);
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = info->usb_online;
		break;
	default:
		ret = -ENODEV;
		break;
	}

	return ret;
}

static enum power_supply_property max8998_usb_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};


static int max8998_bat_get_prop(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct max8998_power_info *info = dev_get_drvdata(psy->dev->parent);
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = info->bat_online;
		break;
	case POWER_SUPPLY_PROP_STATUS:
		if (info->usb_online || info->ac_online)
			val->intval = POWER_SUPPLY_STATUS_CHARGING;
		else
			val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
		break;
	default:
		ret = -ENODEV;
		break;
	}
	return ret;
}

static enum power_supply_property max8998_battery_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_STATUS,
};

#define REQUEST_IRQ(_irq, _name)					\
do {									\
	ret = request_threaded_irq(chip->irq_base + _irq, NULL,		\
				    max8998_charger_handler,		\
				    IRQF_ONESHOT, _name, info);		\
	if (ret)							\
		dev_err(chip->dev, "Failed to request IRQ #%d: %d\n",	\
			_irq, ret);					\
} while (0)

static struct power_supply max8998_power_supplies[] = {
	{
		.name = "max8998-ac",
		.type = POWER_SUPPLY_TYPE_MAINS,
		.properties = max8998_ac_props,
		.num_properties = ARRAY_SIZE(max8998_ac_props),
		.get_property = max8998_ac_get_prop,
	},
	{
		.name = "max8998-usb",
		.type = POWER_SUPPLY_TYPE_USB,
		.properties = max8998_usb_props,
		.num_properties = ARRAY_SIZE(max8998_usb_props),
		.get_property = max8998_usb_get_prop,
	},
	{
		.name = "max8998-battery",
		.type = POWER_SUPPLY_TYPE_BATTERY,
		.properties = max8998_battery_props,
		.num_properties = ARRAY_SIZE(max8998_battery_props),
		.get_property = max8998_bat_get_prop,
	},
};

static __devinit int max8998_power_probe(struct platform_device *pdev)
{
	struct max8998_dev *chip = dev_get_drvdata(pdev->dev.parent);
	struct max8998_platform_data *max8998_pdata;
	struct max8998_power_data *pdata = NULL;
	struct max8998_power_info *info;
	int ret, i;

	if (pdev->dev.parent->platform_data) {
		max8998_pdata = pdev->dev.parent->platform_data;
		pdata = max8998_pdata->power;
	}

	if (!pdata) {
		dev_err(&pdev->dev, "platform data isn't assigned to "
			"power supply\n");
		return -EINVAL;
	}

	info = kzalloc(sizeof(struct max8998_power_info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;
	info->chip = chip;
	info->i2c = chip->i2c;

	info->batt_detect = pdata->batt_detect;
	info->topoff_threshold = pdata->topoff_threshold;
	info->fast_charge = pdata->fast_charge;
	info->set_charger = pdata->set_charger;
	info->ac_online = 0;
	info->usb_online = 0;
	info->bat_online = 0;
	wake_lock_init(&info->vbus_wake_lock, WAKE_LOCK_SUSPEND, "vbus_present");

	dev_set_drvdata(&pdev->dev, info);

	platform_set_drvdata(pdev, info);

	/* init power supplier framework */
	for (i = 0; i < ARRAY_SIZE(max8998_power_supplies); i++) {
		ret = power_supply_register(&pdev->dev,
				&max8998_power_supplies[i]);
		if (ret) {
			dev_err(&pdev->dev, "Failed to register"
					"power supply %d,%d\n", i, ret);
			goto __end__;
		}
		max8998_power_supplies[i].dev->parent = &pdev->dev;
	}

	max8998_check_charger(info);

	REQUEST_IRQ(MAX8998_IRQ_DCINF, "DCIN falling");
	REQUEST_IRQ(MAX8998_IRQ_DCINR, "DCIN rising");
	REQUEST_IRQ(MAX8998_IRQ_JIGF, "JIG falling");
	REQUEST_IRQ(MAX8998_IRQ_JIGR, "JIG rising");

__end__:
	return ret;
}

static __devexit int max8998_power_remove(struct platform_device *pdev)
{
	int i;
	struct max8998_power_info *info = platform_get_drvdata(pdev);

	free_irq(MAX8998_IRQ_DCINF, info);
	free_irq(MAX8998_IRQ_DCINR, info);
	free_irq(MAX8998_IRQ_JIGF, info);
	free_irq(MAX8998_IRQ_JIGR, info);

	for (i = 0; i < ARRAY_SIZE(max8998_power_supplies); i++) {
		power_supply_unregister(&max8998_power_supplies[i]);
	}

	if (info) {
		kfree(info);
	}

	return 0;
}

static struct platform_driver max8998_power_driver = {
	.probe	= max8998_power_probe,
	.remove	= __devexit_p(max8998_power_remove),
	.driver	= {
		.name	= "max8998-power",
	},
};

static int __init max8998_power_init(void)
{
	return platform_driver_register(&max8998_power_driver);
}
module_init(max8998_power_init);

static void __exit max8998_power_exit(void)
{
	platform_driver_unregister(&max8998_power_driver);
}
module_exit(max8998_power_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Battery charger driver for MAX8998");
MODULE_ALIAS("platform:max8998-power");
