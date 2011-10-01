/* linux/drivers/video/samsung/s3cfb_s6e8aa0.c
 *
 * MIPI-DSI based AMS529HA01 AMOLED lcd panel driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/ctype.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/workqueue.h>
#include <linux/backlight.h>
#include <linux/lcd.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-dsim.h>
#include <mach/dsim.h>
#include <mach/mipi_ddi.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#include "s5p-dsim.h"
#include "s6e8aa0_gamma_2_2.h"


#define MIN_BRIGHTNESS		0
#define MAX_BRIGHTNESS		255
#define DEFAULT_GAMMA_LEVEL		10
#define DEFAULT_BRIGHTNESS		122

#define POWER_IS_ON(pwr)		((pwr) <= FB_BLANK_NORMAL)

struct lcd_info {
	unsigned int			bl;
	unsigned int			gamma_mode;
	unsigned int			acl_enable;
	unsigned int			cur_acl;
	unsigned int			current_gamma_mode;
	unsigned int			current_bl;

	unsigned int			ldi_enable;
	unsigned int			power;
	struct mutex			lock;

	unsigned int			panel_initialized;
	unsigned int			partial_mode;

	struct device			*dev;
	struct lcd_device		*ld;
	struct backlight_device		*bd;
	struct lcd_platform_data	*lcd_pd;
	struct early_suspend		early_suspend;
};

static struct mipi_ddi_platform_data *ddi_pd;

static int s6e8ax0_write(struct lcd_info *lcd, const unsigned char *wbuf, int size)
{
	if (size == 1)
		ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_NO_PARA, wbuf[0], 0);
	else if (size == 2)
		ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_1_PARA, wbuf[0], wbuf[1]);
	else
		ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int)wbuf, size);

	return 0;
}

static int s6e8ax0_gamma_ctl(struct lcd_info *lcd)
{
	if (lcd->gamma_mode)
		s6e8ax0_write(lcd, gamma19_table[lcd->bl], GAMMA_PARAM_SIZE);
	else
		s6e8ax0_write(lcd, gamma22_table[lcd->bl], GAMMA_PARAM_SIZE);

	/* Gamma Set Update */
	s6e8ax0_write(lcd, SEQ_GAMMA_UPDATE, ARRAY_SIZE(SEQ_GAMMA_UPDATE));

	lcd->current_bl = lcd->bl;
	lcd->current_gamma_mode = lcd->gamma_mode;

	return 0;
}

static int s6e8ax0_set_link(void *pd, unsigned int dsim_base,
	unsigned char (*cmd_write) (unsigned int dsim_base, unsigned int data0,
	    unsigned int data1, unsigned int data2),
	unsigned char (*cmd_read) (unsigned int dsim_base, unsigned int data0,
	    unsigned int data1, unsigned int data2))
{
	struct mipi_ddi_platform_data *temp_pd = NULL;

	temp_pd = (struct mipi_ddi_platform_data *) pd;
	if (temp_pd == NULL) {
		printk(KERN_ERR "mipi_ddi_platform_data is null.\n");
		return -1;
	}

	ddi_pd = temp_pd;

	ddi_pd->dsim_base = dsim_base;

	if (cmd_write)
		ddi_pd->cmd_write = cmd_write;
	else
		printk(KERN_WARNING "cmd_write function is null.\n");

	if (cmd_read)
		ddi_pd->cmd_read = cmd_read;
	else
		printk(KERN_WARNING "cmd_read function is null.\n");

	return 0;
}

static void panel_acl_send_sequence(struct lcd_info *lcd, const unsigned char *acl_param_tbl)
{
	s6e8ax0_write(lcd, SEQ_APPLY_LEVEL_2_KEY, ARRAY_SIZE(SEQ_APPLY_LEVEL_2_KEY));

	s6e8ax0_write(lcd, acl_param_tbl, ACL_PARAM_SIZE);

	s6e8ax0_write(lcd, SEQ_ACL_ON, ARRAY_SIZE(SEQ_ACL_ON));
}


