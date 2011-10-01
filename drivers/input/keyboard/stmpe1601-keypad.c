/*
 * File: drivers/input/keyboard/stmpe1601_keys.c
 * Description:  keypad driver for STMPE1601
 *               I2C QWERTY Keypad and IO Expander *
 *
 * Licensed under the GPL-2 or later.
 */

#include <linux/module.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/errno.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
#include <linux/input/matrix_keypad.h>
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/sysfs.h>
#include <linux/earlysuspend.h>
#include <linux/io.h>
#include <linux/stmpe1601-keypad.h>

#include <plat/gpio-cfg.h>
#include <mach/regs-gpio.h>
#include <mach/gpio.h>
#include <mach/hardware.h>

#define DEBUG	1

#ifdef DEBUG
#define STMPE1601_KPAD_DEBUG
#endif

#if DEBUG
#define dprintk(fmt, x... ) printk("[%s](%d):"fmt, __FUNCTION__ ,__LINE__, ##x)
#else
#define dprintk(x...) do { } while (0)
#endif



/* Fn Define */
static void key_led_onoff(bool);
static inline int key_led_config(void);
static void keypad_backlight_on_slide(void);
static int g_slide_status;
static bool g_led_status;


struct stmpe1601_kpad *gkpad;

/* Autosleep timeout delay (in ms) supported by STMPE1601 */
static const int stmpe1601_autosleep_delay[] = {
	4, 16, 32, 64, 128, 256, 512, 1024
};
static bool key_led_stat = false;  //status for keypad led
static int keypress = 1;
static int led_timeout = KEYPAD_LED_TIMEOUT;

/* sys fs : /sys/devices/virtual/key/key/key */
struct class *key_class, *keyled_class;
struct device *key_dev, *keyled_dev;


static inline int stmpe1601_read(struct stmpe1601_kpad *kpad, u8 reg)
{
	int ret;

	ret = i2c_smbus_read_byte_data(kpad->i2c, reg);
	if (ret < 0)
		dev_err(&kpad->i2c->dev, "[%s]: I2C Read Error for Reg[0x%02X]: %d\n",
			__func__, reg, ret);

	return ret;
}


static inline int stmpe1601_write(struct stmpe1601_kpad *kpad, u8 reg, u8 val)
{
	int ret;

	ret = i2c_smbus_write_byte_data(kpad->i2c, reg, val);
	if (ret < 0)
		dev_err(&kpad->i2c->dev, "[%s]: I2C Write Error for Reg[0x%02X]: %d\n",
			__func__, reg, ret);

	return ret;
}


static int stmpe1601_endisable(struct stmpe1601_kpad *kpad,
	unsigned int cntrler, bool endis)
{
	unsigned int mask = 0;
	int ret;

	if (cntrler & STMPE_CTRL_PWM)
		mask |= STMPE1601_SYSCON_EN_SPWM;

	if (cntrler & STMPE_CTRL_KEYPAD)
		mask |= STMPE1601_SYS_CTRL_EN_KPC;

	if (cntrler & STMPE_CTRL_GPIO)
		mask |= STMPE1601_SYS_CTRL_EN_GPIO;

	if (cntrler & STMPE_CTRL_ALL)
		mask |= STMPE1601_SYSCON_EN_SPWM | STMPE1601_SYS_CTRL_EN_KPC |
		STMPE1601_SYS_CTRL_EN_GPIO;

	ret = stmpe1601_read(kpad, STMPE1601_REG_SYS_CTRL);
	if (ret < 0)
		return ret;

	ret &= ~mask;
	ret |= (endis ? mask : 0);

	return stmpe1601_write(kpad, STMPE1601_REG_SYS_CTRL, ret);
}

static void key_led_onoff(bool sw)
{
	if (sw) {
		if (g_led_status)
		gpio_set_value(KEYPAD_LED_CTRL_SW, GPIO_LEVEL_HIGH);
	} else
		gpio_set_value(KEYPAD_LED_CTRL_SW, GPIO_LEVEL_LOW);
}


static void keypad_backlight_on_slide()
{
	unsigned int slide_state;

	if (!key_led_stat) {
		slide_state = gpio_get_value(KEYPAD_SLIDE_SW);
		if (slide_state) {
			key_led_onoff(true);
			key_led_stat = true;
		}
	}

	hrtimer_start(&gkpad->timer, ktime_set(led_timeout, 0), HRTIMER_MODE_REL);
}


