/*
 * max8698.c  --  Voltage and current regulation for the Maxim 8698
 *
 * Copyright (C) 2009 Samsung Electronics
 * Kyungmin Park <kyungmin.park@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/max8698.h>
#include <linux/mutex.h>
#include <linux/gpio.h>
#include <linux/delay.h>

/* Registers */
enum {
	MAX8698_REG_ONOFF1,
	MAX8698_REG_ONOFF2,
	MAX8698_REG_ADISCHG_EN1,
	MAX8698_REG_ADISCHG_EN2,
	MAX8698_REG_DVSARM12,
	MAX8698_REG_DVSARM34,
	MAX8698_REG_DVSINT12,
	MAX8698_REG_BUCK3,
	MAX8698_REG_LDO2_LDO3,
	MAX8698_REG_LDO4,
	MAX8698_REG_LDO5,
	MAX8698_REG_LDO6,
	MAX8698_REG_LDO7,
	MAX8698_REG_LDO8_BKCHAR,
	MAX8698_REG_LDO9,
	MAX8698_REG_LBCNFG,
};

enum max8698_dvs_mode {
	MAX8698_DVSARM1,
	MAX8698_DVSARM2,
	MAX8698_DVSARM3,
	MAX8698_DVSARM4,
	MAX8698_DVSINT1,
	MAX8698_DVSINT2,
};

struct max8698_data {
	struct i2c_client	*client;
	struct device		*dev;

	struct mutex		mutex;
	struct max8698_platform_data *pdata;

	int			set1;
	int			set2;
	int			set3;

/*
	struct work_struct	work;
	int			_irq;
	int			_ono;
*/

	int			num_regulators;
	struct regulator_dev	**rdev;
};

static u8 max8698_cache_regs[16] = {
	0xFA, 0xB1, 0xFF, 0xF9,
	0x99, 0x99, 0x99, 0x02,
	0x88, 0x02, 0x0C, 0x0A,
	0x0E, 0x33, 0x0E, 0x16,
};

static int max8698_i2c_cache_read(struct i2c_client *client, u8 reg, u8 *dest)
{
	*dest = max8698_cache_regs[reg];
	return 0;
}

static int max8698_i2c_read(struct i2c_client *client, u8 reg, u8 *dest)
{
	int ret;

	ret = i2c_smbus_read_byte_data(client, reg);
	if (ret < 0)
		return -EIO;

	max8698_cache_regs[reg] = ret;

	*dest = ret & 0xff;
	return 0;
}

static int max8698_i2c_write(struct i2c_client *client, u8 reg, u8 value)
{
	max8698_cache_regs[reg] = value;
	return i2c_smbus_write_byte_data(client, reg, value);
}

static u8 max8698_read_reg(struct max8698_data *max8698, u8 reg)
{
	u8 val = 0;

	mutex_lock(&max8698->mutex);
	max8698_i2c_cache_read(max8698->client, reg, &val);
	mutex_unlock(&max8698->mutex);

	return val;
}

static int max8698_write_reg(struct max8698_data *max8698, u8 reg, u8 value)
{
	mutex_lock(&max8698->mutex);

	max8698_i2c_write(max8698->client, reg, value);

	mutex_unlock(&max8698->mutex);

	return 0;
}

static const int ldo23_voltage_map[] = {
	 800,  850,  900,  950, 1000,
	1050, 1100, 1150, 1200, 1250,
	1300,
};

static const int ldo45679_voltage_map[] = {
	1600, 1700, 1800, 1900, 2000,
	2100, 2200, 2300, 2400, 2500,
	2600, 2700, 2800, 2900, 3000,
	3100, 3200, 3300, 3400, 3500,
	3600,
};

static const int ldo8_voltage_map[] = {
	3000, 3100, 3200, 3300, 3400,
	3500, 3600,
};

static const int buck12_voltage_map[] = {
	 750,  800,  850,  900,  950,
	1000, 1050, 1100, 1150, 1200,
	1250, 1300, 1350, 1400, 1450,
	1500,
};