static int s6e8ax0_set_elvss(struct lcd_info *lcd)
{
	int ret = 0;

	switch (lcd->bl) {
	case 0 ... 4: /* 30cd ~ 100cd */
		ret = s6e8ax0_write(lcd, elvss_table[0], ELVSS_PARAM_SIZE);
		break;
	case 5 ... 10: /* 110cd ~ 160cd */
		ret = s6e8ax0_write(lcd, elvss_table[1], ELVSS_PARAM_SIZE);
		break;
	case 11 ... 14: /* 170cd ~ 200cd */
		ret = s6e8ax0_write(lcd, elvss_table[2], ELVSS_PARAM_SIZE);
		break;
	case 15 ... 24: /* 210cd ~ 300cd */
		ret = s6e8ax0_write(lcd, elvss_table[3], ELVSS_PARAM_SIZE);
		break;
	default:
		break;
	}

	dev_dbg(&lcd->ld->dev, "level  = %d\n", lcd->bl);

	if (ret) {
		ret = -EPERM;
		goto elvss_err;
	}

elvss_err:
	return ret;
}

static int s6e8ax0_set_acl(struct lcd_info *lcd)
{
	int ret = 0;

	if (lcd->acl_enable) {
		if (lcd->cur_acl == 0) {
			if (lcd->bl == 0 || lcd->bl == 1) {
				s6e8ax0_write(lcd, SEQ_ACL_OFF, ARRAY_SIZE(SEQ_ACL_OFF));
				dev_dbg(lcd->dev, "ACL_cutoff_set Percentage : off!!\n");
			} else
				s6e8ax0_write(lcd, SEQ_ACL_ON, ARRAY_SIZE(SEQ_ACL_ON));
		}
		switch (lcd->bl) {
		case 0 ... 1: /* 30cd ~ 40cd */
			if (lcd->cur_acl != 0) {
				s6e8ax0_write(lcd, SEQ_ACL_OFF, ARRAY_SIZE(SEQ_ACL_OFF));
				dev_dbg(&lcd->ld->dev, "ACL_cutoff_set Percentage : off!!\n");
				lcd->cur_acl = 0;
			}
			break;
		case 2 ... 12: /* 70cd ~ 180cd */
			if (lcd->cur_acl != 40) {
				panel_acl_send_sequence(lcd, acl_cutoff_table[1]);
				dev_dbg(&lcd->ld->dev, "ACL_cutoff_set Percentage : 40!!\n");
				lcd->cur_acl = 40;
			}
			break;
		case 13: /* 190cd */
			if (lcd->cur_acl != 43) {
				panel_acl_send_sequence(lcd, acl_cutoff_table[2]);
				dev_dbg(&lcd->ld->dev, "ACL_cutoff_set Percentage : 43!!\n");
				lcd->cur_acl = 43;
			}
			break;
		case 14: /* 200cd */
			if (lcd->cur_acl != 45) {
				panel_acl_send_sequence(lcd, acl_cutoff_table[3]);
				dev_dbg(&lcd->ld->dev, "ACL_cutoff_set Percentage : 45!!\n");
				lcd->cur_acl = 45;
			}
			break;
		case 15: /* 210cd */
			if (lcd->cur_acl != 47) {
				panel_acl_send_sequence(lcd, acl_cutoff_table[4]);
				dev_dbg(&lcd->ld->dev, "ACL_cutoff_set Percentage : 47!!\n");
				lcd->cur_acl = 47;
			}
			break;
		case 16: /* 220cd */
			if (lcd->cur_acl != 48) {
				panel_acl_send_sequence(lcd, acl_cutoff_table[5]);
				dev_dbg(&lcd->ld->dev, "ACL_cutoff_set Percentage : 48!!\n");
				lcd->cur_acl = 48;
			}
			break;
		default:
			if (lcd->cur_acl != 50) {
				panel_acl_send_sequence(lcd, acl_cutoff_table[6]);
				dev_dbg(&lcd->ld->dev, "ACL_cutoff_set Percentage : 50!!\n");
				lcd->cur_acl = 50;
			}
			break;
		}
	} else {
		s6e8ax0_write(lcd, SEQ_ACL_OFF, ARRAY_SIZE(SEQ_ACL_OFF));
		lcd->cur_acl = 0;
		dev_dbg(&lcd->ld->dev, "ACL_cutoff_set Percentage : off!!\n");
	}

	if (ret) {
		ret = -EPERM;
		goto acl_err;
	}

acl_err:
	return ret;
}