static inline int key_led_config()
{
	int ret;

	ret = gpio_request(KEYPAD_LED_CTRL_SW, "GPB_5");
	if (ret < 0)
		return ret;

	s3c_gpio_cfgpin(KEYPAD_LED_CTRL_SW, S3C_GPIO_OUTPUT);
	return 0;
}


static int stmpe1601_kpad_read_data(struct stmpe1601_kpad *kpad, u8 *data)
{
	int ret;

	ret = i2c_smbus_read_i2c_block_data(kpad->i2c, STMPE1601_REG_KPC_DATA_BYTE0,
		STMPE1601_KPC_DATA_LEN, data);
	if (ret < 0)
		dev_err(&kpad->i2c->dev, "[%s]: failed to read key fifo !!!!!\n", __func__);

	return ret;
}


static void stmpe1601_message_read(void)
{
	u8 val, data, key_fifo[STMPE1601_KPC_DATA_LEN];
	int row, col, kcode;
	bool up;
	int slide_state;
	int i, ret;

	/* Key LED ON */
	if (!key_led_stat) {
		slide_state = gpio_get_value(KEYPAD_SLIDE_SW);
		if (slide_state) {
			key_led_onoff(true);
			key_led_stat = true;
		}
	}

	/* Key Read */
	val = stmpe1601_read(gkpad, STMPE1601_REG_INT_STA_LSB);

	if (val & STPME1601_KPC_FIFO_OVR_EN)
		dev_err(&gkpad->i2c->dev, "[Q-KEY] Keypad FIFO Overflow\n");

	if (val & STPME1601_KPC_INT_EN) {
		ret = stmpe1601_kpad_read_data(gkpad, key_fifo);
		if (ret < 0)
			dev_err(&gkpad->i2c->dev, "[Q-KEY] Keypad data read failed\n");

		for (i = 0; i < STMPE1601_KPC_DATA_LEN-2; i++) {
			data = key_fifo[i];
			row = (data & STMPE1601_KPC_DATA_ROW) >> 3;
			col = data & STMPE1601_KPC_DATA_COL;
			kcode = MATRIX_SCAN_CODE(row, col, STMPE1601_KPAD_ROW_SHIFT);
			up = data & STMPE1601_KPC_DATA_UP;
			keypress = up;

			if ((data & STMPE1601_KPC_DATA_NOKEY_MASK) ==
				STMPE1601_KPC_DATA_NOKEY_MASK)
			    continue;
#if 0
			dprintk(KERN_DEBUG"[Q-KEY] Row = %d, Col = %d, UP = %d\n",
				row, col, up);
#else
			printk(KERN_ERR"[Q-KEY] code:%d, row=%d, col=%d, up=%d\n",
				gkpad->keycode[kcode], row, col, up);
#endif
			input_event(gkpad->input, EV_MSC, MSC_SCAN, kcode);
			input_report_key(gkpad->input, gkpad->keycode[kcode], !up);
			input_sync(gkpad->input);
		}
	}

	/* clear interrupt */
	stmpe1601_write(gkpad, STMPE1601_REG_INT_STA_LSB, val);

	/* Keypad glow timeout */
	hrtimer_start(&gkpad->timer, ktime_set(led_timeout, 0), HRTIMER_MODE_REL);
}

irqreturn_t stmpe1601_irq_handler(int irq, void *dev)
{
	stmpe1601_message_read();

	return IRQ_HANDLED;
}

#if defined(CONFIG_MACH_C1_NA_SPR_EPIC2_REV00)
static irqreturn_t slide_irq_handler(int irq, void *dev)
{
	struct stmpe1601_kpad *kpad = (struct stmpe1601_kpad *)dev;
	struct input_dev *input = kpad->input;
	int lid_status = gpio_get_value(KEYPAD_SLIDE_SW);

	printk(KERN_DEBUG"[Q-KEY] lid_status = %d\n", lid_status);
	if (lid_status) {
		input->sw[SW_LID] = 1;
		printk(KERN_DEBUG"[Q-KEY] slide On\n");
		keypad_backlight_on_slide();
	} else {
		input->sw[SW_LID] = 0;
		printk(KERN_DEBUG"[Q-KEY] slide Off\n");
		if (key_led_stat) {
			hrtimer_cancel(&gkpad->timer);
			key_led_onoff(false);
			key_led_stat = false;
		}
	}

	input_report_switch(input, SW_LID, (lid_status ? 0 : 1));
	input_sync(input);

	return IRQ_HANDLED;
}
#endif

