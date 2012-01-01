/*
 *  wacom_i2c.c - Wacom G5 Digitizer Controller (I2C bus)
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/wacom_i2c.h>
#include <linux/earlysuspend.h>
#include <linux/uaccess.h>

#include "wacom_i2c_func.h"
#include "wacom_i2c_flash.h"

#define WACOM_FW_PATH "/sdcard/firmware/wacom_firm.bin"
#define INIT_FIRMWARE_FLASH

/*sec_class sysfs*/
extern struct class *sec_class;
struct device *sec_epen;

struct i2c_client *g_client;


int wacom_i2c_load_fw(struct i2c_client *client)
{
	struct file *fp;
	mm_segment_t old_fs;
	long fsize, nread;
	int ret;
	struct wacom_i2c *wac_i2c = i2c_get_clientdata(client);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(WACOM_FW_PATH, O_RDONLY, S_IRUSR);

	if (IS_ERR(fp)) {
		printk(KERN_ERR "[E-PEN]: failed to open %s.\n", WACOM_FW_PATH);
		goto open_err;
	}

	fsize = fp->f_path.dentry->d_inode->i_size;
	printk(KERN_NOTICE "[E-PEN]: start, file path %s, size %ld Bytes\n", WACOM_FW_PATH, fsize);

	nread = vfs_read(fp, (char __user *)Binary, fsize, &fp->f_pos);
	printk(KERN_ERR "[E-PEN]: nread %ld Bytes\n", nread);
	if (nread != fsize) {
		printk(KERN_ERR "[E-PEN]: failed to read firmware file, nread %ld Bytes\n", nread);
		goto read_err;
	}

	ret = wacom_i2c_flash(wac_i2c);
	if (ret < 0) {
		printk(KERN_ERR "[E-PEN]: failed to write firmware(%d)\n", ret);
		goto fw_write_err;
	}

	filp_close(fp, current->files);
	set_fs(old_fs);

	return 0;

open_err :
	set_fs(old_fs);
	return -ENOENT;

read_err :
	filp_close(fp, current->files);
	set_fs(old_fs);
	return -EIO;

fw_write_err :
	filp_close(fp, current->files);
	set_fs(old_fs);
	return -1;
}

int wacom_i2c_frequency(char buf)
{
	int ret;
	if (g_client == NULL) {
		printk(KERN_DEBUG "[E-PEN]: failed to modify the frequency\n");
		return 0;
	}
	ret = i2c_master_send(g_client, &buf, sizeof(buf));
	if (ret > 0) {
		printk(KERN_INFO "[E-PEN]: buf:%d, sent:%d\n", buf, ret);
	} else {
		printk(KERN_ERR "[E-PEN]: Digitizer is not active\n");
		return -1;
	}
	return 0;
}
EXPORT_SYMBOL(wacom_i2c_frequency);

static irqreturn_t wacom_interrupt(int irq, void *dev_id)
{
	struct wacom_i2c *wac_i2c = dev_id;
	wacom_i2c_coord(wac_i2c);
	return IRQ_HANDLED;
}

static void wacom_i2c_set_input_values(struct i2c_client *client,
				struct wacom_i2c *wac_i2c,
				struct input_dev *input_dev)
{
	/*Set input values before registering input device*/

	input_dev->name = "sec_e-pen";
	input_dev->id.bustype = BUS_I2C;
	input_dev->dev.parent = &client->dev;
	input_dev->evbit[0] |= BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);

	__set_bit(ABS_X, input_dev->absbit);
	__set_bit(ABS_Y, input_dev->absbit);
	__set_bit(ABS_PRESSURE, input_dev->absbit);
	__set_bit(BTN_TOUCH, input_dev->keybit);
	__set_bit(EPEN_TOOL_PEN, input_dev->keybit);
	__set_bit(EPEN_TOOL_RUBBER, input_dev->keybit);
	__set_bit(EPEN_STYLUS, input_dev->keybit);
	__set_bit(KEY_UNKNOWN, input_dev->keybit);
	/*  __set_bit(BTN_STYLUS2, input_dev->keybit); */
	/*  __set_bit(ABS_MISC, input_dev->absbit); */
}

