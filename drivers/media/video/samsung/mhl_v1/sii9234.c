/***************************************************************************

*

*   SiI9244 ? MHL Transmitter Driver

*

* Copyright (C) (2011, Silicon Image Inc)

*

* This program is free software; you can redistribute it and/or modify

* it under the terms of the GNU General Public License as published by

* the Free Software Foundation version 2.

*

* This program is distributed ¡°as is¡± WITHOUT ANY WARRANTY of any

* kind, whether express or implied; without even the implied warranty

* of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the

* GNU General Public License for more details.

*

*****************************************************************************/


#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <plat/pm.h>
#include <asm/irq.h>
#include <linux/delay.h>
#include <plat/gpio-cfg.h>
#include <plat/gpio-core.h>
#include <mach/regs-gpio.h>
#include <mach/regs-clock.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <asm/uaccess.h>
#include <linux/regulator/consumer.h>
#include <linux/mfd/max8998.h>
#include <linux/mfd/max8997.h>


#include "sii9234_driver.h"
#include "Common_Def.h"


#define SUBJECT "MHL_DRIVER"

#define SII_DEV_DBG(format,...)\
	printk ("[ "SUBJECT " (%s,%d) ] " format "\n", __func__, __LINE__, ## __VA_ARGS__);

struct work_struct sii9234_int_work;
struct workqueue_struct *sii9234_wq = NULL;

struct i2c_driver sii9234_i2c_driver;
struct i2c_client *sii9234_i2c_client = NULL;

struct i2c_driver sii9234a_i2c_driver;
struct i2c_client *sii9234a_i2c_client = NULL;

struct i2c_driver sii9234b_i2c_driver;
struct i2c_client *sii9234b_i2c_client = NULL;

struct i2c_driver sii9234c_i2c_driver;
struct i2c_client *sii9234c_i2c_client = NULL;

extern bool sii9234_init(void);

static struct i2c_device_id sii9234_id[] = {
	{"SII9234", 0},
	{}
};

static struct i2c_device_id sii9234a_id[] = {
	{"SII9234A", 0},
	{}
};

static struct i2c_device_id sii9234b_id[] = {
	{"SII9234B", 0},
	{}
};

static struct i2c_device_id sii9234c_id[] = {
	{"SII9234C", 0},
	{}
};

int MHL_i2c_init = 0;


struct sii9234_state {
	struct i2c_client *client;
};

void sii9234_cfg_power(bool on);

static void sii9234_cfg_gpio(void);

irqreturn_t mhl_int_irq_handler(int irq, void *dev_id);

irqreturn_t mhl_wake_up_irq_handler(int irq, void *dev_id);

void sii9234_interrupt_event_work(struct work_struct *p);

#define MHL_SWITCH_TEST	1

#ifdef MHL_SWITCH_TEST
struct class *sec_mhl;
EXPORT_SYMBOL(sec_mhl);

struct device *mhl_switch;
EXPORT_SYMBOL(mhl_switch);

static ssize_t check_MHL_command(struct device *dev, struct device_attribute *attr, char *buf)
{
	int count;
	int res;

	printk(KERN_ERR "[MHL]: check_MHL_command\n");
	sii9234_cfg_power(1);
	res = SiI9234_startTPI();
	count = sprintf(buf,"%d\n", res );
	sii9234_cfg_power(0);
	return count;

}

static ssize_t change_switch_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	char *after;
	unsigned long value = simple_strtoul(buf, &after, 10);
	int i;
	printk(KERN_ERR "[MHL_SWITCH] Change the switch: %ld\n", value);

	if (value == 0) {
		for (i = 0; i <20; i++) {
			printk(KERN_ERR "[MHL] try %d\n", i+1);
			msleep(500);
		}
		s3c_gpio_cfgpin(GPIO_MHL_INT, GPIO_MHL_INT_AF);
		s3c_gpio_setpull(GPIO_MHL_SEL, S3C_GPIO_PULL_UP);
		gpio_set_value(GPIO_MHL_SEL, GPIO_LEVEL_HIGH);
		sii9234_cfg_power(1);
		sii9234_init();
	} else {
		sii9234_cfg_power(0);
		s3c_gpio_setpull(GPIO_MHL_SEL, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_MHL_SEL, GPIO_LEVEL_LOW);
	}
	return size;
}