static ssize_t key_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	if (!keypress)
		return sprintf(buf, "PRESSED\n");
	else
		return sprintf(buf, "RELEASED\n");
}


static ssize_t key_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return 0;
}


static ssize_t keyled_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "Keypad glow timeout = %d\n", led_timeout);
}


static ssize_t keyled_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	sscanf(buf, "%d\n", &led_timeout);
	return count;
}

static DEVICE_ATTR(key, 0644, key_show, key_store);
static DEVICE_ATTR(keyled, 0644, keyled_show, keyled_store);

static enum hrtimer_restart keypad_led_timer_func(struct hrtimer *timer)
{
	key_led_onoff(false);
	key_led_stat = false;

	return HRTIMER_NORESTART;
}


#ifdef CONFIG_HAS_EARLYSUSPEND
void stmpe1601_early_suspend(struct early_suspend *sus)
{
#if 0
	u8 val;
#endif
	printk(KERN_DEBUG"[Q-KEY] %s\n", __func__);

	g_led_status = 0;

	/* Key LED OFF */
	if (key_led_stat) {
		key_led_stat = false;
		hrtimer_cancel(&gkpad->timer);
		key_led_onoff(false);
	}

#if 0
	/* To prevent the kernel crash  */
	/* Sending Keypad controller to hybernate mode */
	val = stmpe1601_read(gkpad, STMPE1601_REG_SYS_CTRL);
	stmpe1601_write(gkpad, STMPE1601_REG_SYS_CTRL,
		(val | STMPE1601_KPC_HYBER_EN));
#endif
}


void stmpe1601_late_resume(struct early_suspend *res)
{
	printk(KERN_DEBUG"[Q-KEY] %s\n", __func__);

	int lid_status = gpio_get_value(KEYPAD_SLIDE_SW);

	g_led_status = 1;

	/* Key LED ON */
	if (lid_status) {
		key_led_stat = true;
		key_led_onoff(true);
		hrtimer_start(&gkpad->timer, ktime_set(led_timeout, 0),
			HRTIMER_MODE_REL);  /* Start keypad glow timeout */
	}
#if 0
	/* To prevent the kernel crash */
	/* To bring device out of hybernate mode */
	stmpe1601_read(gkpad, STMPE1601_REG_SYS_CTRL);
#endif
}

static struct early_suspend stmpe1601_kpad_earlysuspend = {
	.suspend = stmpe1601_early_suspend,
	.resume = stmpe1601_late_resume,
};
#endif

#ifdef STMPE1601_KPAD_DEBUG
static void stmpe1601_reg_dump(struct stmpe1601_kpad *kpad)
{
	u8 reg, reg_val[50];
	u8 reg_addr[] = {
		0x80, 0x81, 0x02, 0x03, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
		0x60, 0x61, 0x62, 0x63, 0x64, 0x92, 0x93, 0x94, 0x95,
	};
	int i, ret;

	/* Chip Version */
	reg = 0x80;
	ret = i2c_smbus_read_i2c_block_data(kpad->i2c, reg, 2, &reg_val[0]);
	if (ret < 0)
		dev_err(&kpad->i2c->dev, "failed to read regs Reg[0x%02X]: %d\n",
			reg, ret);

	/* System Regs */
	reg = 0x02;
	ret = i2c_smbus_read_i2c_block_data(kpad->i2c, reg, 2, &reg_val[2]);
	if (ret < 0)
		dev_err(&kpad->i2c->dev, "failed to read regs Reg[0x%02X]: %d\n",
			reg, ret);

	/* Interrupt Regs */
	reg = 0x10;
	ret = i2c_smbus_read_i2c_block_data(kpad->i2c, reg, 6, &reg_val[4]);
	if (ret < 0)
		dev_err(&kpad->i2c->dev, "failed to read regs Reg[0x%02X]: %d\n",
			reg, ret);

	/* Keypad Controller Regs */
	reg = 0x60;
	ret = i2c_smbus_read_i2c_block_data(kpad->i2c, reg, 5, &reg_val[10]);
	if (ret < 0)
		dev_err(&kpad->i2c->dev, "failed to read regs Reg[0x%02X]: %d\n",
			reg, ret);

	/* Keypad Controller Regs */
	reg = 0x92;
	ret = i2c_smbus_read_i2c_block_data(kpad->i2c, reg, 4, &reg_val[15]);
	if (ret < 0)
		dev_err(&kpad->i2c->dev, "failed to read regs Reg[0x%02X]: %d\n",
			reg, ret);

	printk("#############################################\n");
	printk("########    STMPE Reg Dump    ###############\n");
	printk("#############################################\n");

	for (i = 0; i < 19; i++)
		printk("########   Reg[0x%02X] = 0x%02X   ###############\n",
			reg_addr[i], reg_val[i]);

	printk("#############################################\n");
	printk("########    Dump Completed    ###############\n");
	printk("#############################################\n");
}
#endif