static int wacom_check_emr_prox(struct wacom_g5_callbacks *cb)
{
	struct wacom_i2c *wac = container_of(cb, struct wacom_i2c, callbacks);
	printk(KERN_DEBUG "[E-PEN]:%s:\n", __func__);

	return wac->pen_prox;
}



static int wacom_i2c_remove(struct i2c_client *client)
{
	struct wacom_i2c *wac_i2c = i2c_get_clientdata(client);
	free_irq(client->irq, wac_i2c);
	input_unregister_device(wac_i2c->input_dev);
	kfree(wac_i2c);

	return 0;
}

static void wacom_i2c_early_suspend(struct early_suspend *h)
{
	struct wacom_i2c *wac_i2c = container_of(h, struct wacom_i2c, early_suspend);
	disable_irq(wac_i2c->client->irq);
	/* release pen, if it is pressed*/

	wac_i2c->wac_pdata->early_suspend_platform_hw();
	printk(KERN_DEBUG "[E-PEN]:%s.\n", __func__);
}

static void wacom_i2c_late_resume(struct early_suspend *h)
{
	struct wacom_i2c *wac_i2c = container_of(h, struct wacom_i2c, early_suspend);
	wac_i2c->wac_pdata->late_resume_platform_hw();
	enable_irq(wac_i2c->client->irq);
	printk(KERN_DEBUG "[E-PEN]:%s.\n", __func__);
}

#ifdef CONFIG_HAS_EARLYSUSPEND
#define wacom_i2c_suspend	NULL
#define wacom_i2c_resume	NULL

#else
static int wacom_i2c_suspend(struct i2c_client *client, pm_message_t mesg)
{
	wacom_i2c_early_suspend();
	printk(KERN_DEBUG "[E-PEN]:%s.\n", __func__);
	return 0;
}

static int wacom_i2c_resume(struct i2c_client *client)
{
	wacom_i2c_late_resume();
	enable_irq(wac_i2c->client->irq);
	printk(KERN_DEBUG "[E-PEN]:%s.\n", __func__);
	return 0;
}
#endif


static ssize_t epen_firm_update_status_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);

	printk(KERN_DEBUG "[E-PEN]:%s:(%d)\n", __func__, wac_i2c->wac_feature->firm_update_status);

	if (wac_i2c->wac_feature->firm_update_status == 2) {
		return sprintf(buf, "PASS\n");
	} else if (wac_i2c->wac_feature->firm_update_status == 1) {
		return sprintf(buf, "DOWNLOADING\n");
	} else if (wac_i2c->wac_feature->firm_update_status == -1) {
		return sprintf(buf, "FAIL\n");
	} else {
		return 0;
	}

}

static ssize_t epen_firm_version_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	printk(KERN_DEBUG "[E-PEN]:%s: 0x%x|0x%X\n", __func__, wac_i2c->wac_feature->fw_version, Firmware_version_of_file);

	return sprintf(buf, "0x%X\t0x%X\n", wac_i2c->wac_feature->fw_version, Firmware_version_of_file);
}

static ssize_t epen_firmware_update_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	u8 *data;
	u8 buf_if;
	int ret;

	buf_if = COM_QUERY;
	data = wac_i2c->wac_feature->data;

	printk(KERN_DEBUG "[E-PEN]:%s:\n", __func__);

	disable_irq(wac_i2c->irq);

	wac_i2c->wac_feature->firm_update_status = 1;

	if (*buf == 'F') {
		printk(KERN_ERR "[E-PEN]: Satrt firmware flashing (UMS).\n");
		ret = wacom_i2c_load_fw(wac_i2c->client);
		if (ret < 0) {
			printk(KERN_ERR "[E-PEN]: failed to flash firmware.\n");
			goto failure;
		}
	} else if (*buf == 'B') {
		printk(KERN_ERR "[E-PEN]: Satrt firmware flashing (kernel image).\n");
		ret = wacom_i2c_flash(wac_i2c);
		if (ret < 0) {
			printk(KERN_ERR "[E-PEN]: failed to flash firmware.\n");
			goto failure;
		}
	} else {
		printk(KERN_ERR "[E-PEN]: wrong parameter.\n");
		goto param_err;
	}
	printk(KERN_ERR "[E-PEN]: Finish firmware flashing.\n");

	msleep(1000);

	wacom_i2c_query(wac_i2c);

	wac_i2c->wac_feature->firm_update_status = 2;

	enable_irq(wac_i2c->irq);


	return count;

