/*
 * s5m8751-regulator.c  --  S5M8751 Advanced PMIC with AUDIO DAC
 *
 * Copyright 2009 Samsung Electronics.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/regulator/driver.h>
#include <linux/mfd/s5m8751/core.h>
#include <linux/mfd/s5m8751/regulator.h>

struct s5m8751_regulator {
	char name[NUM_S5M8751_REGULATORS];
	struct regulator_desc desc;
	struct s5m8751 *s5m8751;
	struct regulator_dev *regul;
};

struct s5m8751_regulator_info {
	int def_vol;
	int min_vol;
	int max_vol;
	int step_vol;
};

const struct s5m8751_regulator_info info[] = {
	/* def_vol	min_vol		max_vol		step_vol */
	{ 3300,		1800,		3300,		100 },
	{ 3300,		1800,		3300,		100 },
	{ 1100,		800,		2300,		100 },
	{ 1100,		800,		2300,		100 },
	{ 1800,		1800,		3300,		100 },
	{ 3300,		1800,		3300,		100 },
	{ 1800,		1800,		3300,		25 },
	{ 3300,		1800,		3300,		25 },
	{ 1200,		800,		1650,		25 },
	{ 1100,		800,		1650,		25 },
};

static inline uint8_t s5m8751_mvolts_to_val(int mV, int min, int step)
{
	return (mV - min) / step;
}

static inline int s5m8751_val_to_mvolts(uint8_t val, int min, int step)
{
	return (val * step) + min;
}

static int s5m8751_regulator_set_voltage(struct regulator_dev *rdev, int min_uV,
	int max_uV)
{
	struct s5m8751_regulator *regulator = rdev_get_drvdata(rdev);
	struct s5m8751 *s5m8751 = regulator->s5m8751;
	int ret;
	int volt_reg, regu = rdev_get_id(rdev), mV, min_mV = min_uV / 1000,
		max_mV = max_uV / 1000;
	uint8_t mask, val;

	if (min_mV < info[regu].min_vol || min_mV > info[regu].max_vol)
		return -EINVAL;
	if (max_mV < info[regu].min_vol || max_mV > info[regu].max_vol)
		return -EINVAL;

	mV = s5m8751_mvolts_to_val(min_mV, info[regu].min_vol,
						info[regu].step_vol);

	switch (regu) {
	case S5M8751_LDO1:
		volt_reg = S5M8751_LDO1_VSET;
		mask = S5M8751_LDO1_VSET_MASK;
		break;
	case S5M8751_LDO2:
		volt_reg = S5M8751_LDO2_VSET;
		mask = S5M8751_LDO2_VSET_MASK;
		break;
	case S5M8751_LDO3:
		volt_reg = S5M8751_LDO3_VSET;
		mask = S5M8751_LDO3_VSET_MASK;
		break;
	case S5M8751_LDO4:
		volt_reg = S5M8751_LDO4_VSET;
		mask = S5M8751_LDO4_VSET_MASK;
		break;
	case S5M8751_LDO5:
		volt_reg = S5M8751_LDO5_VSET;
		mask = S5M8751_LDO5_VSET_MASK;
		break;
	case S5M8751_LDO6:
		volt_reg = S5M8751_LDO6_VSET;
		mask = S5M8751_LDO6_VSET_MASK;
		break;
	case S5M8751_BUCK1_1:
		volt_reg = S5M8751_BUCK1_V1_SET;
		mask = S5M8751_BUCK1_V1_SET_MASK;
		break;
	case S5M8751_BUCK1_2:
		volt_reg = S5M8751_BUCK1_V2_SET;
		mask = S5M8751_BUCK1_V2_SET_MASK;
		break;
	case S5M8751_BUCK2_1:
		volt_reg = S5M8751_BUCK2_V1_SET;
		mask = S5M8751_BUCK2_V1_SET_MASK;
		break;
	case S5M8751_BUCK2_2:
		volt_reg = S5M8751_BUCK2_V2_SET;
		mask = S5M8751_BUCK2_V2_SET_MASK;
		break;
	default:
		return -EINVAL;
	}
	ret = s5m8751_reg_read(s5m8751, volt_reg, &val);
	if (ret)
		goto out;

	val &= ~mask;
	ret = s5m8751_reg_write(s5m8751, volt_reg, val | mV);
out:
	return ret;
}