static int __devinit stmpe1601_autosleep(struct stmpe1601_kpad *kpad,
	unsigned int autosleep_timeout)
{
	int timeout = -1;
	u8 scw2 = 0;
	int i, ret;


	/* Rounding autosleep timeout */
	for (i = 0; i < ARRAY_SIZE(stmpe1601_autosleep_delay); i++) {
		if (stmpe1601_autosleep_delay[i] >= autosleep_timeout) {
			timeout = i;
			break;
		}
	}

	if (timeout < 0)
		return -EINVAL;

	scw2 |= timeout | STPME1601_AUTOSLEEP_EN;
	ret = stmpe1601_write(kpad, STMPE1601_REG_SYS_CTRL2, scw2);
	if (ret < 0)
		return ret;

	return 0;
}


static int __devinit stmpe1601_chip_init(struct stmpe1601_kpad *kpad)
{
	const struct stmpe1601_kpad_platform_data *plat = kpad->plat;
	u8 icw;
	int ret;

	if (plat->debounce_ms > STMPE1601_KPAD_MAX_DEBOUNCE)
		return -EINVAL;

	if (plat->scan_count > STMPE1601_KPAD_MAX_SCAN_COUNT)
		return -EINVAL;

	/* Disabling all controllers over chip */
	ret = stmpe1601_endisable(kpad, STMPE_CTRL_ALL, false);
	if (ret < 0) {
		printk(KERN_ERR"[Q-KEY] disable controller block fail\n");
		return ret;
	}

	/* Enable Keypad block */
	ret = stmpe1601_endisable(kpad, STMPE_CTRL_KEYPAD, true);
	if (ret < 0)
		return ret;

	/* 1. Enable GPIO block n set alternate fn to keypad ctrl */
	ret = stmpe1601_endisable(kpad, STMPE_CTRL_GPIO, true);
	if (ret < 0)
		return ret;

	ret = stmpe1601_write(kpad, STMPE1601_GPIO_AF_U_MSB,
		STMPE1601_GPIO_ALT_Fn_KPAD);
	if (ret < 0)
		return ret;

	ret = stmpe1601_write(kpad, STMPE1601_GPIO_AF_U_LSB,
		STMPE1601_GPIO_ALT_Fn_KPAD);
	if (ret < 0)
		return ret;

	ret = stmpe1601_write(kpad, STMPE1601_GPIO_AF_L_MSB,
		STMPE1601_GPIO_ALT_Fn_KPAD);
	if (ret < 0)
		return ret;

	ret = stmpe1601_write(kpad, STMPE1601_GPIO_AF_L_LSB,
		STMPE1601_GPIO_ALT_Fn_KPAD);
	if (ret < 0)
		return ret;

	/* 2.Enabling scanning of columns */
	ret = stmpe1601_write(kpad, STMPE1601_REG_KPC_COL,
		STMPE1601_COL_SCAN_EN_WRD);
	if (ret < 0)
		return ret;

	/* 3.Enable scanning of rows */
	ret = stmpe1601_write(kpad, STMPE1601_REG_KPC_ROW_LSB,
		STMPE1601_ROW_SCAN_EN_WRD);
	if (ret < 0)
		return ret;

	/* 4.Setting scanning cycles */
	ret = stmpe1601_write(kpad, STMPE1601_REG_KPC_CTRL_MSB,
		plat->scan_count<<4);
	if (ret < 0)
		return ret;

	/* 5.Setting debounce and Scanning */
	ret = stmpe1601_write(kpad, STMPE1601_REG_KPC_CTRL_LSB,
		(plat->debounce_ms<<1 | STMPE1601_SCAN_EN));
	if (ret < 0)
		return ret;

	/* Enabling Autosleep Functionality */
	if (plat->autosleep_timeout) {
		ret = stmpe1601_autosleep(kpad, plat->autosleep_timeout);
		if (ret < 0)
			printk(KERN_ERR"[Q-KEY] autosleep functionality is not set\n");
	}

	/* Interrupt configuration n Enabling */
	if ((plat->irq_trigger == IRQF_TRIGGER_FALLING) ||
		(plat->irq_trigger == IRQF_TRIGGER_RISING))
		icw |= STMPE1601_EDGE_INT;

	if ((plat->irq_trigger == IRQF_TRIGGER_RISING) ||
		(plat->irq_trigger == IRQF_TRIGGER_HIGH))
		icw |= STMPE1601_AHRE_INT;

	icw |= STMPE1601_GIM_SET;

	ret = stmpe1601_write(kpad, STMPE1601_REG_INT_CTRL_LSB, icw);
	if (ret < 0)
		return ret;

	ret = stmpe1601_read(kpad, STMPE1601_REG_INT_EN_MASK_LSB);
	if (ret < 0)
		return ret;

	ret = stmpe1601_write(kpad, STMPE1601_REG_INT_EN_MASK_LSB,
		(ret | STPME1601_KPC_INT_EN | STPME1601_KPC_FIFO_OVR_EN));
	if (ret < 0)
		return ret;

	return 0;
}