static int update_brightness(struct lcd_info *lcd)
{
	int ret;

	ret = s6e8ax0_gamma_ctl(lcd);
	if (ret)
		return -EPERM;

	ret = s6e8ax0_set_elvss(lcd);
	if (ret)
		return -EPERM;

	ret = s6e8ax0_set_acl(lcd);
	if (ret)
		return -EPERM;

	return 0;
}

static int s6e8ax0_ldi_init(struct lcd_info *lcd)
{
	int ret = 0;

	s6e8ax0_write(lcd, SEQ_APPLY_LEVEL_2_KEY, ARRAY_SIZE(SEQ_APPLY_LEVEL_2_KEY));
	s6e8ax0_write(lcd, SEQ_SLEEP_OUT, ARRAY_SIZE(SEQ_SLEEP_OUT));
	s6e8ax0_write(lcd, SEQ_PANEL_CONDITION_SET, ARRAY_SIZE(SEQ_PANEL_CONDITION_SET));
	s6e8ax0_write(lcd, SEQ_DISPLAY_CONDITION_SET, ARRAY_SIZE(SEQ_DISPLAY_CONDITION_SET));
	s6e8ax0_write(lcd, SEQ_GAMMA_CONDITION_SET, ARRAY_SIZE(SEQ_GAMMA_CONDITION_SET));
	s6e8ax0_write(lcd, SEQ_GAMMA_UPDATE, ARRAY_SIZE(SEQ_GAMMA_UPDATE));
	s6e8ax0_write(lcd, SEQ_ETC_SOURCE_CONTROL, ARRAY_SIZE(SEQ_ETC_SOURCE_CONTROL));
	s6e8ax0_write(lcd, SEQ_ETC_ELVSS_CONTROL, ARRAY_SIZE(SEQ_ETC_ELVSS_CONTROL));
	s6e8ax0_write(lcd, SEQ_ETC_PENTILE_CONTROL, ARRAY_SIZE(SEQ_ETC_PENTILE_CONTROL));
	s6e8ax0_write(lcd, SEQ_ETC_MIPI_CONTROL1, ARRAY_SIZE(SEQ_ETC_MIPI_CONTROL1));
	s6e8ax0_write(lcd, SEQ_ETC_MIPI_CONTROL2, ARRAY_SIZE(SEQ_ETC_MIPI_CONTROL2));
	s6e8ax0_write(lcd, SEQ_ETC_POWER_CONTROL, ARRAY_SIZE(SEQ_ETC_POWER_CONTROL));
	s6e8ax0_write(lcd, SEQ_ETC_MIPI_CONTROL3, ARRAY_SIZE(SEQ_ETC_MIPI_CONTROL3));
	s6e8ax0_write(lcd, SEQ_ETC_MIPI_CONTROL4, ARRAY_SIZE(SEQ_ETC_MIPI_CONTROL4));

	return ret;
}

static int s6e8ax0_ldi_enable(struct lcd_info *lcd)
{
	int ret = 0;

	s6e8ax0_write(lcd, SEQ_DISPLAY_ON, ARRAY_SIZE(SEQ_DISPLAY_ON));

	return ret;
}

static int s6e8ax0_ldi_disable(struct lcd_info *lcd)
{
	int ret = 0;

	s6e8ax0_write(lcd, SEQ_DISPLAY_OFF, ARRAY_SIZE(SEQ_DISPLAY_OFF));
	s6e8ax0_write(lcd, SEQ_STANDBY_ON, ARRAY_SIZE(SEQ_STANDBY_ON));

	return ret;
}

static int s6e8ax0_power_on(struct lcd_info *lcd)
{
	int ret = 0;
	struct lcd_platform_data *pd = NULL;
	pd = lcd->lcd_pd;

	dev_info(&lcd->ld->dev, "%s\n", __func__);

	ret = s6e8ax0_ldi_init(lcd);

	if (ret) {
		dev_err(&lcd->ld->dev, "failed to initialize ldi.\n");
		goto err;
	}

	mdelay(120);

	ret = s6e8ax0_ldi_enable(lcd);
	if (ret) {
		dev_err(&lcd->ld->dev, "failed to enable ldi.\n");
		goto err;
	}

	update_brightness(lcd);

	lcd->ldi_enable = 1;

err:
	return ret;
}