static int s5m8751_regulator_get_voltage(struct regulator_dev *rdev)
{
	struct s5m8751_regulator *regulator = rdev_get_drvdata(rdev);
	struct s5m8751 *s5m8751 = regulator->s5m8751;
	int ret;
	int volt_reg, regu = rdev_get_id(rdev);
	uint8_t mask, val;

	switch (regu) {
	case S5M8751_LDO1:
		volt_reg = S5M8751_LDO1_VSET;
		mask = S5M8751_LDO1_VSET_MASK;
		break;
	case S5M8751_LDO2:
		volt_reg = S5M8751_LDO2_VSET;
		mask = S5M8751_LDO2_VSET_MASK;
		break;
	case S5M8751_LDO3:
		volt_reg = S5M8751_LDO3_VSET;
		mask = S5M8751_LDO3_VSET_MASK;
		break;
	case S5M8751_LDO4:
		volt_reg = S5M8751_LDO4_VSET;
		mask = S5M8751_LDO4_VSET_MASK;
		break;
	case S5M8751_LDO5:
		volt_reg = S5M8751_LDO5_VSET;
		mask = S5M8751_LDO5_VSET_MASK;
		break;
	case S5M8751_LDO6:
		volt_reg = S5M8751_LDO6_VSET;
		mask = S5M8751_LDO6_VSET_MASK;
		break;
	case S5M8751_BUCK1_1:
		volt_reg = S5M8751_BUCK1_V1_SET;
		mask = S5M8751_BUCK1_V1_SET_MASK;
		break;
	case S5M8751_BUCK1_2:
		volt_reg = S5M8751_BUCK1_V2_SET;
		mask = S5M8751_BUCK1_V2_SET_MASK;
		break;
	case S5M8751_BUCK2_1:
		volt_reg = S5M8751_BUCK2_V1_SET;
		mask = S5M8751_BUCK2_V1_SET_MASK;
		break;
	case S5M8751_BUCK2_2:
		volt_reg = S5M8751_BUCK2_V2_SET;
		mask = S5M8751_BUCK2_V2_SET_MASK;
		break;
	default:
		return -EINVAL;
	}
	ret = s5m8751_reg_read(s5m8751, volt_reg, &val);
	if (ret)
		goto out;

	val &= mask;
	ret = s5m8751_val_to_mvolts(val, info[regu].min_vol,
					info[regu].step_vol) * 1000;
out:
	return ret;
}

static int s5m8751_regulator_enable(struct regulator_dev *rdev)
{
	struct s5m8751_regulator *regulator = rdev_get_drvdata(rdev);
	struct s5m8751 *s5m8751 = regulator->s5m8751;
	int ret;
	int regu = rdev_get_id(rdev);
	uint8_t enable_reg, shift;

	switch (regu) {
	case S5M8751_LDO1:
		enable_reg = S5M8751_ONOFF2;
		shift = S5M8751_LDO1_SHIFT;
		break;
	case S5M8751_LDO2:
		enable_reg = S5M8751_ONOFF2;
		shift = S5M8751_LDO2_SHIFT;
		break;
	case S5M8751_LDO3:
		enable_reg = S5M8751_ONOFF2;
		shift = S5M8751_LDO3_SHIFT;
		break;
	case S5M8751_LDO4:
		enable_reg = S5M8751_ONOFF2;
		shift = S5M8751_LDO4_SHIFT;
		break;
	case S5M8751_LDO5:
		enable_reg = S5M8751_ONOFF2;
		shift = S5M8751_LDO5_SHIFT;
		break;
	case S5M8751_LDO6:
		enable_reg = S5M8751_ONOFF3;
		shift = S5M8751_LDO6_SHIFT;
		break;
	case S5M8751_BUCK1_1:
	case S5M8751_BUCK1_2:
		enable_reg = S5M8751_ONOFF3;
		shift = S5M8751_BUCK1_SHIFT;
		break;
	case S5M8751_BUCK2_1:
	case S5M8751_BUCK2_2:
		enable_reg = S5M8751_ONOFF2;
		shift = S5M8751_BUCK2_SHIFT;
		break;
	default:
		return -EINVAL;
	}
	ret = s5m8751_set_bits(s5m8751, enable_reg, 1 << shift);

	return ret;
}