void MHL_On(bool on)
{
	printk("[MHL] USB path change : %d\n", on);
	if (on == 1) {
		if(gpio_get_value(GPIO_MHL_SEL))
			printk("[MHL] GPIO_MHL_SEL : already 1\n");
		else {
			gpio_set_value(GPIO_MHL_SEL, GPIO_LEVEL_HIGH);
			sii9234_cfg_power(1);
			sii9234_init();
		}
	} else {
		if(!gpio_get_value(GPIO_MHL_SEL))
			printk("[MHL] GPIO_MHL_SEL : already 0\n");
		else {
			sii9234_cfg_power(0);
			gpio_set_value(GPIO_MHL_SEL, GPIO_LEVEL_LOW);
		}
	}

}
EXPORT_SYMBOL(MHL_On);


static DEVICE_ATTR(mhl_sel, S_IRUGO | S_IWUSR | S_IXOTH, check_MHL_command, change_switch_store);
#endif

static ssize_t MHD_check_read(struct device *dev, struct device_attribute *attr, char *buf)
{
	int count;
	int res = 0;
	#if 0

	s3c_gpio_setpull(GPIO_MHL_SEL, S3C_GPIO_PULL_UP);	//MHL_SEL

	gpio_set_value(GPIO_MHL_SEL, GPIO_LEVEL_HIGH);


	//TVout_LDO_ctrl(true);

	if(!MHD_HW_IsOn())
	{
		sii9234_tpi_init();
		res = MHD_Read_deviceID();
		MHD_HW_Off();
	}
	else
	{
		sii9234_tpi_init();
		res = MHD_Read_deviceID();
	}

	I2C_WriteByte(0x72, 0xA5, 0xE1);
	res = 0;
	res = I2C_ReadByte(0x72, 0xA5);

	printk(KERN_ERR "A5 res %x",res);

	res = 0;
	res = I2C_ReadByte(0x72, 0x1B);

	printk(KERN_ERR "Device ID res %x",res);

	res = 0;
	res = I2C_ReadByte(0x72, 0x1C);

	printk(KERN_ERR "Device Rev ID res %x",res);

	res = 0;
	res = I2C_ReadByte(0x72, 0x1D);

	printk(KERN_ERR "Device Reserved ID res %x",res);

	printk(KERN_ERR "\n####HDMI_EN1 %x MHL_RST %x GPIO_MHL_SEL %x\n",gpio_get_value(GPIO_HDMI_EN),gpio_get_value(GPIO_MHL_RST),gpio_get_value(GPIO_MHL_SEL));

	res = I2C_ReadByte(0x7A, 0x3D);

	res = I2C_ReadByte(0x7A, 0xFF);

	s3c_gpio_setpull(GPIO_MHL_SEL, S3C_GPIO_PULL_NONE);	//MHL_SEL

	gpio_set_value(GPIO_MHL_SEL, GPIO_LEVEL_LOW);
#endif
	count = sprintf(buf,"%d\n", res );
	//TVout_LDO_ctrl(false);
	return count;
}

static ssize_t MHD_check_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	printk(KERN_ERR "input data --> %s\n", buf);

	return size;
}

static DEVICE_ATTR(MHD_file, S_IRUGO , MHD_check_read, MHD_check_write);


struct i2c_client* get_sii9234_client(u8 device_id)
{

	struct i2c_client* client_ptr;

	if(device_id == 0x72)
		client_ptr = sii9234_i2c_client;
	else if(device_id == 0x7A)
		client_ptr = sii9234a_i2c_client;
	else if(device_id == 0x92)
		client_ptr = sii9234b_i2c_client;
	else if(device_id == 0xC8)
		client_ptr = sii9234c_i2c_client;
	else
		client_ptr = NULL;

	return client_ptr;
}
EXPORT_SYMBOL(get_sii9234_client);