param_err:

failure:
	wac_i2c->wac_feature->firm_update_status = -1;
	enable_irq(wac_i2c->irq);
	return -1;

}
static DEVICE_ATTR(epen_firm_update, S_IWUSR|S_IWGRP, NULL, epen_firmware_update_store);		/* firmware update */
static DEVICE_ATTR(epen_firm_update_status, S_IRUGO, epen_firm_update_status_show, NULL);		/* return firmware update status*/
static DEVICE_ATTR(epen_firm_version, S_IRUGO, epen_firm_version_show, NULL);					/* return firmware version */

/*
static struct attribute *w8501_attributes[] = {
	&dev_attr_w8501_firmware_in_binary,
	NULL
};
*/

static int wacom_i2c_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	struct wacom_i2c *wac_i2c;
	int i, ret;
	i = ret = 0;

	printk(KERN_DEBUG "[E-PEN]:%s:\n", __func__);

	/*Check I2C functionality*/
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
		goto err3;

	/*Obtain kernel memory space for wacom i2c*/
	wac_i2c = kzalloc(sizeof(struct wacom_i2c), GFP_KERNEL);
	wac_i2c->wac_feature = &wacom_feature_EMR;

	/*Initializing for semaphor*/
	mutex_init(&wac_i2c->lock);

	/*Register platform data*/
	wac_i2c->wac_pdata = client->dev.platform_data;

	/*Register callbacks*/
	wac_i2c->callbacks.check_prox = wacom_check_emr_prox;
	if (wac_i2c->wac_pdata->register_cb)
		wac_i2c->wac_pdata->register_cb(&wac_i2c->callbacks);

	/*Register wacom i2c to input device*/
	wac_i2c->input_dev = input_allocate_device();
	if (wac_i2c == NULL || wac_i2c->input_dev == NULL)
		goto fail;
	wacom_i2c_set_input_values(client, wac_i2c, wac_i2c->input_dev);

	wac_i2c->client = client;
	wac_i2c->irq = client->irq;

	/*Change below if irq is needed*/
	wac_i2c->irq_flag = 1;

#ifdef CONFIG_HAS_EARLYSUSPEND
		wac_i2c->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
		wac_i2c->early_suspend.suspend = wacom_i2c_early_suspend;
		wac_i2c->early_suspend.resume = wacom_i2c_late_resume;
		register_early_suspend(&wac_i2c->early_suspend);
#endif

#if defined(INIT_FIRMWARE_FLASH)
	wacom_i2c_query(wac_i2c);
	printk(KERN_NOTICE "[E-PEN] wacom fw ver : %x, new fw ver : %x\n",
		wac_i2c->wac_feature->fw_version, Firmware_version_of_file);
	if( wac_i2c->wac_feature->fw_version < Firmware_version_of_file )
	{
		printk(KERN_NOTICE "[E-PEN]: INIT_FIRMWARE_FLASH is enabled.\n");
		ret = wacom_i2c_flash(wac_i2c);
		printk(KERN_ERR "[E-PEN]: flashed.(%d)\n", ret);
	}