static int s5m8751_regulator_disable(struct regulator_dev *rdev)
{
	struct s5m8751_regulator *regulator = rdev_get_drvdata(rdev);
	struct s5m8751 *s5m8751 = regulator->s5m8751;
	int regu = rdev_get_id(rdev);
	int ret;
	uint8_t enable_reg, shift;

	switch (regu) {
	case S5M8751_LDO1:
		enable_reg = S5M8751_ONOFF2;
		shift = S5M8751_LDO1_SHIFT;
		break;
	case S5M8751_LDO2:
		enable_reg = S5M8751_ONOFF2;
		shift = S5M8751_LDO2_SHIFT;
		break;
	case S5M8751_LDO3:
		enable_reg = S5M8751_ONOFF2;
		shift = S5M8751_LDO3_SHIFT;
		break;
	case S5M8751_LDO4:
		enable_reg = S5M8751_ONOFF2;
		shift = S5M8751_LDO4_SHIFT;
		break;
	case S5M8751_LDO5:
		enable_reg = S5M8751_ONOFF2;
		shift = S5M8751_LDO5_SHIFT;
		break;
	case S5M8751_LDO6:
		enable_reg = S5M8751_ONOFF3;
		shift = S5M8751_LDO6_SHIFT;
		break;
	case S5M8751_BUCK1_1:
	case S5M8751_BUCK1_2:
		enable_reg = S5M8751_ONOFF3;
		shift = S5M8751_BUCK1_SHIFT;
		break;
	case S5M8751_BUCK2_1:
	case S5M8751_BUCK2_2:
		enable_reg = S5M8751_ONOFF2;
		shift = S5M8751_BUCK2_SHIFT;
		break;
	default:
		return -EINVAL;
	}
	ret = s5m8751_clear_bits(s5m8751, enable_reg, 1 << shift);

	return ret;
}

static int s5m8751_regulator_set_suspend_enable(struct regulator_dev *rdev)
{
	struct s5m8751_regulator *regulator = rdev_get_drvdata(rdev);
	struct s5m8751 *s5m8751 = regulator->s5m8751;
	int ret;
	int regu = rdev_get_id(rdev);
	uint8_t enable_reg, shift;

	switch (regu) {
	case S5M8751_LDO1:
		enable_reg = S5M8751_SLEEP_CNTL1;
		shift = S5M8751_LDO1_SHIFT;
		break;
	case S5M8751_LDO2:
		enable_reg = S5M8751_SLEEP_CNTL1;
		shift = S5M8751_LDO2_SHIFT;
		break;
	case S5M8751_LDO3:
		enable_reg = S5M8751_SLEEP_CNTL1;
		shift = S5M8751_LDO3_SHIFT;
		break;
	case S5M8751_LDO4:
		enable_reg = S5M8751_SLEEP_CNTL1;
		shift = S5M8751_LDO4_SHIFT;
		break;
	case S5M8751_LDO5:
		enable_reg = S5M8751_SLEEP_CNTL1;
		shift = S5M8751_LDO5_SHIFT;
		break;
	case S5M8751_LDO6:
		enable_reg = S5M8751_SLEEP_CNTL2;
		shift = S5M8751_LDO6_SHIFT;
		break;
	case S5M8751_BUCK1_1:
	case S5M8751_BUCK1_2:
		enable_reg = S5M8751_SLEEP_CNTL2;
		shift = S5M8751_BUCK1_SHIFT;
		break;
	case S5M8751_BUCK2_1:
	case S5M8751_BUCK2_2:
		enable_reg = S5M8751_SLEEP_CNTL2;
		shift = S5M8751_BUCK2_SHIFT;
		break;
	default:
		return -EINVAL;
	}

	ret = s5m8751_set_bits(s5m8751, enable_reg, 1 << shift);

	return ret;
}