u8 sii9234_i2c_read(struct i2c_client *client, u8 reg)
{
	u8 ret;

	if(!MHL_i2c_init)
	{
		SII_DEV_DBG("I2C not ready");
		return 0;
	}

	i2c_smbus_write_byte(client, reg);


	ret = i2c_smbus_read_byte(client);

	//printk(KERN_ERR "#######Read reg %x data %x\n", reg, ret);

	if (ret < 0)
	{
		SII_DEV_DBG("i2c read fail");
		return -EIO;
	}
	return ret;

}
EXPORT_SYMBOL(sii9234_i2c_read);


int sii9234_i2c_write(struct i2c_client *client, u8 reg, u8 data)
{
	if(!MHL_i2c_init)
	{
		SII_DEV_DBG("I2C not ready");
		return 0;
	}

	//printk(KERN_ERR "#######Write reg %x data %x\n", reg, data);
	return i2c_smbus_write_byte_data(client, reg, data);
}
EXPORT_SYMBOL(sii9234_i2c_write);


void sii9234_interrupt_event_work(struct work_struct *p)
{

	printk(KERN_ERR "[MHL]sii9234_interrupt_event_work() is called\n");
	sii9234_interrupt_event();
}


void mhl_int_irq_handler_sched(void)
{
	//printk(KERN_ERR "mhl_int_irq_handler_sched() is called\n");
	queue_work(sii9234_wq, &sii9234_int_work);
}


irqreturn_t mhl_int_irq_handler(int irq, void *dev_id)
{
	printk(KERN_ERR "[MHL]mhl_int_irq_handler() is called\n");

	if (gpio_get_value(GPIO_MHL_SEL))
		mhl_int_irq_handler_sched();

	return IRQ_HANDLED;
}


irqreturn_t mhl_wake_up_irq_handler(int irq, void *dev_id)
{

	printk(KERN_ERR "mhl_wake_up_irq_handler() is called\n");

	if (gpio_get_value(GPIO_MHL_SEL))
		mhl_int_irq_handler_sched();

	return IRQ_HANDLED;
}

static int sii9234_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	/* int retval; */

	struct sii9234_state *state;

	struct class *mhl_class;
	struct device *mhl_dev;

	state = kzalloc(sizeof(struct sii9234_state), GFP_KERNEL);
	if (state == NULL) {
		printk(KERN_ERR "failed to allocate memory \n");
		return -ENOMEM;
	}

	state->client = client;
	i2c_set_clientdata(client, state);



	/* rest of the initialisation goes here. */

	printk(KERN_ERR "SII9234 attach success!!!\n");

	sii9234_i2c_client = client;

	MHL_i2c_init = 1;

	mhl_class = class_create(THIS_MODULE, "mhl");
	if (IS_ERR(mhl_class))
	{
		pr_err("Failed to create class(mhl)!\n");
	}

	mhl_dev = device_create(mhl_class, NULL, 0, NULL, "mhl_dev");
	if (IS_ERR(mhl_dev))
	{
		pr_err("Failed to create device(mhl_dev)!\n");
	}

	if (device_create_file(mhl_dev, &dev_attr_MHD_file) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_MHD_file.attr.name);

	return 0;

}



static int __devexit sii9234_remove(struct i2c_client *client)
{
	struct sii9234_state *state = i2c_get_clientdata(client);
	kfree(state);

	return 0;
}

static int sii9234a_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct sii9234_state *state;

	state = kzalloc(sizeof(struct sii9234_state), GFP_KERNEL);
	if (state == NULL) {
		printk(KERN_ERR "failed to allocate memory \n");
		return -ENOMEM;
	}

	state->client = client;
	i2c_set_clientdata(client, state);

	/* rest of the initialisation goes here. */

	printk(KERN_ERR "SII9234A attach success!!!\n");

	sii9234a_i2c_client = client;

	return 0;

}



static int __devexit sii9234a_remove(struct i2c_client *client)
{
	struct sii9234_state *state = i2c_get_clientdata(client);
	kfree(state);
	return 0;
}