static int __devinit stmpe1601_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	struct stmpe1601_kpad *kpad;
	struct stmpe1601_kpad_platform_data *pdata = client->dev.platform_data;
	struct input_dev *input;
	u8 revid, verid;
	int ret;

	printk(KERN_DEBUG"[Q-KEY] %s\n", __func__);

	g_led_status = 1;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(&client->dev, "SMBUS Byte Data not Supported\n");
		return -EIO;
	}

	if (!pdata) {
		dev_err(&client->dev, "No platform data available\n");
		return -EINVAL;
	}

	if (!pdata->keymap) {
		dev_err(&client->dev, "Keymap data is not available\n");
		return -EINVAL;
	}

	kpad = kzalloc(sizeof(struct stmpe1601_kpad), GFP_KERNEL);
	if (!kpad) {
		dev_err(&client->dev, "No memory available\n");
		return -ENOMEM;
	}

	input = input_allocate_device();
	if (!input) {
		dev_err(&client->dev, "Failed to allocate input subsystem\n");
		ret = -ENOMEM;
		goto err;
	}

	kpad->dev = &client->dev;
	kpad->i2c = client;
	kpad->partnum = id->driver_data;
	kpad->input = input;
	kpad->plat = pdata;

	revid = stmpe1601_read(kpad, STMPE1601_REG_CHIP_ID);
	if (revid < 0) {
		printk(KERN_ERR"[Q-KEY] Not able to read revID\n");
		ret = revid;
		goto err1;
	}

	verid = stmpe1601_read(kpad, STMPE1601_REG_VERSION_ID);
	if (verid < 0) {
		printk(KERN_ERR"[Q-KEY] Not able to read verID\n");
		ret = verid;
		goto err1;
	}

	if (revid != STMPE1601_CHIP_ID) {
		printk(KERN_ALERT"[Q-KEY] device is not supported by this driver\n");
		return -EINVAL;
	}

	printk(KERN_INFO"[Q-KEY] ID=0x%02X, VER=0x%02X ### STMPE1601 detected\n",
		revid, verid);

	input->name = client->name;
	input->phys = "stmpe1601-keys/input0";
	input->dev.parent = &client->dev;
	input->id.bustype = BUS_I2C;
	input->id.vendor = 0x0000;
	input->id.product = revid;
	input->id.version = verid;

	input_set_drvdata(input, kpad);

	/* setup input device */
	__set_bit(EV_KEY, input->evbit);

	if (pdata->repeat)
		__set_bit(EV_REP, input->evbit);

	input_set_capability(input, EV_MSC, MSC_SCAN);

	input->keycode = kpad->keycode;
	input->keycodesize = sizeof(kpad->keycode[0]);
	input->keycodemax = ARRAY_SIZE(kpad->keycode);

	matrix_keypad_build_keymap(pdata->keymap, STMPE1601_KPAD_ROW_SHIFT,
		input->keycode, input->keybit);

	ret = stmpe1601_chip_init(kpad);
	if (ret) {
		printk(KERN_ALERT "[Q-KEY] STMPE1601 initialization failed\n");
		goto err1;
	}

	ret = input_register_device(input);
	if (ret) {
		dev_err(&client->dev, "Failed to register input subsystem\n");
		goto err1;
	}

	set_irq_type(kpad->i2c->irq, IRQ_TYPE_EDGE_FALLING);

	ret = request_threaded_irq(kpad->i2c->irq, NULL, stmpe1601_irq_handler,
		kpad->plat->irq_trigger | IRQF_DISABLED, "stmpe1601_irq", kpad);
	if (ret) {
		dev_err(&client->dev, "[Q-KEY] Keypad IRQ %d is already occupied\n",
			kpad->i2c->irq);
		goto err2;
	}
