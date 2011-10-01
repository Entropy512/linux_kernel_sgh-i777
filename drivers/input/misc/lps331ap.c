/*
 * Copyright (C) 2011 Samsung Electronics. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/lps331ap.h>

/* device id */
#define DEVICE_ID		0xBB

/* Register definitions */
#define REF_P_XL		0x08
#define REF_P_L			0x09
#define REF_P_H			0x0A
#define REF_T_L			0x0B
#define REF_T_H			0x0C
#define WHO_AM_I		0x0F
#define RES_CONF		0x10
#define CTRL_REG1		0x20
#define CTRL_REG2		0x21
#define CTRL_REG3		0x22
#define INT_CFG_REG		0x23
#define INT_SOURCE_REG		0x24
#define THS_P_LOW_REG		0x25
#define THS_P_HIGH_REG		0x26
#define STATUS_REG		0x27
#define PRESS_POUT_XL_REH	0x28
#define PRESS_OUT_L		0x29
#define PRESS_OUT_H		0x2A
#define TEMP_OUT_L		0x2B
#define TEMP_OUT_H		0x2C
#define AUTO_INCREMENT		0x80

/* poll delays */
#define DELAY_LOWBOUND		(10 * NSEC_PER_MSEC)
#define DELAY_DEFAULT		(200 * NSEC_PER_MSEC)

/* pressure and temperature min, max ranges */
#define PRESSURE_MAX		(1260 * 4096)
#define PRESSURE_MIN		(260 * 4096)
#define TEMP_MAX		(300 * 480)
#define TEMP_MIN		(-300 * 480)

struct barometer_data {
	struct i2c_client *client;
	struct mutex lock;
	struct workqueue_struct *wq;
	struct work_struct work_pressure;
	struct input_dev *input_dev;
	struct hrtimer timer;
	struct completion data_ready;
	struct lps331ap_platform_data *pdata;
	bool enabled;
	int temperature;
	int pressure;
	int irq;
	ktime_t poll_delay;
};

static const char default_regs[] = {
	0x84, /* CTRL_REG1: active mode, block data update */
	0x01, /* CTRL_REG2: one shot */
	0x24, /* CTRL_REG3: data ready for interrupt 1 and 2 */
	0x6A, /* REG_CONF : higher precision */
};

static void barometer_enable(struct barometer_data *barom)
{
	if (!barom->enabled) {
		barom->enabled = true;
		hrtimer_start(&barom->timer, barom->poll_delay,
			HRTIMER_MODE_REL);
	}
}

static void barometer_disable(struct barometer_data *barom)
{
	int err;

	if (barom->enabled) {
		barom->enabled = false;
		hrtimer_cancel(&barom->timer);
		cancel_work_sync(&barom->work_pressure);
	}

	/* set power-down mode */
	err = i2c_smbus_write_byte_data(barom->client, CTRL_REG1, 0x00);
	if (err < 0)
		pr_err("%s: power-down failed\n", __func__);
}

static int baromter_wait_for_data_ready(struct barometer_data *barom)
{
	int err;

	enable_irq(barom->irq);
	err = wait_for_completion_timeout(&barom->data_ready, 3*HZ);
	disable_irq(barom->irq);

	if (err > 0)
		return 0;
	else if (err == 0) {
		pr_err("%s: wait timed out\n", __func__);
		return -ETIMEDOUT;
	}

	return err;
}