static int s6e8ax0_power_off(struct lcd_info *lcd)
{
	int ret = 0;

	dev_info(&lcd->ld->dev, "%s\n", __func__);

	lcd->ldi_enable = 0;

	msleep(60);

	ret = s6e8ax0_ldi_disable(lcd);

	mdelay(120);

	return ret;
}

static int s6e8ax0_power(struct lcd_info *lcd, int power)
{
	int ret = 0;

	if (POWER_IS_ON(power) && !POWER_IS_ON(lcd->power))
		ret = s6e8ax0_power_on(lcd);
	else if (!POWER_IS_ON(power) && POWER_IS_ON(lcd->power))
		ret = s6e8ax0_power_off(lcd);

	if (!ret)
		lcd->power = power;

	return ret;
}

static int s6e8ax0_set_power(struct lcd_device *ld, int power)
{
	struct lcd_info *lcd = lcd_get_data(ld);

	if (power != FB_BLANK_UNBLANK && power != FB_BLANK_POWERDOWN &&
		power != FB_BLANK_NORMAL) {
		dev_err(&lcd->ld->dev, "power value should be 0, 1 or 4.\n");
		return -EINVAL;
	}

	return s6e8ax0_power(lcd, power);
}

static int s6e8ax0_get_power(struct lcd_device *ld)
{
	struct lcd_info *lcd = lcd_get_data(ld);

	return lcd->power;
}

static int get_backlight_level_from_brightness(int brightness)
{
	int backlightlevel;

	/* brightness setting from platform is from 0 to 255
	 * But in this driver, brightness is only supported from 0 to 24 */

	switch (brightness) {
	case 0:
		backlightlevel = 0;
		break;
	case 1 ... 29:
		backlightlevel = 0;
		break;
	case 30 ... 34:
		backlightlevel = 1;
		break;
	case 35 ... 44:
		backlightlevel = 2;
		break;
	case 45 ... 54:
		backlightlevel = 3;
		break;
	case 55 ... 64:
		backlightlevel = 4;
		break;
	case 65 ... 74:
		backlightlevel = 5;
		break;
	case 75 ... 83:
		backlightlevel = 6;
		break;
	case 84 ... 93:
		backlightlevel = 7;
		break;
	case 94 ... 103:
		backlightlevel = 8;
		break;
	case 104 ... 113:
		backlightlevel = 9;
		break;
	case 114 ... 122:
		backlightlevel = 10;
		break;
	case 123 ... 132:
		backlightlevel = 11;
		break;
	case 133 ... 142:
		backlightlevel = 12;
		break;
	case 143 ... 152:
		backlightlevel = 13;
		break;
	case 153 ... 162:
		backlightlevel = 14;
		break;
	case 163 ... 171:
		backlightlevel = 15;
		break;
	case 172 ... 181:
		backlightlevel = 16;
		break;
	case 182 ... 191:
		backlightlevel = 17;
		break;
	case 192 ... 201:
		backlightlevel = 18;
		break;
	case 202 ... 210:
		backlightlevel = 19;
		break;
	case 211 ... 220:
		backlightlevel = 20;
		break;
	case 221 ... 230:
		backlightlevel = 21;
		break;
	case 231 ... 240:
		backlightlevel = 22;
		break;
	case 241 ... 250:
		backlightlevel = 23;
		break;
	case 251 ... 255:
		backlightlevel = 24;
		break;
	default:
		backlightlevel = 10;
		break;
	}
	return backlightlevel;
}

static int s6e8ax0_set_brightness(struct backlight_device *bd)
{
	int ret = 0;
	int brightness = bd->props.brightness;
	struct lcd_info *lcd = bl_get_data(bd);

	if (brightness < MIN_BRIGHTNESS ||
		brightness > bd->props.max_brightness) {
		dev_err(&bd->dev, "lcd brightness should be %d to %d. now %d\n",
			MIN_BRIGHTNESS, MAX_BRIGHTNESS, brightness);
		return -EINVAL;
	}

	lcd->bl = get_backlight_level_from_brightness(brightness);

	if ((lcd->ldi_enable) && (lcd->current_bl != lcd->bl)) {
		ret = update_brightness(lcd);
		dev_info(&lcd->ld->dev, "brightness=%d, bl=%d\n", bd->props.brightness, lcd->bl);
		if (ret < 0)
			return -EINVAL;
	}

	return ret;
}