#ifdef CONFIG_MACH_C1_NA_SPR_EPIC2_REV00
	set_irq_type(KEYPAD_SLIDE_SW, IRQ_TYPE_EDGE_BOTH);

	ret = request_threaded_irq(pdata->slide_irq, NULL, slide_irq_handler,
		(IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING | IRQF_DISABLED),
		"slide_irq", kpad);
	if (ret) {
		dev_err(&client->dev, "[Q-KEY] slide irq failed...\n");
		goto err3;
	}

	__set_bit(SW_LID, input->swbit);
	__set_bit(EV_SW, input->evbit);

	if (gpio_get_value(KEYPAD_SLIDE_SW))
		input->sw[SW_LID] = 0;
	else
		input->sw[SW_LID] = 1;
#endif
	__clear_bit(KEY_RESERVED, input->keybit);


	device_init_wakeup(&client->dev, 1);
	i2c_set_clientdata(client, kpad);

#ifdef CONFIG_HAS_EARLYSUSPEND
	stmpe1601_kpad_earlysuspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 1;
	register_early_suspend(&stmpe1601_kpad_earlysuspend);
#endif

	/* Timer for keypad LED */
	hrtimer_init(&kpad->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	kpad->timer.function = keypad_led_timer_func;

	/* Keypad LED gpio config */
	ret = key_led_config();
	if (ret < 0) {
		dev_err(&client->dev, "[Q-KEY] Keypad LED control gpio busy\n");
		goto err4;
	}

	gkpad = kpad;

#ifdef STMPE1601_KPAD_DEBUG
	stmpe1601_reg_dump(kpad);
#endif

	printk(KERN_DEBUG"[Q-KEY] STMPE1601 keypad driver registered\n");

	return 0;

err4:
#ifdef CONFIG_MACH_C1_NA_SPR_EPIC2_REV00
	free_irq(KEYPAD_SLIDE_SW, kpad);
#endif
err3:
	free_irq(kpad->i2c->irq, kpad);
err2:
	input_unregister_device(input);
	input = NULL;
err1:
	input_free_device(input);
err:
	kfree(kpad);

	printk(KERN_DEBUG"[Q-KEY] STMPE1601 keypad driver register fail\n");

	return ret;
}


static int __devexit stmpe1601_remove(struct i2c_client *client)
{
	struct stmpe1601_kpad *kpad = i2c_get_clientdata(client);

	printk(KERN_DEBUG"[Q-KEY] %s\n", __func__);

	hrtimer_cancel(&kpad->timer);
#ifdef CONFIG_MACH_C1_NA_SPR_EPIC2_REV00
	free_irq(KEYPAD_SLIDE_SW, kpad);
#endif
	free_irq(kpad->i2c->irq, kpad);
	input_unregister_device(kpad->input);
	i2c_set_clientdata(client, NULL);
	kfree(kpad);
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&stmpe1601_kpad_earlysuspend);
#endif

	return 0;
}