static int s5m8751_regulator_set_suspend_disable(struct regulator_dev *rdev)
{
	struct s5m8751_regulator *regulator = rdev_get_drvdata(rdev);
	struct s5m8751 *s5m8751 = regulator->s5m8751;
	int ret;
	int regu = rdev_get_id(rdev);
	uint8_t enable_reg, shift;

	switch (regu) {
	case S5M8751_LDO1:
		enable_reg = S5M8751_SLEEP_CNTL1;
		shift = S5M8751_LDO1_SHIFT;
		break;
	case S5M8751_LDO2:
		enable_reg = S5M8751_SLEEP_CNTL1;
		shift = S5M8751_LDO2_SHIFT;
		break;
	case S5M8751_LDO3:
		enable_reg = S5M8751_SLEEP_CNTL1;
		shift = S5M8751_LDO3_SHIFT;
		break;
	case S5M8751_LDO4:
		enable_reg = S5M8751_SLEEP_CNTL1;
		shift = S5M8751_LDO4_SHIFT;
		break;
	case S5M8751_LDO5:
		enable_reg = S5M8751_SLEEP_CNTL1;
		shift = S5M8751_LDO5_SHIFT;
		break;
	case S5M8751_LDO6:
		enable_reg = S5M8751_SLEEP_CNTL2;
		shift = S5M8751_LDO6_SHIFT;
		break;
	case S5M8751_BUCK1_1:
	case S5M8751_BUCK1_2:
		enable_reg = S5M8751_SLEEP_CNTL2;
		shift = S5M8751_BUCK1_SHIFT;
		break;
	case S5M8751_BUCK2_1:
	case S5M8751_BUCK2_2:
		enable_reg = S5M8751_SLEEP_CNTL2;
		shift = S5M8751_BUCK2_SHIFT;
		break;
	default:
		return -EINVAL;
	}

	ret = s5m8751_clear_bits(s5m8751, enable_reg, 1 << shift);

	return ret;
}

static int s5m8751_regulator_is_enabled(struct regulator_dev *rdev)
{
	struct s5m8751_regulator *regulator = rdev_get_drvdata(rdev);
	struct s5m8751 *s5m8751 = regulator->s5m8751;
	int ret;
	int regu = rdev_get_id(rdev), shift;
	uint8_t enable_reg, val;

	switch (regu) {
	case S5M8751_LDO1:
		enable_reg = S5M8751_ONOFF2;
		shift = S5M8751_LDO1_SHIFT;
		break;
	case S5M8751_LDO2:
		enable_reg = S5M8751_ONOFF2;
		shift = S5M8751_LDO2_SHIFT;
		break;
	case S5M8751_LDO3:
		enable_reg = S5M8751_ONOFF2;
		shift = S5M8751_LDO3_SHIFT;
		break;
	case S5M8751_LDO4:
		enable_reg = S5M8751_ONOFF2;
		shift = S5M8751_LDO4_SHIFT;
		break;
	case S5M8751_LDO5:
		enable_reg = S5M8751_ONOFF2;
		shift = S5M8751_LDO5_SHIFT;
		break;
	case S5M8751_LDO6:
		enable_reg = S5M8751_ONOFF3;
		shift = S5M8751_LDO6_SHIFT;
		break;
	case S5M8751_BUCK1_1:
	case S5M8751_BUCK1_2:
		enable_reg = S5M8751_ONOFF3;
		shift = S5M8751_BUCK1_SHIFT;
		break;
	case S5M8751_BUCK2_1:
	case S5M8751_BUCK2_2:
		enable_reg = S5M8751_ONOFF2;
		shift = S5M8751_BUCK2_SHIFT;
		break;
	default:
		return -EINVAL;
	}
	ret = s5m8751_reg_read(s5m8751, enable_reg, &val);
	if (ret)
		goto out;

	ret = val & (1 << shift);
out:
	return ret;
}