static int sii9234b_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct sii9234_state *state;

	state = kzalloc(sizeof(struct sii9234_state), GFP_KERNEL);
	if (state == NULL) {
		printk(KERN_ERR "failed to allocate memory \n");
		return -ENOMEM;
	}

	state->client = client;
	i2c_set_clientdata(client, state);

	/* rest of the initialisation goes here. */

	printk(KERN_ERR "SII9234B attach success!!!\n");

	sii9234b_i2c_client = client;


	return 0;

}



static int __devexit sii9234b_remove(struct i2c_client *client)
{
	struct sii9234_state *state = i2c_get_clientdata(client);
	kfree(state);
	return 0;
}


static int sii9234c_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct sii9234_state *state;
	int ret;

	state = kzalloc(sizeof(struct sii9234_state), GFP_KERNEL);
	if (state == NULL) {
		printk(KERN_ERR "failed to allocate memory \n");
		return -ENOMEM;
	}

	state->client = client;
	i2c_set_clientdata(client, state);

	/* rest of the initialisation goes here. */

	printk(KERN_ERR "SII9234C attach success!!!\n");

	sii9234c_i2c_client = client;

	msleep(100);

	sii9234_wq = create_singlethread_workqueue("sii9234_wq");
	INIT_WORK(&sii9234_int_work, sii9234_interrupt_event_work);

	ret = request_threaded_irq(MHL_INT_IRQ, NULL, mhl_int_irq_handler,
				IRQF_SHARED , "mhl_int", (void *) state);
	if (ret) {
		printk(KERN_ERR "unable to request irq mhl_int"
					" err:: %d\n", ret);
		return ret;
	}
	printk(KERN_ERR "MHL int reques successful %d\n", ret);

	ret = request_threaded_irq(MHL_WAKEUP_IRQ, NULL,
		mhl_wake_up_irq_handler, IRQF_SHARED,
		"mhl_wake_up", (void *) state);
	if (ret) {
		printk(KERN_ERR "unable to request irq mhl_wake_up"
					" err:: %d\n", ret);
		return ret;
	}

	return 0;

}



static int __devexit sii9234c_remove(struct i2c_client *client)
{
	struct sii9234_state *state = i2c_get_clientdata(client);
	kfree(state);
	return 0;
}

struct i2c_driver sii9234_i2c_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "SII9234",
	},
	.id_table	= sii9234_id,
	.probe	= sii9234_i2c_probe,
	.remove	= __devexit_p(sii9234_remove),
	.command = NULL,
};

struct i2c_driver sii9234a_i2c_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "SII9234A",
	},
	.id_table	= sii9234a_id,
	.probe	= sii9234a_i2c_probe,
	.remove	= __devexit_p(sii9234a_remove),
	.command = NULL,
};

struct i2c_driver sii9234b_i2c_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "SII9234B",
	},
	.id_table	= sii9234b_id,
	.probe	= sii9234b_i2c_probe,
	.remove	= __devexit_p(sii9234b_remove),
	.command = NULL,
};

struct i2c_driver sii9234c_i2c_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "SII9234C",
	},
	.id_table	= sii9234c_id,
	.probe	= sii9234c_i2c_probe,
	.remove	= __devexit_p(sii9234c_remove),
	.command = NULL,
};


extern struct device * fimc_get_active_device(void);