#ifdef CONFIG_PM
static int stmpe1601_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct stmpe1601_kpad *kpad = i2c_get_clientdata(client);

	printk(KERN_DEBUG"[Q-KEY] %s\n", __func__);

	gpio_set_value(KEYPAD_LED_CTRL_SW, GPIO_LEVEL_LOW);

	int irq = gpio_to_irq(KEYPAD_SLIDE_SW);

	disable_irq(client->irq);
	disable_irq(irq);

	if (device_may_wakeup(&client->dev)) {
		enable_irq_wake(client->irq);
		enable_irq_wake(irq);
	}

	g_slide_status = gpio_get_value(KEYPAD_SLIDE_SW);

	return 0;
}


static int stmpe1601_resume(struct i2c_client *client)
{
	struct stmpe1601_kpad *kpad = i2c_get_clientdata(client);

	gpio_set_value(KEYPAD_LED_CTRL_SW, GPIO_LEVEL_LOW);

	int irq = gpio_to_irq(KEYPAD_SLIDE_SW);
	int lid_status = gpio_get_value(KEYPAD_SLIDE_SW);

	printk(KERN_DEBUG"[Q-KEY] %s\n", __func__);

	if (device_may_wakeup(&client->dev)) {
		disable_irq_wake(client->irq);
		disable_irq_wake(irq);
	}

	if (lid_status)
		kpad->input->sw[SW_LID] = 1;
	else
		kpad->input->sw[SW_LID] = 0;

	if (lid_status != g_slide_status) {
		printk(KERN_DEBUG"[Q-KEY]1 g_slide_status = %d, lid_status = %d\n",
			g_slide_status, lid_status);
		input_report_switch(kpad->input, SW_LID, (lid_status ? 0 : 1));
		input_sync(kpad->input);
	} else {
		printk(KERN_DEBUG"[Q-KEY]2 g_slide_status = %d, lid_status = %d\n",
			g_slide_status, lid_status);
	}
	enable_irq(client->irq);
	enable_irq(irq);

	return 0;
}
#else
#define stmpe1601_suspend	NULL
#define stmpe1601_resume	NULL
#endif

static const struct i2c_device_id stmpe1601_id[] = {
	{ "sec_keypad", 0 },
	{ }
};

static struct i2c_driver stmpe1601_driver = {
	.driver.name    = "sec_keypad",
	.driver.owner   = THIS_MODULE,
	.probe          = stmpe1601_probe,
	.remove         = __devexit_p(stmpe1601_remove),
	.id_table       = stmpe1601_id,
	.suspend        = stmpe1601_suspend,
	.resume         = stmpe1601_resume,
};

static int __init stmpe1601_init(void)
{
/* KeyLed */
	keyled_class = class_create(THIS_MODULE, "keyled");
	if (!keyled_class)
		printk("Failed to create class(keyled)!\n");

	keyled_dev = device_create(keyled_class, NULL, 0, NULL, "keyled");
	if (!keyled_dev)
		printk("Failed to create device(keyled)!\n");

	if (device_create_file(keyled_dev, &dev_attr_keyled) < 0)
		printk("Failed to create device file(%s)!\n", dev_attr_keyled.attr.name);

/* Key */
	key_class = class_create(THIS_MODULE, "key");
	if (!key_class)
		printk("Failed to create class(key)!\n");

	key_dev = device_create(key_class, NULL, 0, NULL, "key");
	if (!key_dev)
		printk("Failed to create device(key)!\n");

	if (device_create_file(key_dev, &dev_attr_key) < 0)
		printk("Failed to create device file(%s)!\n", dev_attr_key.attr.name);

	return i2c_add_driver(&stmpe1601_driver);
}

static void __exit stmpe1601_exit(void)
{
	i2c_del_driver(&stmpe1601_driver);
}

module_init(stmpe1601_init);
module_exit(stmpe1601_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("STMPE1601 KeyExpander Driver");
MODULE_AUTHOR("SAMSUNG ELECTORNICS");
