/* linux/driver/regulator/samsung_pd.c
 *
 * Based on linux/driver/regulator/fixed.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Samsung power domain support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/err.h>
#include <linux/io.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/samsung_pd.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/clk.h>

struct samsung_pd_data {
	struct regulator_desc desc;
	struct regulator_dev *dev;
	int microvolts;
	unsigned startup_delay;
	bool is_enabled;
	struct clk_should_be_running *clk_run;
	void (*enable)(void);
	void (*disable)(void);
};


static int samsung_pd_clk_enable(struct clk_should_be_running *clk_run)
{
	struct clk *clkp;
	struct platform_device tmp_dev;
	int i;

	tmp_dev.dev.bus = &platform_bus_type;

	for (i = 0;; i++) {
		if (clk_run[i].clk_name == NULL)
			break;

		tmp_dev.id = clk_run[i].id;
		clkp = clk_get(&tmp_dev.dev, clk_run[i].clk_name);

		if (IS_ERR(clkp)) {
			printk(KERN_ERR "unable to get clock %s\n", clk_run[i].clk_name);
		} else {
			clk_enable(clkp);
			clk_put(clkp);
		}
	}

	return 0;
}

static int samsung_pd_clk_disable(struct clk_should_be_running *clk_run)
{
	struct clk *clkp;
	struct platform_device tmp_dev;
	int i;

	tmp_dev.dev.bus = &platform_bus_type;

	for (i = 0;; i++) {
		if (clk_run[i].clk_name == NULL)
			break;

		tmp_dev.id = clk_run[i].id;
		clkp = clk_get(&tmp_dev.dev, clk_run[i].clk_name);

		if (IS_ERR(clkp)) {
			printk(KERN_ERR "unable to get clock %s\n", clk_run[i].clk_name);
		} else {
			clk_disable(clkp);
			clk_put(clkp);
		}
	}

	return 0;
}

static int samsung_pd_is_enabled(struct regulator_dev *dev)
{
	struct samsung_pd_data *data = rdev_get_drvdata(dev);

	return data->is_enabled;
}

static int samsung_pd_enable(struct regulator_dev *dev)
{
	struct samsung_pd_data *data = rdev_get_drvdata(dev);

	if (data->clk_run)
		samsung_pd_clk_enable(data->clk_run);

	data->enable();
	data->is_enabled = true;

	if (data->clk_run)
		samsung_pd_clk_disable(data->clk_run);

	return 0;
}

static int samsung_pd_disable(struct regulator_dev *dev)
{
	struct samsung_pd_data *data = rdev_get_drvdata(dev);

	data->disable();
	data->is_enabled = false;

	return 0;
}

static int samsung_pd_enable_time(struct regulator_dev *dev)
{
	struct samsung_pd_data *data = rdev_get_drvdata(dev);

	return data->startup_delay;
}

static int samsung_pd_get_voltage(struct regulator_dev *dev)
{
	struct samsung_pd_data *data = rdev_get_drvdata(dev);

	return data->microvolts;
}

static int samsung_pd_list_voltage(struct regulator_dev *dev,
				      unsigned selector)
{
	struct samsung_pd_data *data = rdev_get_drvdata(dev);

	if (selector != 0)
		return -EINVAL;

	return data->microvolts;
}

static struct regulator_ops samsung_pd_ops = {
	.is_enabled = samsung_pd_is_enabled,
	.enable = samsung_pd_enable,
	.disable = samsung_pd_disable,
	.enable_time = samsung_pd_enable_time,
	.get_voltage = samsung_pd_get_voltage,
	.list_voltage = samsung_pd_list_voltage,
};

static int __devinit reg_samsung_pd_probe(struct platform_device *pdev)
{
	struct samsung_pd_config *config = pdev->dev.platform_data;
	struct samsung_pd_data *drvdata;
	int ret;

	drvdata = kzalloc(sizeof(struct samsung_pd_data), GFP_KERNEL);
	if (drvdata == NULL) {
		dev_err(&pdev->dev, "Failed to allocate device data\n");
		ret = -ENOMEM;
		goto err;
	}

	drvdata->desc.name = kstrdup(config->supply_name, GFP_KERNEL);
	if (drvdata->desc.name == NULL) {
		dev_err(&pdev->dev, "Failed to allocate supply name\n");
		ret = -ENOMEM;
		goto err;
	}

	drvdata->desc.type = REGULATOR_VOLTAGE;
	drvdata->desc.owner = THIS_MODULE;
	drvdata->desc.ops = &samsung_pd_ops;
	drvdata->desc.n_voltages = 1;

	drvdata->microvolts = config->microvolts;
	drvdata->startup_delay = config->startup_delay;

	drvdata->is_enabled = config->enabled_at_boot;
	drvdata->clk_run = config->clk_run;

	drvdata->enable = config->enable;
	drvdata->disable = config->disable;

	if (drvdata->is_enabled)
		drvdata->enable();

	drvdata->dev = regulator_register(&drvdata->desc, &pdev->dev,
					  config->init_data, drvdata);
	if (IS_ERR(drvdata->dev)) {
		ret = PTR_ERR(drvdata->dev);
		dev_err(&pdev->dev, "Failed to register regulator: %d\n", ret);
		goto err_name;
	}

	platform_set_drvdata(pdev, drvdata);

	dev_dbg(&pdev->dev, "%s supplying %duV\n", drvdata->desc.name,
		drvdata->microvolts);

	return 0;

err_name:
	kfree(drvdata->desc.name);
err:
	kfree(drvdata);
	return ret;
}

static int __devexit reg_samsung_pd_remove(struct platform_device *pdev)
{
	struct samsung_pd_data *drvdata = platform_get_drvdata(pdev);

	regulator_unregister(drvdata->dev);
	kfree(drvdata->desc.name);
	kfree(drvdata);

	return 0;
}

static struct platform_driver regulator_samsung_pd_driver = {
	.probe		= reg_samsung_pd_probe,
	.remove		= __devexit_p(reg_samsung_pd_remove),
	.driver		= {
		.name		= "reg-samsung-pd",
		.owner		= THIS_MODULE,
	},
};

static int __init regulator_samsung_pd_init(void)
{
	return platform_driver_register(&regulator_samsung_pd_driver);
}
subsys_initcall(regulator_samsung_pd_init);

static void __exit regulator_samsung_pd_exit(void)
{
	platform_driver_unregister(&regulator_samsung_pd_driver);
}
module_exit(regulator_samsung_pd_exit);

MODULE_AUTHOR("Changhwan Youn <chaos@samsung.com>");
MODULE_DESCRIPTION("Samsung power domain regulator");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:reg-samsung-pd");