void sii9234_cfg_power(bool on)
{

	if(on)
	{
//		s3c_gpio_cfgpin(GPIO_HDMI_EN,S3C_GPIO_OUTPUT);	//HDMI_EN
#ifdef CONFIG_TARGET_LOCALE_KOR
		gpio_set_value(GPIO_HDMI_EN,GPIO_LEVEL_HIGH);
#else
		if(system_rev < 7) {
#ifndef CONFIG_MACH_C1_NA_SPR_EPIC2_REV00
			gpio_set_value(GPIO_HDMI_EN,GPIO_LEVEL_HIGH);
#endif
		} else
			gpio_set_value(GPIO_HDMI_EN_REV07,GPIO_LEVEL_HIGH);
#endif

		s3c_gpio_cfgpin(GPIO_MHL_RST,S3C_GPIO_OUTPUT);	//MHL_RST
		s3c_gpio_setpull(GPIO_MHL_RST, S3C_GPIO_PULL_NONE);


		s3c_gpio_setpull(GPIO_AP_SCL_18V, S3C_GPIO_PULL_DOWN);
		gpio_set_value(GPIO_MHL_RST, GPIO_LEVEL_LOW);
		msleep(10);
		gpio_set_value(GPIO_MHL_RST, GPIO_LEVEL_HIGH);
		s3c_gpio_setpull(GPIO_AP_SCL_18V, S3C_GPIO_PULL_NONE);

		sii9234_unmaks_interrupt();
	}
	else
	{
		gpio_set_value(GPIO_MHL_RST, GPIO_LEVEL_LOW);
		msleep(10);
		gpio_set_value(GPIO_MHL_RST, GPIO_LEVEL_HIGH);
#ifdef CONFIG_TARGET_LOCALE_KOR
		gpio_set_value(GPIO_HDMI_EN,GPIO_LEVEL_HIGH);
#else
		if(system_rev < 7) {
#ifndef CONFIG_MACH_C1_NA_SPR_EPIC2_REV00
		  gpio_set_value(GPIO_HDMI_EN,GPIO_LEVEL_LOW);
#endif
		}else
			gpio_set_value(GPIO_HDMI_EN_REV07,GPIO_LEVEL_LOW);
#endif
		gpio_set_value(GPIO_MHL_RST, GPIO_LEVEL_LOW);
	}
	printk(KERN_ERR "[MHL]%s : %d \n",__func__,on);

out:
	return;
}


static void sii9234_cfg_gpio()
{
	s3c_gpio_cfgpin(GPIO_AP_SDA_18V, S3C_GPIO_SFN(0x0));	//AP_MHL_SDA
	s3c_gpio_setpull(GPIO_AP_SDA_18V, S3C_GPIO_PULL_NONE);

	s3c_gpio_cfgpin(GPIO_AP_SCL_18V,S3C_GPIO_SFN(0x1));	//AP_MHL_SCL
	s3c_gpio_setpull(GPIO_AP_SCL_18V, S3C_GPIO_PULL_NONE);

	s3c_gpio_cfgpin(GPIO_MHL_WAKE_UP,S3C_GPIO_INPUT);//GPH1(6) XEINT 14
	set_irq_type(MHL_WAKEUP_IRQ, IRQ_TYPE_EDGE_RISING);
	s3c_gpio_setpull(GPIO_MHL_WAKE_UP, S3C_GPIO_PULL_DOWN);

	s3c_gpio_setpull(GPIO_MHL_INT, S3C_GPIO_PULL_DOWN);
	set_irq_type(MHL_INT_IRQ, IRQ_TYPE_EDGE_RISING);
	s3c_gpio_cfgpin(GPIO_MHL_INT, GPIO_MHL_INT_AF);

#ifdef CONFIG_TARGET_LOCALE_KOR
	s3c_gpio_cfgpin(GPIO_HDMI_EN,S3C_GPIO_OUTPUT);	//HDMI_EN
	gpio_set_value(GPIO_HDMI_EN,GPIO_LEVEL_LOW);
	s3c_gpio_setpull(GPIO_HDMI_EN, S3C_GPIO_PULL_NONE);
#else
	if(system_rev < 7) {
#ifndef CONFIG_MACH_C1_NA_SPR_EPIC2_REV00
		s3c_gpio_cfgpin(GPIO_HDMI_EN,S3C_GPIO_OUTPUT);	//HDMI_EN
		gpio_set_value(GPIO_HDMI_EN,GPIO_LEVEL_LOW);
		s3c_gpio_setpull(GPIO_HDMI_EN, S3C_GPIO_PULL_NONE);
#endif
	}
	else {
		s3c_gpio_cfgpin(GPIO_HDMI_EN_REV07,S3C_GPIO_OUTPUT);	//HDMI_EN
		gpio_set_value(GPIO_HDMI_EN_REV07,GPIO_LEVEL_LOW);
		s3c_gpio_setpull(GPIO_HDMI_EN_REV07, S3C_GPIO_PULL_NONE);
	}
#endif

	s3c_gpio_cfgpin(GPIO_MHL_RST,S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_MHL_RST, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_MHL_RST, GPIO_LEVEL_LOW);

	s3c_gpio_cfgpin(GPIO_MHL_SEL,S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_MHL_SEL, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_MHL_SEL, GPIO_LEVEL_LOW);

}