static int barometer_read_pressure_data(struct barometer_data *barom)
{
	int err = 0;
	u8 temp_data[2];
	u8 press_data[3];

	/* To read pressure data,
	 * you are supposed to follow the below 7 steps every time
	 * recommended by STMicroelectronics */

	/* 1. set power-down */
	err = i2c_smbus_write_byte_data(barom->client,
					CTRL_REG1, 0x00);
	if (err < 0) {
		pr_err("%s: power-down failed\n", __func__);
		goto done;
	}

	/* 2. set higher precision */
	err = i2c_smbus_write_byte_data(barom->client,
					RES_CONF, default_regs[3]);
	if (err < 0) {
		pr_err("%s: higher precision failed\n", __func__);
		goto done;
	}

	/* 3. set active mode */
	err = i2c_smbus_write_byte_data(barom->client,
			CTRL_REG1, default_regs[0]);
	if (err < 0) {
		pr_err("%s: active mode failed\n", __func__);
		goto done;
	}

	/* 4. trigger one shot */
	err = i2c_smbus_write_byte_data(barom->client,
			CTRL_REG2, default_regs[1]);
	if (err < 0) {
		pr_err("%s: one shot trigger failed\n", __func__);
		goto done;
	}

	/* 5. wait for data ready */
	if (barom->pdata->irq >= 0) {
		/* wait for data ready intterupt */
		err = baromter_wait_for_data_ready(barom);
		if (err < 0) {
			pr_err("%s: data not ready\n", __func__);
			goto done;
		}
	} else {
		/* just wait for a certain time */
		msleep(10);
	}

	/* 6. read temperature data */
	err = i2c_smbus_read_i2c_block_data(barom->client,
			TEMP_OUT_L | AUTO_INCREMENT,
			sizeof(temp_data), temp_data);
	if (err < 0) {
		pr_err("%s: TEMP_OUT i2c reading failed\n", __func__);
		goto done;
	} else
		barom->temperature = (temp_data[1] << 8) | temp_data[0];

	/* 7. read pressure data */
	err = i2c_smbus_read_i2c_block_data(barom->client,
			PRESS_POUT_XL_REH | AUTO_INCREMENT,
			sizeof(press_data), press_data);
	if (err < 0) {
		pr_err("%s: PRESS_OUT i2c reading failed\n", __func__);
		goto done;
	} else
		barom->pressure = (press_data[2] << 16)
				| (press_data[1] << 8) | press_data[0];

done:
	return err;
}

static void barometer_get_pressure_data(struct work_struct *work)
{
	struct barometer_data *barom =
		container_of(work, struct barometer_data, work_pressure);
	int err;

	err = barometer_read_pressure_data(barom);
	if (err < 0) {
		pr_err("%s: read pressure data failed\n", __func__);
		return;
	}

	/* report pressure amd temperature values */
	input_report_abs(barom->input_dev, ABS_MISC, barom->temperature);
	input_report_abs(barom->input_dev, ABS_PRESSURE, barom->pressure);
	input_sync(barom->input_dev);
}

static enum hrtimer_restart barometer_timer_func(struct hrtimer *timer)
{
	struct barometer_data *barom =
		container_of(timer, struct barometer_data, timer);

	queue_work(barom->wq, &barom->work_pressure);
	hrtimer_forward_now(&barom->timer, barom->poll_delay);
	return HRTIMER_RESTART;
}

static ssize_t barometer_poll_delay_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct barometer_data *barom = dev_get_drvdata(dev);
	return sprintf(buf, "%lld\n", ktime_to_ns(barom->poll_delay));
}

static ssize_t barometer_poll_delay_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	int err;
	int64_t new_delay;
	struct barometer_data *barom = dev_get_drvdata(dev);

	err = strict_strtoll(buf, 10, &new_delay);
	if (err < 0)
		return err;

	pr_debug("%s: new delay = %lldns, old delay = %lldns\n",
		__func__, new_delay, ktime_to_ns(barom->poll_delay));

	if (new_delay < DELAY_LOWBOUND)
		new_delay = DELAY_LOWBOUND;

	mutex_lock(&barom->lock);
	if (new_delay != ktime_to_ns(barom->poll_delay)) {
		hrtimer_cancel(&barom->timer);
		barom->poll_delay = ns_to_ktime(new_delay);
		hrtimer_start(&barom->timer, barom->poll_delay,
			HRTIMER_MODE_REL);

	}
	mutex_unlock(&barom->lock);

	return size;
}

static ssize_t barometer_enable_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct barometer_data *barom = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", barom->enabled);
}