static int s6e8ax0_get_brightness(struct backlight_device *bd)
{
	return bd->props.brightness;
}

static struct lcd_ops s6e8ax0_lcd_ops = {
	.set_power = s6e8ax0_set_power,
	.get_power = s6e8ax0_get_power,
};

static const struct backlight_ops s6e8ax0_backlight_ops  = {
	.get_brightness = s6e8ax0_get_brightness,
	.update_status = s6e8ax0_set_brightness,
};

static ssize_t acl_enable_show(struct device *dev, struct
device_attribute *attr, char *buf)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);
	char temp[3];

	sprintf(temp, "%d\n", lcd->acl_enable);
	strcpy(buf, temp);

	return strlen(buf);
}
static ssize_t acl_enable_store(struct device *dev, struct
device_attribute *attr, const char *buf, size_t size)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);
	int value;
	int rc;

	rc = strict_strtoul(buf, (unsigned int)0, (unsigned long *)&value);
	if (rc < 0)
		return rc;
	else {
		if (lcd->acl_enable != value) {
			dev_info(dev, "%s - %d, %d\n", __func__, lcd->acl_enable, value);
			lcd->acl_enable = value;
			if (lcd->ldi_enable)
				s6e8ax0_set_acl(lcd);
		}
		return 0;
	}
}

static DEVICE_ATTR(acl_set, 0664, acl_enable_show, acl_enable_store);

static ssize_t gamma_mode_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);
	char temp[10];

	switch (lcd->gamma_mode) {
	case 0:
		sprintf(temp, "2.2 mode\n");
		strcat(buf, temp);
		break;
	case 1:
		sprintf(temp, "1.9 mode\n");
		strcat(buf, temp);
		break;
	default:
		dev_info(dev, "gamma mode should be 0:2.2, 1:1.9)\n");
		break;
	}

	return strlen(buf);
}

static ssize_t gamma_mode_store(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t len)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);
	int rc;

	rc = strict_strtoul(buf, 0, (unsigned long *)&lcd->gamma_mode);

	if (rc < 0)
		return rc;

	if (lcd->gamma_mode > 1) {
		lcd->gamma_mode = 0;
		dev_err(dev, "there are only 2 types of gamma mode(0:2.2, 1:1.9)\n");
	} else
		dev_info(dev, "%s :: gamma_mode=%d\n", __func__, lcd->gamma_mode);

	if (lcd->ldi_enable) {
		if ((lcd->current_bl == lcd->bl) && (lcd->current_gamma_mode == lcd->gamma_mode))
			dev_err(dev, "there is no gamma_mode & brightness changed\n");
		else
			s6e8ax0_gamma_ctl(lcd);
	}
	return len;
}

static DEVICE_ATTR(gamma_mode, 0664, gamma_mode_show, gamma_mode_store);


#ifdef CONFIG_HAS_EARLYSUSPEND
void s6e8ax0_early_suspend(struct early_suspend *h)
{
	struct lcd_info *lcd = container_of(h, struct lcd_info, early_suspend);

	dev_info(&lcd->ld->dev, "+%s\n", __func__);
	s6e8ax0_power(lcd, FB_BLANK_POWERDOWN);
	dev_info(&lcd->ld->dev, "-%s\n", __func__);

	return ;
}

void s6e8ax0_late_resume(struct early_suspend *h)
{
	struct lcd_info *lcd = container_of(h, struct lcd_info, early_suspend);

	dev_info(&lcd->ld->dev, "+%s\n", __func__);
	s6e8ax0_power(lcd, FB_BLANK_UNBLANK);
	dev_info(&lcd->ld->dev, "-%s\n", __func__);

	return ;
}
#endif