static int mhl_open(struct inode *ip, struct file *fp)
{
	printk(KERN_ERR "[%s] \n",__func__);
	return 0;

}

static int mhl_release(struct inode *ip, struct file *fp)
{

	printk(KERN_ERR "[%s] \n",__func__);
	return 0;
}


static int mhl_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	printk(KERN_ERR "[%s] \n",__func__);

#if 0
	byte data;

	switch(cmd)
	{
		case MHL_READ_RCP_DATA:
			data = GetCbusRcpData();
			ResetCbusRcpData();
			put_user(data,(byte *)arg);
			printk(KERN_ERR "MHL_READ_RCP_DATA read");
			break;

		default:
		break;
	}
#endif
	return 0;
}

static struct file_operations mhl_fops = {
	.owner  = THIS_MODULE,
	.open   = mhl_open,
	.release = mhl_release,
	.ioctl = mhl_ioctl,
};

static struct miscdevice mhl_device = {
    .minor  = MISC_DYNAMIC_MINOR,
    .name   = "mhl",
    .fops   = &mhl_fops,
};

static int __init sii9234_module_init(void)
{
	int ret;

	sii9234_cfg_gpio();

	/* sii9234_cfg_power(1);	//Turn On power to sii9234
	*/

#ifdef MHL_SWITCH_TEST
	sec_mhl = class_create(THIS_MODULE, "sec_mhl");
	if (IS_ERR(sec_mhl))
		printk(KERN_ERR "[MHL] Failed to create class (sec_mhl)\n");

	mhl_switch = device_create(sec_mhl, NULL, 0, NULL, "switch");
	if (IS_ERR(mhl_switch))
		printk(KERN_ERR "[MHL] Failed to create device (mhl_switch)\n");
	if (device_create_file(mhl_switch, &dev_attr_mhl_sel) < 0)
		printk(KERN_ERR "[MHL] Failed to create file (mhl_sel)");
#endif

	ret = misc_register(&mhl_device);
	if(ret) {
		pr_err(KERN_ERR "misc_register failed - mhl \n");
	}

	ret = i2c_add_driver(&sii9234_i2c_driver);
	if (ret != 0)
		printk(KERN_ERR "[MHL SII9234] can't add i2c driver\n");
	else
		printk(KERN_ERR "[MHL SII9234] add i2c driver\n");
	ret = i2c_add_driver(&sii9234a_i2c_driver);
	if (ret != 0)
		printk(KERN_ERR "[MHL SII9234A] can't add i2c driver\n");
	else
		printk(KERN_ERR "[MHL SII9234A] add i2c driver\n");
	ret = i2c_add_driver(&sii9234b_i2c_driver);
	if (ret != 0)
		printk(KERN_ERR "[MHL SII9234B] can't add i2c driver\n");
	else
		printk(KERN_ERR "[MHL SII9234B] add i2c driver\n");

	ret = i2c_add_driver(&sii9234c_i2c_driver);
	if (ret != 0)
		printk(KERN_ERR "[MHL SII9234C] can't add i2c driver\n");
	else
		printk(KERN_ERR "[MHL SII9234C] add i2c driver\n");

	return ret;
}
module_init(sii9234_module_init);
static void __exit sii9234_exit(void)
{
	i2c_del_driver(&sii9234_i2c_driver);
	i2c_del_driver(&sii9234a_i2c_driver);
	i2c_del_driver(&sii9234b_i2c_driver);
	i2c_del_driver(&sii9234c_i2c_driver);

};
module_exit(sii9234_exit);

MODULE_DESCRIPTION("Sii9234 MHL driver");
MODULE_AUTHOR("Aakash Manik");
MODULE_LICENSE("GPL");