#endif

	wacom_i2c_query(wac_i2c);

	if (wac_i2c->wac_pdata->xy_switch) {
		input_set_abs_params(wac_i2c->input_dev, ABS_X, 0,
			wac_i2c->wac_feature->y_max, 4, 0);
		input_set_abs_params(wac_i2c->input_dev, ABS_Y, 0,
			wac_i2c->wac_feature->x_max, 4, 0);
	} else {
		input_set_abs_params(wac_i2c->input_dev, ABS_X, 0,
			wac_i2c->wac_feature->x_max, 4, 0);
		input_set_abs_params(wac_i2c->input_dev, ABS_Y, 0,
			wac_i2c->wac_feature->y_max, 4, 0);
	}
	input_set_abs_params(wac_i2c->input_dev, ABS_PRESSURE, 0,
			wac_i2c->wac_feature->pressure_max, 0, 0);
	input_set_drvdata(wac_i2c->input_dev, wac_i2c);

	/*Set client data*/
	i2c_set_clientdata(client, wac_i2c);

	/*Before registering input device, data in each input_dev must be set*/
	if (input_register_device(wac_i2c->input_dev))
		goto err2;

	g_client = client;

	/*  if(wac_i2c->irq_flag) */
	/*   disable_irq(wac_i2c->irq); */

	sec_epen= device_create(sec_class, NULL, 0, NULL, "sec_epen");
	dev_set_drvdata(sec_epen, wac_i2c);

	if (IS_ERR(sec_epen))
			printk(KERN_ERR "Failed to create device(sec_epen)!\n");

	if (device_create_file(sec_epen, &dev_attr_epen_firm_update)< 0)
			printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_epen_firm_update.attr.name);

	if (device_create_file(sec_epen, &dev_attr_epen_firm_update_status)< 0)
			printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_epen_firm_update_status.attr.name);

	if (device_create_file(sec_epen, &dev_attr_epen_firm_version)< 0)
			printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_epen_firm_version.attr.name);


	/*Request IRQ*/
	if (wac_i2c->irq_flag) {
		ret = request_threaded_irq(wac_i2c->irq, NULL, wacom_interrupt, IRQF_DISABLED|IRQF_TRIGGER_HIGH|IRQF_ONESHOT, wac_i2c->name, wac_i2c);
		if (ret < 0)
			goto err1;
	}

	return 0;

err3:
	printk(KERN_ERR "[E-PEN]: No I2C functionality found\n");
	return -ENODEV;

err2:
	printk(KERN_ERR "[E-PEN]: err2 occured\n");
	input_free_device(wac_i2c->input_dev);
	return -EIO;

err1:
	printk(KERN_ERR "[E-PEN]: err1 occured(num:%d)\n", ret);
	input_free_device(wac_i2c->input_dev);
	wac_i2c->input_dev = NULL;
	return -EIO;

fail:
	printk(KERN_ERR "[E-PEN]: fail occured\n");
	return -ENOMEM;
}



static const struct i2c_device_id wacom_i2c_id[] = {
	{"wacom_g5sp_i2c", 0},
	{},
};

/*Create handler for wacom_i2c_driver*/
static struct i2c_driver wacom_i2c_driver = {
	.driver = {
		.name = "wacom_g5sp_i2c",
	},
	.probe = wacom_i2c_probe,
	.remove = wacom_i2c_remove,
	.suspend = wacom_i2c_suspend,
	.resume = wacom_i2c_resume,
	.id_table = wacom_i2c_id,
};

static int __init wacom_i2c_init(void)
{
	int ret = 0;

#if defined(WACOM_SLEEP_WITH_PEN_SLP)
	printk(KERN_ERR "[E-PEN]: %s: Sleep type-PEN_SLP pin\n", __func__);
#elif defined(WACOM_SLEEP_WITH_PEN_LDO_EN)
	printk(KERN_ERR "[E-PEN]: %s: Sleep type-PEN_LDO_EN pin\n", __func__);
#endif

	ret = i2c_add_driver(&wacom_i2c_driver);
	if (ret)
		printk(KERN_ERR "[E-PEN]: fail to i2c_add_driver\n");
	return ret;

}

static void __exit wacom_i2c_exit(void)
{
	i2c_del_driver(&wacom_i2c_driver);
}


late_initcall(wacom_i2c_init);
module_exit(wacom_i2c_exit);

MODULE_AUTHOR("Samsung");
MODULE_DESCRIPTION("Driver for Wacom G5SP Digitizer Controller");

MODULE_LICENSE("GPL");