static ssize_t barometer_enable_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t size)
{
	bool new_value;
	struct barometer_data *barom = dev_get_drvdata(dev);

	pr_debug("%s: enable %s\n", __func__, buf);

	if (sysfs_streq(buf, "1")) {
		new_value = true;
	} else if (sysfs_streq(buf, "0")) {
		new_value = false;
	} else {
		pr_err("%s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	pr_debug("%s: new_value = %d, old state = %d\n",
		__func__, new_value, barom->enabled);

	mutex_lock(&barom->lock);
	if (new_value)
		barometer_enable(barom);
	else
		barometer_disable(barom);
	mutex_unlock(&barom->lock);

	return size;
}


static DEVICE_ATTR(poll_delay, S_IRUGO | S_IWUSR | S_IWGRP,
		barometer_poll_delay_show, barometer_poll_delay_store);

static DEVICE_ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
		barometer_enable_show, barometer_enable_store);

static struct attribute *barometer_sysfs_attrs[] = {
	&dev_attr_enable.attr,
	&dev_attr_poll_delay.attr,
	NULL
};

static struct attribute_group barometer_attribute_group = {
	.attrs = barometer_sysfs_attrs,
};

static irqreturn_t barometer_irq_handler(int irq, void *data)
{
	struct barometer_data *barom = data;
	complete(&barom->data_ready);
	return IRQ_HANDLED;
}

static int setup_input_device(struct barometer_data *barom)
{
	int err;

	barom->input_dev = input_allocate_device();
	if (!barom->input_dev) {
		pr_err("%s: could not allocate input device\n", __func__);
		return -ENOMEM;
	}

	input_set_drvdata(barom->input_dev, barom);
	barom->input_dev->name = "barometer_sensor";

	/* temperature */
	input_set_capability(barom->input_dev, EV_ABS, ABS_MISC);
	input_set_abs_params(barom->input_dev, ABS_MISC,
				TEMP_MIN, TEMP_MAX, 0, 0);

	/* pressure */
	input_set_capability(barom->input_dev, EV_ABS, ABS_PRESSURE);
	input_set_abs_params(barom->input_dev, ABS_PRESSURE,
				PRESSURE_MIN, PRESSURE_MAX, 0, 0);

	err = input_register_device(barom->input_dev);
	if (err) {
		pr_err("%s: unable to register input polled device %s\n",
			__func__, barom->input_dev->name);
		goto err_register_device;
	}

	goto done;

err_register_device:
	input_free_device(barom->input_dev);
done:
	return err;
}

static int setup_irq(struct barometer_data *barom)
{
	struct lps331ap_platform_data *pdata = barom->pdata;
	int err = 0;

	/* if there's no interrupt pin */
	if (pdata->irq < 0) {
		pr_debug("%s: no intterupt pin\n", __func__);
		goto done;
	}

	/* enable interrupt mode */
	err = i2c_smbus_write_byte_data(barom->client,
			CTRL_REG3, default_regs[2]);
	if (err < 0) {
		pr_err("%s: setting interrupt failed\n", __func__);
		goto done;
	}

	/* setup gpio irq */
	err = gpio_request(pdata->irq, "gpio_barometer_int");
	if (err < 0) {
		pr_err("%s: gpio %d request failed (%d)\n",
			__func__, pdata->irq, err);
		goto done;
	}

	barom->irq = gpio_to_irq(pdata->irq);

	err = request_irq(barom->irq, barometer_irq_handler,
			IRQF_TRIGGER_RISING,
			 "barometer_int", barom);
	if (err < 0) {
		pr_err("%s: request_irq(%d) failed (%d)\n",
			__func__, barom->irq, err);
		goto err_request_irq;
	}

	disable_irq(barom->irq);

	goto done;

err_request_irq:
	gpio_free(pdata->irq);
done:
	return err;
}