static const int *ldo_voltage_map[] = {
	NULL,
	ldo23_voltage_map,	/* LDO1 */
	ldo23_voltage_map,	/* LDO2 */
	ldo23_voltage_map,	/* LDO3 */
	ldo45679_voltage_map,	/* LDO4 */
	ldo45679_voltage_map,	/* LDO5 */
	ldo45679_voltage_map,	/* LDO6 */
	ldo45679_voltage_map,	/* LDO7 */
	ldo8_voltage_map,	/* LDO8 */
	ldo45679_voltage_map,	/* LDO9 */
	buck12_voltage_map,	/* BUCK1 */
	buck12_voltage_map,	/* BUCK2 */
	ldo45679_voltage_map,	/* BUCK3 */
};

static int max8698_get_ldo(struct regulator_dev *rdev)
{
	return rdev_get_id(rdev);
}

static int max8698_ldo_list_voltage(struct regulator_dev *rdev, unsigned selector)
{
	int ldo = max8698_get_ldo(rdev);

	if (ldo > ARRAY_SIZE(ldo_voltage_map))
		return -EINVAL;

	return 1000 * ldo_voltage_map[ldo][selector];
}

static int max8698_get_register(struct regulator_dev *rdev,
					int *reg, int *shift)
{
	int ldo = max8698_get_ldo(rdev);

	switch (ldo) {
	case MAX8698_LDO2 ... MAX8698_LDO5:
		*reg = MAX8698_REG_ONOFF1;
		*shift = 4 - (ldo - MAX8698_LDO2);
		break;
	case MAX8698_LDO6 ... MAX8698_LDO9:
		*reg = MAX8698_REG_ONOFF2;
		*shift = 7 - (ldo - MAX8698_LDO6);
		break;
	case MAX8698_BUCK1 ... MAX8698_BUCK3:
		*reg = MAX8698_REG_ONOFF1;
		*shift = 7 - (ldo - MAX8698_BUCK1);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int max8698_ldo_is_enabled(struct regulator_dev *rdev)
{
	struct max8698_data *max8698 = rdev_get_drvdata(rdev);
	int reg, shift, value;

	value = max8698_get_register(rdev, &reg, &shift);
	if (value)
		return value;

	value = max8698_read_reg(max8698, reg);

	return value & (1 << shift);
}

static int max8698_ldo_enable(struct regulator_dev *rdev)
{
	struct max8698_data *max8698 = rdev_get_drvdata(rdev);
	int reg, shift = 8, value, ret;
	
	ret = max8698_get_register(rdev, &reg, &shift);
	if (ret)
		return ret;

	value = max8698_read_reg(max8698, reg);
	if (!(value & (1 << shift))) {
		value |= (1 << shift);
		ret = max8698_write_reg(max8698, reg, value);
	}
	printk("%s[%d] ldo %d\n", __func__, __LINE__, max8698_get_ldo(rdev));

	return ret;
}

static int max8698_ldo_disable(struct regulator_dev *rdev)
{
	struct max8698_data *max8698 = rdev_get_drvdata(rdev);
	int reg, shift = 8, value, ret;

	ret = max8698_get_register(rdev, &reg, &shift);
	if (ret)
		return ret;

	value = max8698_read_reg(max8698, reg);
	if (value & (1 << shift)) {
		value &= ~(1 << shift);
		ret = max8698_write_reg(max8698, reg, value);
	}
	printk("%s[%d] ldo %d\n", __func__, __LINE__, max8698_get_ldo(rdev));

	return ret;
}

static int max8698_get_voltage_register(struct regulator_dev *rdev,
				int *_reg, int *_shift, int *_mask)
{
	struct max8698_data *max8698 = rdev_get_drvdata(rdev);
	int ldo = max8698_get_ldo(rdev);
	int reg, shift = -1, mask = 0xff;
	int temp;

	switch (ldo) {
	case MAX8698_LDO2 ... MAX8698_LDO3:
		reg = MAX8698_REG_LDO2_LDO3;
		mask = 0xf;
		if (ldo == MAX8698_LDO2)
			shift = 4;
		else
			shift = 0;
		break;
	case MAX8698_LDO4 ... MAX8698_LDO7:
		reg = MAX8698_REG_LDO4 + (ldo - MAX8698_LDO4);
		break;
	case MAX8698_LDO8:
		reg = MAX8698_REG_LDO8_BKCHAR;
		mask = 0xf;
		shift = 4;
		break;
	case MAX8698_LDO9:
		reg = MAX8698_REG_LDO9;
		break;
	case MAX8698_BUCK1:
		reg = MAX8698_REG_DVSARM12;
		if (gpio_is_valid(max8698->set1) &&
		    gpio_is_valid(max8698->set2)) {
			temp = (gpio_get_value(max8698->set2) << 1) +
				gpio_get_value(max8698->set1);
			reg += temp/2;
			mask = 0xf;
			shift = (temp%2) ? 0:4; 
		}
		break;
	case MAX8698_BUCK2:
		reg = MAX8698_REG_DVSINT12;
		if (gpio_is_valid(max8698->set3)) {
			temp = gpio_get_value(max8698->set3);
			mask = 0xf;
			shift = (temp%2) ? 0:4;
		}
		break;
	case MAX8698_BUCK3:
		reg = MAX8698_REG_BUCK3;
		break;
		break;
	default:
		return -EINVAL;
	}

	*_reg = reg;
	*_shift = shift;
	*_mask = mask;

	return 0;
}

static int max8698_ldo_get_voltage(struct regulator_dev *rdev)
{
	struct max8698_data *max8698 = rdev_get_drvdata(rdev);
	int ldo = max8698_get_ldo(rdev);
	int reg, shift = -1, mask, value, ret;

	ret = max8698_get_voltage_register(rdev, &reg, &shift, &mask);
	if (ret)
		return ret;

	value = max8698_read_reg(max8698, reg);
	if (shift >= 0) {
		value >>= shift;
		value &= mask;
	}
	if (ldo > ARRAY_SIZE(ldo_voltage_map))
		return -EINVAL;

	return ldo_voltage_map[ldo][value] * 1000;
}

static int max8698_ldo_set_voltage(struct regulator_dev *rdev,
				int min_uV, int max_uV)
{
	struct max8698_data *max8698 = rdev_get_drvdata(rdev);
	int min_vol = min_uV / 1000, max_vol = max_uV / 1000;
	int ldo = max8698_get_ldo(rdev);
	const int *vol_map = ldo_voltage_map[ldo];
	int reg, shift = -1, mask, value, ret;
	int i;

	for (i = 0; i < vol_map[i]; i++) {
		if (vol_map[i] >= min_vol)
			break;
	}

	if (!vol_map[i] || vol_map[i] > max_vol)
		return -EINVAL;

	ret = max8698_get_voltage_register(rdev, &reg, &shift, &mask);
	if (ret)
		return ret;

	value = max8698_read_reg(max8698, reg);
	if (shift >= 0) {
		value &= ~(mask << shift);
		value |= i << shift;
	} else
		value = i;
	ret = max8698_write_reg(max8698, reg, value);

	return ret;
}

static int max8698_get_voltage(struct regulator_dev *rdev)
{
	struct max8698_data *max8698 = rdev_get_drvdata(rdev);
	int ldo = max8698_get_ldo(rdev);
	int reg, shift = -1, mask, value, ret;

	ret = max8698_get_voltage_register(rdev, &reg, &shift, &mask);
	if (ret)
		return ret;

	value = max8698_read_reg(max8698, reg);
	if (shift >= 0) {
		value >>= shift;
		value &= mask;
	}
	if (ldo > ARRAY_SIZE(ldo_voltage_map))
		return -EINVAL;

	return ldo_voltage_map[ldo][value] * 1000;
}

static int max8698_set_voltage(struct regulator_dev *rdev,
				int min_uV, int max_uV)
{
	struct max8698_data *max8698 = rdev_get_drvdata(rdev);
	int min_vol = min_uV / 1000, max_vol = max_uV / 1000;
	int ldo = max8698_get_ldo(rdev);
	const int *vol_map = ldo_voltage_map[ldo];
	int reg, shift = -1, mask, value, ret;
	int i;

	for (i = 0; i < vol_map[i]; i++) {
		if (vol_map[i] >= min_vol)
			break;
	}

	if (!vol_map[i] || vol_map[i] > max_vol)
		return -EINVAL;

	ret = max8698_get_voltage_register(rdev, &reg, &shift, &mask);
	if (ret)
		return ret;

	value = max8698_read_reg(max8698, reg);
	if (shift >= 0) {
		value &= ~(mask << shift);
		value |= i << shift;
	} else
		value = i;
	ret = max8698_write_reg(max8698, reg, value);

	return ret;
}


static int max8698_set_buck_dvs(struct max8698_data *max8698, int mode)
{
	switch (mode) {
	case MAX8698_DVSARM1:
		gpio_set_value(max8698->set2, 0);
		gpio_set_value(max8698->set1, 0);
		break;
	case MAX8698_DVSARM2:
		gpio_set_value(max8698->set2, 0);
		gpio_set_value(max8698->set1, 1);
		break;
	case MAX8698_DVSARM3:
		gpio_set_value(max8698->set2, 1);
		gpio_set_value(max8698->set1, 0);
		break;
	case MAX8698_DVSARM4:
		gpio_set_value(max8698->set2, 1);
		gpio_set_value(max8698->set1, 1);
		break;
	case MAX8698_DVSINT1:
		gpio_set_value(max8698->set3, 0);
		break;
	case MAX8698_DVSINT2:
		gpio_set_value(max8698->set3, 1);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int max8698_buck_set_voltage(struct regulator_dev *rdev,
				int min_uV, int max_uV)
{
	struct max8698_data *max8698 = rdev_get_drvdata(rdev);
	struct max8698_platform_data *pdata = max8698->pdata;
	int min_vol = min_uV / 1000;
	int ldo = max8698_get_ldo(rdev);

	if (ldo == MAX8698_BUCK1) {
		if (min_vol == buck12_voltage_map[pdata->dvsarm2])
			return max8698_set_buck_dvs(max8698, MAX8698_DVSARM2);
		else if (min_vol == buck12_voltage_map[pdata->dvsarm3])
			return max8698_set_buck_dvs(max8698, MAX8698_DVSARM3);
		else if (min_vol == buck12_voltage_map[pdata->dvsarm4])
			return max8698_set_buck_dvs(max8698, MAX8698_DVSARM4);

		return max8698_set_buck_dvs(max8698, MAX8698_DVSARM1);
	}
	if (ldo == MAX8698_BUCK2) {
		if (min_vol == buck12_voltage_map[pdata->dvsint2])
			return max8698_set_buck_dvs(max8698, MAX8698_DVSINT2);

		return max8698_set_buck_dvs(max8698, MAX8698_DVSINT1);
	}
	
	return max8698_set_voltage(rdev, min_uV, max_uV);
}

static int max8698_set_suspend_voltage(struct regulator_dev *rdev, int uV)
{
	int ldo = max8698_get_ldo(rdev);
	
	switch (ldo) {

	case MAX8698_BUCK1 ... MAX8698_BUCK3:
		return max8698_buck_set_voltage(rdev, uV, uV);
	case MAX8698_LDO1 ... MAX8698_LDO9:
		return max8698_set_voltage(rdev, uV, uV);
	default:
		return -EINVAL;
	}
}

static struct regulator_ops max8698_ldo_ops = {
	.list_voltage	= max8698_ldo_list_voltage,
	.is_enabled	= max8698_ldo_is_enabled,
	.enable		= max8698_ldo_enable,
	.disable	= max8698_ldo_disable,
	.get_voltage	= max8698_get_voltage,
	.set_voltage	= max8698_set_voltage,
	.set_suspend_enable	= max8698_ldo_enable,
	.set_suspend_disable	= max8698_ldo_disable,
	.set_suspend_voltage	= max8698_set_suspend_voltage,
};

static struct regulator_ops max8698_buck_ops = {
	.list_voltage	= max8698_ldo_list_voltage,
	.is_enabled	= max8698_ldo_is_enabled,
	.enable		= max8698_ldo_enable,
	.disable	= max8698_ldo_disable,
	.get_voltage	= max8698_get_voltage,
	.set_voltage	= max8698_buck_set_voltage,
	.set_suspend_enable	= max8698_ldo_enable,
	.set_suspend_disable	= max8698_ldo_disable,
	.set_suspend_voltage	= max8698_set_suspend_voltage,
};

static struct regulator_desc regulators[] = {
	{
		.name		= "LDO1",
		.id		= MAX8698_LDO1,
		.ops		= &max8698_ldo_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO2",
		.id		= MAX8698_LDO2,
		.ops		= &max8698_ldo_ops,
		.n_voltages	= ARRAY_SIZE(ldo23_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO3",
		.id		= MAX8698_LDO3,
		.ops		= &max8698_ldo_ops,
		.n_voltages	= ARRAY_SIZE(ldo23_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO4",
		.id		= MAX8698_LDO4,
		.ops		= &max8698_ldo_ops,
		.n_voltages	= ARRAY_SIZE(ldo45679_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO5",
		.id		= MAX8698_LDO5,
		.ops		= &max8698_ldo_ops,
		.n_voltages	= ARRAY_SIZE(ldo45679_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO6",
		.id		= MAX8698_LDO6,
		.ops		= &max8698_ldo_ops,
		.n_voltages	= ARRAY_SIZE(ldo45679_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO7",
		.id		= MAX8698_LDO7,
		.ops		= &max8698_ldo_ops,
		.n_voltages	= ARRAY_SIZE(ldo45679_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO8",
		.id		= MAX8698_LDO8,
		.ops		= &max8698_ldo_ops,
		.n_voltages	= ARRAY_SIZE(ldo8_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO9",
		.id		= MAX8698_LDO9,
		.ops		= &max8698_ldo_ops,
		.n_voltages	= ARRAY_SIZE(ldo45679_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "BUCK1",
		.id		= MAX8698_BUCK1,
		.ops		= &max8698_buck_ops,
		.n_voltages	= ARRAY_SIZE(buck12_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "BUCK2",
		.id		= MAX8698_BUCK2,
		.ops		= &max8698_buck_ops,
		.n_voltages	= ARRAY_SIZE(buck12_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "BUCK3",
		.id		= MAX8698_BUCK3,
		.ops		= &max8698_buck_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	},
};

static int __devinit max8698_pmic_probe(struct i2c_client *client,
				const struct i2c_device_id *i2c_id)
{
	struct regulator_dev **rdev;
	struct max8698_platform_data *pdata = client->dev.platform_data;
	struct max8698_data *max8698;
	int i = 0, id, ret;

	if (!pdata)
		return -EINVAL;

	max8698 = kzalloc(sizeof(struct max8698_data), GFP_KERNEL);
	if (!max8698)
		return -ENOMEM;

	max8698->rdev = kzalloc(sizeof(struct regulator_dev *) * (pdata->num_regulators + 1), GFP_KERNEL);
	if (!max8698->rdev) {
		kfree(max8698);
		return -ENOMEM;
	}

	max8698->client = client;
	max8698->dev = &client->dev;
	max8698->pdata = pdata;
	max8698->set1 = pdata->set1 ? : -EINVAL;
	max8698->set2 = pdata->set2 ? : -EINVAL;
	max8698->set3 = pdata->set3 ? : -EINVAL;
	mutex_init(&max8698->mutex);

	max8698->num_regulators = pdata->num_regulators;
	for (i = 0; i < pdata->num_regulators; i++) {
		id = pdata->regulators[i].id - MAX8698_LDO1;
		max8698->rdev[i] = regulator_register(&regulators[id],
			max8698->dev, pdata->regulators[i].initdata, max8698);

		ret = IS_ERR(max8698->rdev[i]);
		if (ret)
			dev_err(max8698->dev, "regulator init failed\n");
	}
	rdev = max8698->rdev;
	
	if (pdata->dvsint1 && pdata->dvsint2 && gpio_is_valid(pdata->set3)) {
		max8698_write_reg(max8698, MAX8698_REG_DVSINT12,
			pdata->dvsint1 | (pdata->dvsint2 << 4));

		ret = gpio_request(pdata->set3, "MAX8698 SET3");
		if (ret)
			goto out_set3;
		gpio_direction_output(pdata->set3, 0);
	}

	mdelay(1);

	if (pdata->dvsarm1 && pdata->dvsarm2 && pdata->dvsarm3 &&
	    pdata->dvsarm4 &&
	    gpio_is_valid(pdata->set1) && gpio_is_valid(pdata->set2)) {
		max8698_write_reg(max8698, MAX8698_REG_DVSARM12,
			pdata->dvsarm1 | (pdata->dvsarm2 << 4));
		max8698_write_reg(max8698, MAX8698_REG_DVSARM34,
			pdata->dvsarm3 | (pdata->dvsarm4 << 4));

		ret = gpio_request(pdata->set1, "MAX8698 SET1");
		if (ret)
			return ret;
		gpio_direction_output(pdata->set1, 0);
		ret = gpio_request(pdata->set2, "MAX8698 SET2");
		if (ret)
			goto out_set2;
		gpio_direction_output(pdata->set2, 0);
	}
#if 0
	if (pdata->dvsint1 && pdata->dvsint2 && gpio_is_valid(pdata->set3)) {
		max8698_write_reg(max8698, MAX8698_REG_DVSINT12,
			pdata->dvsint1 | (pdata->dvsint2 << 4));

		ret = gpio_request(pdata->set3, "MAX8698 SET3");
		if (ret)
			goto out_set3;
		gpio_direction_output(pdata->set3, 0);
	}
#endif
	i2c_set_clientdata(client, max8698);

	return 0;
out_set3:
	gpio_free(pdata->set2);
out_set2:
	gpio_free(pdata->set1);
	return ret;
}

static int __devexit max8698_pmic_remove(struct i2c_client *client)
{
	struct max8698_data *max8698 = i2c_get_clientdata(client);
	struct max8698_platform_data *pdata = max8698->pdata;
	struct regulator_dev **rdev = max8698->rdev;
	int i;

	for (i = 0; i <= max8698->num_regulators; i++)
		if (rdev[i])
			regulator_unregister(rdev[i]);
	if (pdata->set1)
		gpio_free(pdata->set1);
	if (pdata->set2)
		gpio_free(pdata->set2);
	if (pdata->set3)
		gpio_free(pdata->set3);

	kfree(max8698->rdev);
	kfree(max8698);
	i2c_set_clientdata(client, NULL);
	return 0;
}

static const struct i2c_device_id max8698_ids[] = {
	{ "max8698", 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, max8698_ids);

static struct i2c_driver max8698_pmic_driver = {
	.probe		= max8698_pmic_probe,
	.remove		= __devexit_p(max8698_pmic_remove),
	.driver		= {
		.name	= "max8698",
	},
	.id_table	= max8698_ids,
};

static int __init max8698_pmic_init(void)
{
	return i2c_add_driver(&max8698_pmic_driver);
}
module_init(max8698_pmic_init);

static void __exit max8698_pmic_exit(void)
{
	i2c_del_driver(&max8698_pmic_driver);
};
module_exit(max8698_pmic_exit);

MODULE_DESCRIPTION("MAXIM 8698 voltage regulator driver");
MODULE_AUTHOR("Kyungmin Park <kyungmin.park@samsung.com>");
MODULE_LICENSE("GPL");