static struct regulator_ops s5m8751_regulator_ops = {
	.set_voltage = s5m8751_regulator_set_voltage,
	.get_voltage = s5m8751_regulator_get_voltage,
	.enable = s5m8751_regulator_enable,
	.disable = s5m8751_regulator_disable,
	.is_enabled = s5m8751_regulator_is_enabled,
	.set_suspend_enable = s5m8751_regulator_set_suspend_enable,
	.set_suspend_disable = s5m8751_regulator_set_suspend_disable,
};

static __devinit int s5m8751_regulator_probe(struct platform_device *pdev)
{
	struct s5m8751 *s5m8751 = dev_get_drvdata(pdev->dev.parent);
	struct s5m8751_pdata *pdata = s5m8751->dev->platform_data;
	int id = pdev->id % ARRAY_SIZE(pdata->regulator);
	struct s5m8751_regulator *regulator;
	struct resource *res;
	int ret;

	if (pdata == NULL || pdata->regulator[id] == NULL)
		return -ENODEV;

	regulator = kzalloc(sizeof(struct s5m8751_regulator), GFP_KERNEL);
	if (regulator == NULL) {
		dev_err(&pdev->dev, "Unable to allocate private data\n");
		return -ENOMEM;
	}

	regulator->s5m8751 = s5m8751;

	res = platform_get_resource(pdev, IORESOURCE_IO, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "No I/O resource\n");
		ret = -EINVAL;
		goto err;
	}

	regulator->desc.name = regulator->name;
	regulator->desc.id = id;
	regulator->desc.type = REGULATOR_VOLTAGE;
	regulator->desc.ops = &s5m8751_regulator_ops;
	regulator->desc.owner = THIS_MODULE;

	regulator->regul = regulator_register(&regulator->desc, &pdev->dev,
			pdata->regulator[id], regulator);

	if (IS_ERR(regulator->regul)) {
		ret = PTR_ERR(regulator->regul);
		dev_err(s5m8751->dev, "Failed to register Regulator%d: %d\n",
			id + 1, ret);
		goto err;
	}

	platform_set_drvdata(pdev, regulator);

	return 0;

err:
	kfree(regulator);
	return ret;
}

static __devexit int s5m8751_regulator_remove(struct platform_device *pdev)
{
	struct s5m8751_regulator *regulator = platform_get_drvdata(pdev);

	regulator_unregister(regulator->regul);
	kfree(regulator);

	return 0;
}

static struct platform_driver s5m8751_regulator_driver = {
	.probe = s5m8751_regulator_probe,
	.remove = __devexit_p(s5m8751_regulator_remove),
	.driver		= {
		.name	= "s5m8751-regulator",
		.owner	= THIS_MODULE,
	},
};

static int __init s5m8751_regulator_init(void)
{
	int ret;

	ret = platform_driver_register(&s5m8751_regulator_driver);
	if (ret != 0)
		pr_err("Failed to register S5M8751 Regulator driver: %d\n",
									ret);
	return ret;
}
module_init(s5m8751_regulator_init);

static void __exit s5m8751_regulator_exit(void)
{
	platform_driver_unregister(&s5m8751_regulator_driver);
}
module_exit(s5m8751_regulator_exit);

/* Module Information */
MODULE_DESCRIPTION("S5M8751 Regulator driver");
MODULE_AUTHOR("Jongbin, Won <jongbin.won@samsung.com>");
MODULE_LICENSE("GPL");