static int __devinit barometer_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int err, ret;
	struct barometer_data *barom;

	/* initialize barometer_data */
	barom = kzalloc(sizeof(*barom), GFP_KERNEL);
	if (!barom) {
		pr_err("%s: failed to allocate memory for module\n", __func__);
		err = -ENOMEM;
		goto exit;
	}

	/* i2c function check */
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("%s: client not i2c capable\n", __func__);
		err = -EIO;
		goto err_i2c_check;
	}

	/* read chip id */
	ret = i2c_smbus_read_byte_data(client, WHO_AM_I);
	if (ret != DEVICE_ID) {
		if (ret < 0) {
			pr_err("%s: i2c for reading chip id failed\n",
								__func__);
			err = ret;
		} else {
			pr_err("%s : Device identification failed\n",
								__func__);
			err = -ENODEV;
		}
		goto err_device_id;
	}

	/* setup i2c client, platform data, mutex and completion */
	barom->client = client;
	i2c_set_clientdata(client, barom);
	barom->pdata = client->dev.platform_data;
	mutex_init(&barom->lock);
	init_completion(&barom->data_ready);

	/* setup timer and workqueue */
	hrtimer_init(&barom->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	barom->poll_delay = ns_to_ktime(DELAY_DEFAULT);
	barom->timer.function = barometer_timer_func;

	barom->wq = create_singlethread_workqueue("barometer_wq");
	if (!barom->wq) {
		err = -ENOMEM;
		pr_err("%s: could not create workqueue\n", __func__);
		goto err_create_workqueue;
	}

	INIT_WORK(&barom->work_pressure, barometer_get_pressure_data);

	/* setup input device */
	err = setup_input_device(barom);
	if (err) {
		pr_err("%s: setup nput device failed\n", __func__);
		goto err_setup_input_device;
	}

	/* setup interrupt */
	err = setup_irq(barom);
	if (err < 0) {
		pr_err("%s: setup irq failed\n", __func__);
		goto err_setup_irq;
	}

	/* create sysfs(enable, poll_delay) */
	err = sysfs_create_group(&barom->input_dev->dev.kobj,
				&barometer_attribute_group);
	if (err) {
		pr_err("%s: could not create sysfs group\n", __func__);
		goto err_sysfs_create_group;
	}

	return 0;

err_sysfs_create_group:
	if (barom->pdata->irq >= 0) {
		if (!barom->enabled)
			enable_irq(barom->irq);
		free_irq(barom->irq, barom);
		gpio_free(barom->pdata->irq);
	}
err_setup_irq:
	input_unregister_device(barom->input_dev);
err_setup_input_device:
	destroy_workqueue(barom->wq);
err_create_workqueue:
	mutex_destroy(&barom->lock);
err_device_id:
err_i2c_check:
	kfree(barom);
exit:
	return err;
}

static int __devexit barometer_remove(struct i2c_client *client)
{
	struct barometer_data *barom = i2c_get_clientdata(client);

	barometer_disable(barom);
	sysfs_remove_group(&barom->input_dev->dev.kobj,
				&barometer_attribute_group);
	if (barom->pdata->irq >= 0) {
		if (!barom->enabled)
			enable_irq(barom->irq);
		free_irq(barom->irq, barom);
		gpio_free(barom->pdata->irq);
	}
	input_unregister_device(barom->input_dev);
	hrtimer_cancel(&barom->timer);
	cancel_work_sync(&barom->work_pressure);
	destroy_workqueue(barom->wq);
	mutex_destroy(&barom->lock);
	kfree(barom);

	return 0;
}

static int barometer_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct barometer_data *barom = i2c_get_clientdata(client);

	if (barom->enabled)
		barometer_enable(barom);
	return 0;
}

static int barometer_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct barometer_data *barom = i2c_get_clientdata(client);

	barometer_disable(barom);
	return 0;
}

static const struct i2c_device_id barometer_id[] = {
	{"lps331ap", 0},
	{},
};

MODULE_DEVICE_TABLE(i2c, barometer_id);
static const struct dev_pm_ops barometer_pm_ops = {
	.suspend	= barometer_suspend,
	.resume		= barometer_resume,
};

static struct i2c_driver barometer_driver = {
	.driver = {
		.name	= "lps331ap",
		.owner	= THIS_MODULE,
		.pm	= &barometer_pm_ops,
	},
	.probe		= barometer_probe,
	.remove		= __devexit_p(barometer_remove),
	.id_table	= barometer_id,
};

static int __init barometer_init(void)
{
	return i2c_add_driver(&barometer_driver);
}

static void __exit barometer_exit(void)
{
	i2c_del_driver(&barometer_driver);
}
module_init(barometer_init);
module_exit(barometer_exit);

MODULE_AUTHOR("Seulki Lee <tim.sk.lee@samsung.com>");
MODULE_DESCRIPTION("LPS331AP digital output barometer");
MODULE_LICENSE("GPL");