static int s6e8ax0_probe(struct device *dev)
{
	int ret = 0;
	struct lcd_info *lcd;

	lcd = kzalloc(sizeof(struct lcd_info), GFP_KERNEL);
	if (!lcd) {
		pr_err("failed to allocate for lcd\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

	lcd->ld = lcd_device_register("lcd", dev,
		lcd, &s6e8ax0_lcd_ops);
	if (IS_ERR(lcd->ld)) {
		pr_err("failed to register lcd device\n");
		ret = PTR_ERR(lcd->ld);
		goto out_free_lcd;
	}

	lcd->bd = backlight_device_register("pwm-backlight", dev,
		lcd, &s6e8ax0_backlight_ops, NULL);
	if (IS_ERR(lcd->bd)) {
		pr_err("failed to register backlight device\n");
		ret = PTR_ERR(lcd->bd);
		goto out_free_backlight;
	}

	lcd->dev = dev;
	lcd->bd->props.max_brightness = MAX_BRIGHTNESS;
	lcd->bd->props.brightness = DEFAULT_BRIGHTNESS;
	lcd->bl = DEFAULT_GAMMA_LEVEL;
	lcd->current_bl = lcd->bl;
	lcd->gamma_mode = 0;
	lcd->current_gamma_mode = 0;

	lcd->acl_enable = 0;
	lcd->cur_acl = 0;

	lcd->power = FB_BLANK_UNBLANK;
	lcd->ldi_enable = 1;

	lcd->panel_initialized = 0;
	lcd->partial_mode = 0;

	ret = device_create_file(lcd->dev, &dev_attr_gamma_mode);
	if (ret < 0)
		dev_err(&lcd->ld->dev, "failed to add sysfs entries\n");

	ret = device_create_file(lcd->dev, &dev_attr_acl_set);
	if (ret < 0)
		dev_err(&lcd->ld->dev, "failed to add sysfs entries\n");

	dev_set_drvdata(dev, lcd);

#ifdef CONFIG_HAS_EARLYSUSPEND
	lcd->early_suspend.suspend = s6e8ax0_early_suspend;
	lcd->early_suspend.resume = s6e8ax0_late_resume;
	lcd->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1;
	register_early_suspend(&lcd->early_suspend);
#endif

	mutex_init(&lcd->lock);

	dev_info(&lcd->ld->dev, "s6e8ax0 lcd panel driver has been probed.\n");

	return 0;

out_free_backlight:
	lcd_device_unregister(lcd->ld);
	kfree(lcd);
	return ret;
out_free_lcd:
	kfree(lcd);
	return ret;
err_alloc:
	return ret;
}


static int __devexit s6e8ax0_remove(struct device *dev)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);

	s6e8ax0_power(lcd, FB_BLANK_POWERDOWN);
	lcd_device_unregister(lcd->ld);
	backlight_device_unregister(lcd->bd);
	kfree(lcd);

	return 0;
}

/* Power down all displays on reboot, poweroff or halt. */
static void s6e8ax0_shutdown(struct device *dev)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);

	s6e8ax0_power(lcd, FB_BLANK_POWERDOWN);
}

static struct mipi_lcd_driver s6e8ax0_mipi_driver = {
	.name = "s6e8aa0",
//	.init			= s6e8ax0_panel_init,
//	.display_on		= s6e8ax0_display_on,
//	.display_off		= s6e8ax0_display_off,
	.set_link		= s6e8ax0_set_link,
	.probe			= s6e8ax0_probe,
	.remove			= __devexit_p(s6e8ax0_remove),
	.shutdown		= s6e8ax0_shutdown,

#if 0
	/* add lcd partial and dim mode functionalies */
	.partial_mode_status = s6e8ax0_partial_mode_status,
	.partial_mode_on	= s6e8ax0_partial_mode_on,
	.partial_mode_off	= s6e8ax0_partial_mode_off,
#endif
};

static int s6e8ax0_init(void)
{
	s5p_dsim_register_lcd_driver(&s6e8ax0_mipi_driver);

	return 0;
}

static void s6e8ax0_exit(void)
{
	return;
}

module_init(s6e8ax0_init);
module_exit(s6e8ax0_exit);

MODULE_DESCRIPTION("MIPI-DSI S6E8AA0:AMS529HA01 (800x1280) Panel Driver");
MODULE_LICENSE("GPL");

