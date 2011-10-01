/*
 * ld9040 AMOLED LCD panel driver.
 *
 * Author: Donghwa Lee  <dh09.lee@samsung.com>
 *
 * Derived from drivers/video/omap/lcd-apollon.c
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <linux/wait.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/lcd.h>
#include <linux/backlight.h>
#include <linux/serial_core.h>
#include <plat/regs-serial.h>
#include <plat/s5pv310.h>
#include <linux/ld9040.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#define MAX_GAMMA_LEVEL		25
#define GAMMA_TABLE_COUNT		21

#define SLEEPMSEC		0x1000
#define ENDDEF			0x2000
#define	DEFMASK			0xFF00
#define COMMAND_ONLY		0xFE
#define DATA_ONLY		0xFF

#define MIN_BRIGHTNESS		0
#define MAX_BRIGHTNESS		255
#define POWER_IS_ON(pwr)	((pwr) <= FB_BLANK_NORMAL)

static unsigned int get_lcdtype;
module_param_named(get_lcdtype, get_lcdtype, uint, 0444);
MODULE_PARM_DESC(get_lcdtype, " get_lcdtype  in Bootloader");
struct ld9040 {
	struct device			*dev;
	struct spi_device		*spi;
	unsigned int			power;
	unsigned int			gamma_mode;
	unsigned int			current_brightness;
	unsigned int			gamma_table_count;
	unsigned int			bl;
	unsigned int			beforepower;
	unsigned int			ldi_enable;
	unsigned int 			acl_enable;
	unsigned int 			cur_acl;
	struct mutex	lock;
	struct lcd_device		*ld;
	struct backlight_device		*bd;
	struct lcd_platform_data	*lcd_pd;
	struct early_suspend    early_suspend;
};

static int ld9040_spi_write_byte(struct ld9040 *lcd, int addr, int data)
{
	u16 buf[1];
	struct spi_message msg;

	struct spi_transfer xfer = {
		.len		= 2,
		.tx_buf		= buf,
	};

	buf[0] = (addr << 8) | data;

	spi_message_init(&msg);
	spi_message_add_tail(&xfer, &msg);

	return spi_sync(lcd->spi, &msg);
}

static int ld9040_spi_write(struct ld9040 *lcd,
	unsigned char address, unsigned char command)
{
	int ret = 0;

	if (address != DATA_ONLY)
		ret = ld9040_spi_write_byte(lcd, 0x0, address);
	if (command != COMMAND_ONLY)
		ret = ld9040_spi_write_byte(lcd, 0x1, command);

	return ret;
}

static int ld9040_panel_send_sequence(struct ld9040 *lcd,
	const unsigned short *wbuf)
{
	int ret = 0, i = 0;

	while ((wbuf[i] & DEFMASK) != ENDDEF) {
		if ((wbuf[i] & DEFMASK) != SLEEPMSEC) {
			ret = ld9040_spi_write(lcd, wbuf[i], wbuf[i+1]);
			if (ret)
				break;
		} else
			udelay(wbuf[i+1]*1000);
		i += 2;
	}

	return ret;
}
static int get_gamma_value_from_bl(int bl)
{
	int backlightlevel;

	/* brightness setting from platform is from 0 to 255
	 * But in this driver, brightness is only supported from 0 to 24 */

	switch(bl){
		case 0 ... 10:
			backlightlevel=0;
			break;
		case 11 ... 21:
			backlightlevel=1;
			break;
		case 22 ... 32:
			backlightlevel=2;
			break;
		case 33 ... 42:
			backlightlevel=3;
			break;
		case 43 ... 52:
			backlightlevel=4;
			break;
		case 53 ... 63:
			backlightlevel=5;
			break;
		case 64 ... 73:
			backlightlevel=6;
			break;
		case 74 ... 83:
			backlightlevel=7;
			break;
		case 84 ... 95:
			backlightlevel=8;
			break;
		case 96 ... 105:
			backlightlevel=9;
			break;
		case 106 ... 116:
			backlightlevel=10;
			break;
		case 117 ... 127:
			backlightlevel=11;
			break;
		case 128 ... 136:
			backlightlevel=12;
			break;
		case 137 ... 146:
			backlightlevel=13;
			break;
		case 147 ... 156:
			backlightlevel=14;
			break;
		case 157 ... 169:
			backlightlevel=15;
			break;
		case 170 ... 180:
			backlightlevel=16;
			break;
		case 181 ... 191:
			backlightlevel=17;
			break;
		case 192 ... 201:
			backlightlevel=18;
			break;
		case 202 ... 211:
			backlightlevel=19;
			break;
		case 212 ... 223:
			backlightlevel=20;
			break;
		case 224 ... 233:
			backlightlevel=21;
			break;
		case 234 ... 243:
			backlightlevel=22;
			break;
		case 244 ... 249:
			backlightlevel=23;
			break;
		case 250 ... 255:
			backlightlevel=24;
			break;
		default :
			break;
	}
	return backlightlevel;

}
static int ld9040_gamma_ctl(struct ld9040 *lcd)
{
	int ret = 0;
	const unsigned short *gamma;
	struct ld9040_panel_data *pdata = lcd->lcd_pd->pdata;

	if(get_lcdtype==1){ /* M2 */
		if (lcd->gamma_mode)
			gamma = pdata->gamma19_table[lcd->bl];
		else
			gamma = pdata->gamma22_table[lcd->bl];
	}
	else { /* SM2 */
		if (lcd->gamma_mode)
			gamma = pdata->gamma_sm2_19_table[lcd->bl];
		else
			gamma = pdata->gamma_sm2_22_table[lcd->bl];
	}
	/* start gamma table updating. */
	ret = ld9040_panel_send_sequence(lcd, pdata->gamma_start);
	if (ret) {
		dev_err(lcd->dev, "failed to disable gamma table updating.\n");
		goto gamma_err;
	}
	ret = ld9040_panel_send_sequence(lcd, gamma);
	if (ret) {
		dev_err(lcd->dev, "failed to disable gamma table updating.\n");
		goto gamma_err;
	}
	/* update gamma table. */
	ret = ld9040_panel_send_sequence(lcd, pdata->gamma_ctrl);
	if (ret)
		dev_err(lcd->dev, "failed to update gamma table.\n");

gamma_err:
	return ret;
}

static int ld9040_set_elvss(struct ld9040 *lcd)
{
	int ret = 0;
	struct ld9040_panel_data *pdata = lcd->lcd_pd->pdata;
	if(get_lcdtype){  /* for M2 */
		switch (lcd->bl) {
		case 0 ... 4: /* 30cd ~ 100cd */
			ret = ld9040_panel_send_sequence(lcd, pdata->elvss_table[0]);
			break;
		case 5 ... 10: /* 110cd ~ 160cd */
			ret = ld9040_panel_send_sequence(lcd, pdata->elvss_table[1]);
			break;
		case 11 ... 14: /* 170cd ~ 200cd */
			ret = ld9040_panel_send_sequence(lcd, pdata->elvss_table[2]);
			break;
		case 15 ... 24: /* 210cd ~ 300cd */
			ret = ld9040_panel_send_sequence(lcd, pdata->elvss_table[3]);
			break;
		default:
			break;
		}
	}
	else{/* for SM2 */
		switch (lcd->bl) {
		case 0 ... 4: /* 30cd ~ 100cd */
			ret = ld9040_panel_send_sequence(lcd, pdata->elvss_sm2_table[0]);
			break;
		case 5 ... 10: /* 110cd ~ 160cd */
			ret = ld9040_panel_send_sequence(lcd, pdata->elvss_sm2_table[1]);
			break;
		case 11 ... 14: /* 170cd ~ 200cd */
			ret = ld9040_panel_send_sequence(lcd, pdata->elvss_sm2_table[2]);
			break;
		case 15 ... 24: /* 210cd ~ 300cd */
			ret = ld9040_panel_send_sequence(lcd, pdata->elvss_sm2_table[3]);
			break;
		default:
			break;
		}
	}
	dev_dbg(lcd->dev, "level  = %d\n", lcd->bl);

	if (ret) {
		dev_err(lcd->dev, "failed to initialize ldi.\n");
		return -EIO;
	}
	return ret;
}

static int ld9040_set_acl(struct ld9040 *lcd)
{
	int ret = 0;
	struct ld9040_panel_data *pdata = lcd->lcd_pd->pdata;

	if (lcd->acl_enable) {
		if(lcd->cur_acl == 0)  {
			ret = ld9040_panel_send_sequence(lcd, pdata->acl_on);
		}
		switch (lcd->bl) {
		case 0 ... 1: /* 30cd ~ 40cd */
			if (lcd->cur_acl != 0) {
				ret = ld9040_panel_send_sequence(lcd, pdata->acl_table[0]);
				dev_dbg(lcd->dev, "ACL_cutoff_set Percentage : off!!\n");
				lcd->cur_acl = 0;
			}
			break;
		case 2 ... 12: /* 70cd ~ 180cd */
			if (lcd->cur_acl != 40) {
				ret = ld9040_panel_send_sequence(lcd, pdata->acl_table[1]);
				dev_dbg(lcd->dev, "ACL_cutoff_set Percentage : 40!!\n");
				lcd->cur_acl = 40;
			}
			break;
		case 13: /* 190cd */
			if (lcd->cur_acl != 43) {
				ret = ld9040_panel_send_sequence(lcd, pdata->acl_table[2]);
				dev_dbg(lcd->dev, "ACL_cutoff_set Percentage : 43!!\n");
				lcd->cur_acl = 43;
			}
			break;
		case 14: /* 200cd */
			if (lcd->cur_acl != 45) {
				ret = ld9040_panel_send_sequence(lcd, pdata->acl_table[3]);
				dev_dbg(lcd->dev, "ACL_cutoff_set Percentage : 45!!\n");
				lcd->cur_acl = 45;
			}
			break;
		case 15: /* 210cd */
			if (lcd->cur_acl != 47) {
				ret = ld9040_panel_send_sequence(lcd, pdata->acl_table[4]);
				dev_dbg(lcd->dev, "ACL_cutoff_set Percentage : 47!!\n");
				lcd->cur_acl = 47;
			}
			break;
		case 16: /* 220cd */
			if (lcd->cur_acl != 48) {
				ret = ld9040_panel_send_sequence(lcd, pdata->acl_table[5]);
				dev_dbg(lcd->dev, "ACL_cutoff_set Percentage : 48!!\n");
				lcd->cur_acl = 48;
			}
			break;
		default:
			if (lcd->cur_acl != 50) {
				ret = ld9040_panel_send_sequence(lcd, pdata->acl_table[6]);
				dev_dbg(lcd->dev, "ACL_cutoff_set Percentage : 50!!\n");
				lcd->cur_acl = 50;
			}
			break;
		}
	}
	else{
			ret = ld9040_panel_send_sequence(lcd, pdata->acl_table[0]);
			lcd->cur_acl = 0;
			dev_dbg(lcd->dev, "ACL_cutoff_set Percentage : off!!\n");
	}

	if (ret) {
		dev_err(lcd->dev, "failed to initialize ldi.\n");
		return -EIO;
	}
	return ret;
}

static int ld9040_ldi_init(struct ld9040 *lcd)
{
	int ret, i;
	struct ld9040_panel_data *pdata = lcd->lcd_pd->pdata;
	if(get_lcdtype){  /* for M2 */
		const unsigned short *init_seq[] = {
			pdata->seq_user_set,
			pdata->seq_displayctl_set,
			pdata->seq_gtcon_set,
			pdata->seq_panelcondition_set,
			pdata->acl_on,
			pdata->sleep_out,
			pdata->elvss_on,
			pdata->seq_pwrctl_set,
			pdata->seq_gamma_set1,
			pdata->gamma_ctrl,
		};
		for (i = 0; i < ARRAY_SIZE(init_seq); i++) {
			ret = ld9040_panel_send_sequence(lcd, init_seq[i]);
			mdelay(5);
			if (ret)
				break;
		}

	}
	else { /* for SM2 */
		const unsigned short *init_seq_sm2[] = {
			pdata->seq_user_set,
			pdata->seq_displayctl_set,
			pdata->seq_gtcon_set,
			pdata->seq_panelcondition_set,
			pdata->acl_on,
			pdata->sleep_out,
			pdata->elvss_on,
			pdata->seq_pwrctl_set,
			pdata->seq_sm2_gamma_set1,
			pdata->gamma_ctrl,
		};
		for (i = 0; i < ARRAY_SIZE(init_seq_sm2); i++) {
			ret = ld9040_panel_send_sequence(lcd, init_seq_sm2[i]);
			mdelay(5);
			if (ret)
				break;
		}

	}
	return ret;
}

static int ld9040_ldi_enable(struct ld9040 *lcd)
{
	int ret = 0;
	struct ld9040_panel_data *pdata = lcd->lcd_pd->pdata;

	ret = ld9040_panel_send_sequence(lcd, pdata->display_on);

	return ret;
}

static int ld9040_ldi_disable(struct ld9040 *lcd)
{
	int ret;
	struct ld9040_panel_data *pdata = lcd->lcd_pd->pdata;

	ret = ld9040_panel_send_sequence(lcd, pdata->display_off);
	ret = ld9040_panel_send_sequence(lcd, pdata->sleep_in);

	return ret;
}

static int update_brightness(struct ld9040 *lcd)
{
	int ret;

	ret = ld9040_set_elvss(lcd);
	if (ret) {
		dev_err(lcd->dev, "lcd brightness setting failed.\n");
		return -EIO;
	}

	ret = ld9040_set_acl(lcd);
	if (ret) {
		dev_err(lcd->dev, "lcd brightness setting failed.\n");
		return -EIO;
	}

	ret = ld9040_gamma_ctl(lcd);
	if (ret) {
		dev_err(lcd->dev, "lcd brightness setting failed.\n");
		return -EIO;
	}

	return 0;
}


static int ld9040_power_on(struct ld9040 *lcd)
{
	int ret = 0;
	struct lcd_platform_data *pd = NULL;
	pd = lcd->lcd_pd;

	if (!pd) {
		dev_err(lcd->dev, "platform data is NULL.\n");
		return -EFAULT;
	}

	if (!pd->power_on) {
		dev_err(lcd->dev, "power_on is NULL.\n");
		return -EFAULT;
	} else {
		pd->power_on(lcd->ld, 1);
		mdelay(pd->power_on_delay);
	}
#if 0
	if (!pd->gpio_cfg_lateresume) {
		dev_err(lcd->dev, "gpio_cfg_lateresume is NULL.\n");
		ret = -EFAULT;
		goto err;
	} else
		pd->gpio_cfg_lateresume(lcd->ld);
#endif

	if (!pd->reset) {
		dev_err(lcd->dev, "reset is NULL.\n");
		return -EFAULT;
	} else {
		pd->reset(lcd->ld);
		mdelay(pd->reset_delay);
	}


	ret = ld9040_ldi_init(lcd);

	if (ret) {
		dev_err(lcd->dev, "failed to initialize ldi.\n");
		goto err;
	}

	ret = ld9040_ldi_enable(lcd);
	if (ret) {
		dev_err(lcd->dev, "failed to enable ldi.\n");
		goto err;
	}

	update_brightness(lcd);

	lcd->ldi_enable = 1;
err:

	return ret;;
}

static int ld9040_power_off(struct ld9040 *lcd)
{
	int ret = 0;
	struct lcd_platform_data *pd = NULL;

	pd = lcd->lcd_pd;
	if (!pd) {
		dev_err(lcd->dev, "platform data is NULL.\n");
		return -EFAULT;
	}

	mdelay(pd->sleep_in_delay);


	ret = ld9040_ldi_disable(lcd);
	if (ret) {
		dev_err(lcd->dev, "lcd setting failed.\n");
		ret = -EIO;
		goto err;
	}
	mdelay(pd->power_off_delay);

	if (!pd->gpio_cfg_earlysuspend) {
		dev_err(lcd->dev, "gpio_cfg_earlysuspend is NULL.\n");
		ret = -EFAULT;
		goto err;
	} else
		pd->gpio_cfg_earlysuspend(lcd->ld);

	if (!pd->power_on) {
		dev_err(lcd->dev, "power_on is NULL.\n");
		ret = -EFAULT;
		goto err;
	} else {
		pd->power_on(lcd->ld, 0);
	}

	lcd->ldi_enable = 0;

err:
	return ret;
}

static int ld9040_power(struct ld9040 *lcd, int power)
{
	int ret = 0;

	if (POWER_IS_ON(power) && !POWER_IS_ON(lcd->power))
		ret = ld9040_power_on(lcd);
	else if (!POWER_IS_ON(power) && POWER_IS_ON(lcd->power))
		ret = ld9040_power_off(lcd);

	if (!ret)
		lcd->power = power;

	return ret;
}

static int ld9040_set_power(struct lcd_device *ld, int power)
{
	struct ld9040 *lcd = lcd_get_data(ld);

	if (power != FB_BLANK_UNBLANK && power != FB_BLANK_POWERDOWN &&
		power != FB_BLANK_NORMAL) {
		dev_err(lcd->dev, "power value should be 0, 1 or 4.\n");
		return -EINVAL;
	}

	return ld9040_power(lcd, power);
}

static int ld9040_get_power(struct lcd_device *ld)
{
	struct ld9040 *lcd = lcd_get_data(ld);

	return lcd->power;
}

static int ld9040_get_brightness(struct backlight_device *bd)
{
	return bd->props.brightness;
}

static int ld9040_set_brightness(struct backlight_device *bd)
{
	int ret = 0, bl = bd->props.brightness;
	struct ld9040 *lcd = bl_get_data(bd);

#if 1 /* kyNam_110127 */
	static int BRIGHTNESS_DIM_EUR = 20;
	static int BRIGHTNESS_MIN_EUR = 30;
	static int BRIGHTNESS_MAX_EUR = 255;

	static int BRIGHTNESS_DIM_KOR = 5;
	static int BRIGHTNESS_MIN_KOR = 15;
	static int BRIGHTNESS_MAX_KOR = 255;
	int brightnessKor = 0;

	if (bl >= 0 && bl < BRIGHTNESS_DIM_EUR) {
		brightnessKor = bl *BRIGHTNESS_DIM_KOR / BRIGHTNESS_DIM_EUR;
		/*dev_err(&bd->dev, "[M250S] bl(eur)=%d, (kor)=%d 
		 * (<Dimming)\n", bl,  brightnessKor);*/
	}
	else if( bl >= BRIGHTNESS_DIM_EUR && bl < BRIGHTNESS_MIN_EUR) {
		/* ( kor = (bl - EUR_DIM) / (EUR_MIN - EUR_DIM) * 
		 * (KOR_MIN - KOR_DIM) +KOR_DIM */
		brightnessKor = (bl -BRIGHTNESS_DIM_EUR) * 
			(BRIGHTNESS_MIN_KOR -BRIGHTNESS_DIM_KOR);
		brightnessKor /= (BRIGHTNESS_MIN_EUR - BRIGHTNESS_DIM_EUR);
		brightnessKor += BRIGHTNESS_DIM_KOR;
		/*dev_err(&bd->dev, "[M250S] bl(eur)=%d, (kor)=%d 
		 * (>=Dimming <Min)\n", bl, brightnessKor  );*/
	}
	else if( bl >= BRIGHTNESS_MIN_EUR && bl <= BRIGHTNESS_MAX_EUR) {
		/* ( kor = (bl - EUR_MIN) / (EUR_MAX - EUR_MIN) * 
		 * (KOR_MAX -KOR_MIN) +KOR_MIN */
		brightnessKor = (bl -BRIGHTNESS_MIN_EUR) * 
			(BRIGHTNESS_MAX_KOR -BRIGHTNESS_MIN_KOR);
		brightnessKor /= (BRIGHTNESS_MAX_EUR - BRIGHTNESS_MIN_EUR);
		brightnessKor += BRIGHTNESS_MIN_KOR;
		/*dev_err(&bd->dev, "[M250S] bl(eur)=%d, (kor)=%d 
		 * (>=Min <=Max)\n", bl, brightnessKor  );*/
	}
	else  {
		dev_err(&bd->dev, "[M250S] bl(eur)=%d, (kor)=%d"
				"(Unknown Case)\n", bl, brightnessKor);
	}

	bl = brightnessKor;
#endif

	if (bl < MIN_BRIGHTNESS ||
		bl > bd->props.max_brightness) {
		dev_err(&bd->dev, "lcd brightness should be %d to %d. now %d\n",
			MIN_BRIGHTNESS, MAX_BRIGHTNESS, bl);
		return -EINVAL;
	}

	/*dev_err( &bd->dev, "lcd brightness : %d #################\n", bl);*/

	lcd->bl = get_gamma_value_from_bl(bl);


	if (lcd->ldi_enable) {
		ret = update_brightness(lcd);
		if (ret < 0)
			dev_err(&bd->dev, "update brightness failed.\n");
	}

	return ret;
}

static struct lcd_ops ld9040_lcd_ops = {
	.set_power = ld9040_set_power,
	.get_power = ld9040_get_power,
};

static const struct backlight_ops ld9040_backlight_ops  = {
	.get_brightness = ld9040_get_brightness,
	.update_status = ld9040_set_brightness,
};

static ssize_t ld9040_sysfs_backlihgt_level_test(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t len)
{
	struct ld9040 *lcd = dev_get_drvdata(dev);
	unsigned long brightness;
	int rc;

	rc = strict_strtoul(buf, 0, &brightness);
	if (rc < 0)
		return rc;
	else
		lcd->bd->props.brightness = brightness;

	ld9040_set_brightness(lcd->bd);
	return 0;
}

static DEVICE_ATTR(baktst, 0664,
		NULL, ld9040_sysfs_backlihgt_level_test);
static ssize_t acl_set_show(struct device *dev, struct
device_attribute *attr, char *buf)
{
	struct ld9040 *lcd = dev_get_drvdata(dev);
	char temp[3];

	sprintf(temp, "%d\n", lcd->acl_enable);
	strcpy(buf, temp);

	return strlen(buf);
}
static ssize_t acl_set_store(struct device *dev, struct
device_attribute *attr, const char *buf, size_t size)
{
	struct ld9040 *lcd = dev_get_drvdata(dev);
	int value;
	int rc;

	rc = strict_strtoul(buf, (unsigned int) 0, (unsigned long *)&value);
	if (rc < 0)
		return rc;
	else{
		if (lcd->acl_enable != value) {
			lcd->acl_enable = value;
			ld9040_set_acl(lcd);
		}
		return 0;
	}
}

static DEVICE_ATTR(acl_set, 0664,
		acl_set_show, acl_set_store);

static ssize_t lcdtype_show(struct device *dev, struct
device_attribute *attr, char *buf)
{

	char temp[15];
	sprintf(temp, "SMD_AMS427G03\n");
	strcat(buf, temp);
	return strlen(buf);
}

static DEVICE_ATTR(lcdtype, 0664,
		lcdtype_show, NULL);
static ssize_t ld9040_sysfs_show_gamma_mode(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct ld9040 *lcd = dev_get_drvdata(dev);
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
		dev_info(dev, "gamma mode could be 0:2.2, 1:1.9 or 2:1.7)n");
		break;
	}

	return strlen(buf);
}

static ssize_t ld9040_sysfs_store_gamma_mode(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t len)
{
	struct ld9040 *lcd = dev_get_drvdata(dev);
	int rc;

	dev_info(dev, "ld9040_sysfs_store_gamma_mode\n");

	rc = strict_strtoul(buf, 0, (unsigned long *)&lcd->gamma_mode);
	if (rc < 0)
		return rc;

	if (lcd->ldi_enable)
		ld9040_gamma_ctl(lcd);

	return len;
}

static DEVICE_ATTR(gamma_mode, 0664,
		ld9040_sysfs_show_gamma_mode, ld9040_sysfs_store_gamma_mode);

static ssize_t ld9040_sysfs_show_gamma_table(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct ld9040 *lcd = dev_get_drvdata(dev);
	char temp[3];

	sprintf(temp, "%d\n", lcd->gamma_table_count);
	strcpy(buf, temp);

	return strlen(buf);
}

static DEVICE_ATTR(gamma_table, 0664,
		ld9040_sysfs_show_gamma_table, NULL);

#if defined(CONFIG_PM)
#ifdef CONFIG_HAS_EARLYSUSPEND

void ld9040_power_down(struct ld9040 *lcd)
{
	lcd->beforepower = lcd->power;
	ld9040_power(lcd, FB_BLANK_POWERDOWN);
}

void ld9040_power_up(struct ld9040 *lcd)
{
	if (lcd->beforepower == FB_BLANK_UNBLANK)
		lcd->power = FB_BLANK_POWERDOWN;

	ld9040_power(lcd, lcd->beforepower);
}

#else
#define ld9040_suspend		NULL
#define ld9040_resume		NULL
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
void ld9040_early_suspend(struct early_suspend *h)
{
	struct ld9040 *lcd = container_of(h, struct ld9040,
								early_suspend);
	printk("+ld9040_early_suspend()\n");
	ld9040_power_down(lcd);
	printk("-ld9040_early_suspend()\n");
	return ;
}

void ld9040_late_resume(struct early_suspend *h)
{
	struct ld9040 *lcd = container_of(h, struct ld9040,
								early_suspend);
	printk("+ld9040_late_resume()\n");
	ld9040_power_up(lcd);
	printk("-ld9040_late_resume()\n");

	return ;
}
#endif
#endif

static int ld9040_probe(struct spi_device *spi)
{
	int ret = 0;
	struct ld9040 *lcd;
	struct ld9040_panel_data *pdata;

	lcd = kzalloc(sizeof(struct ld9040), GFP_KERNEL);
	if (!lcd) {
		pr_err("failed to allocate for lcd\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

	mutex_init(&lcd->lock);

	/* ld9040 lcd panel uses 3-wire 9bits SPI Mode. */
	spi->bits_per_word = 9;

	ret = spi_setup(spi);
	if (ret < 0) {
		dev_err(&spi->dev, "spi setup failed.\n");
		goto out_free_lcd;
	}

	lcd->spi = spi;
	lcd->dev = &spi->dev;

	lcd->lcd_pd = (struct lcd_platform_data *)spi->dev.platform_data;
	if (!lcd->lcd_pd) {
		dev_err(&spi->dev, "platform data is NULL.\n");
		goto out_free_lcd;
	}

	lcd->ld = lcd_device_register("ld9040", &spi->dev,
		lcd, &ld9040_lcd_ops);
	if (IS_ERR(lcd->ld)) {
		ret = PTR_ERR(lcd->ld);
		goto out_free_lcd;
	}

	lcd->bd = backlight_device_register("pwm-backlight", &spi->dev,
		lcd, &ld9040_backlight_ops, NULL);
	if (IS_ERR(lcd->bd)) {
		ret = PTR_ERR(lcd->bd);
		goto out_free_lcd;
	}

	lcd->bd->props.max_brightness = MAX_BRIGHTNESS;
	lcd->bd->props.brightness = MAX_BRIGHTNESS;
	lcd->bl = MAX_GAMMA_LEVEL - 1;

	lcd->acl_enable = 1;
	lcd->cur_acl = 0;


	/*
	 * it gets gamma table count available so it gets user
	 * know that.
	 */
	pdata = lcd->lcd_pd->pdata;

	lcd->gamma_table_count =
	   pdata->gamma_table_size / (MAX_GAMMA_LEVEL * sizeof(int));

	ret = device_create_file(&(spi->dev), &dev_attr_gamma_mode);
	if (ret < 0)
		dev_err(&(spi->dev), "failed to add sysfs entries\n");

	ret = device_create_file(&(spi->dev), &dev_attr_gamma_table);
	if (ret < 0)
		dev_err(&(spi->dev), "failed to add sysfs entries\n");

	ret = device_create_file(&(spi->dev), &dev_attr_baktst);
	if (ret < 0)
		dev_err(&(spi->dev), "failed to add sysfs entries\n");
	ret = device_create_file(&(spi->dev), &dev_attr_acl_set);
	if (ret < 0)
		dev_err(&(spi->dev), "failed to add sysfs entries\n");

	ret = device_create_file(&(spi->dev), &dev_attr_lcdtype);
	if (ret < 0)
		dev_err(&(spi->dev), "failed to add sysfs entries\n");

#if 0	/* FixMe : Temporary Debugging for Booting Issue */
/* Do not turn off lcd during booting */
#ifndef LCD_ON_FROM_BOOTLOADER
	lcd->lcd_pd->lcd_enabled = 0;
#else
	lcd->lcd_pd->lcd_enabled = 1;
#endif
#else
	lcd->lcd_pd->lcd_enabled = 0;
#endif
	/*
	 * if lcd panel was on from bootloader like u-boot then
	 * do not lcd on.
	 */
	if (!lcd->lcd_pd->lcd_enabled) {
		/*
		 * if lcd panel was off from bootloader then
		 * current lcd status is powerdown and then
		 * it enables lcd panel.
		 */
		lcd->power = FB_BLANK_POWERDOWN;

		ld9040_power(lcd, FB_BLANK_UNBLANK);
	} else
		lcd->power = FB_BLANK_UNBLANK;

	lcd->ldi_enable = 1;

	dev_set_drvdata(&spi->dev, lcd);

#ifdef CONFIG_HAS_EARLYSUSPEND
	lcd->early_suspend.suspend = ld9040_early_suspend;
	lcd->early_suspend.resume = ld9040_late_resume;
	lcd->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1;
	register_early_suspend(&lcd->early_suspend);
#endif

	dev_info(&spi->dev, "ld9040 panel driver has been probed.\n");
	return 0;

out_free_lcd:
	mutex_destroy(&lcd->lock);
	kfree(lcd);
err_alloc:
	return ret;
}

static int __devexit ld9040_remove(struct spi_device *spi)
{
	struct ld9040 *lcd = dev_get_drvdata(&spi->dev);

	ld9040_power(lcd, FB_BLANK_POWERDOWN);
	lcd_device_unregister(lcd->ld);

	return 0;
}

/* Power down all displays on reboot, poweroff or halt. */
static void ld9040_shutdown(struct spi_device *spi)
{
	struct ld9040 *lcd = dev_get_drvdata(&spi->dev);

	ld9040_power(lcd, FB_BLANK_POWERDOWN);
}

static struct spi_driver ld9040_driver = {
	.driver = {
		.name	= "ld9040",
		.bus	= &spi_bus_type,
		.owner	= THIS_MODULE,
	},
	.probe		= ld9040_probe,
	.remove		= __devexit_p(ld9040_remove),
	.shutdown	= ld9040_shutdown,
};

static int __init ld9040_init(void)
{
	return spi_register_driver(&ld9040_driver);
}

static void __exit ld9040_exit(void)
{
	spi_unregister_driver(&ld9040_driver);
}

module_init(ld9040_init);
module_exit(ld9040_exit);

MODULE_AUTHOR("Donghwa Lee <dh09.lee@samsung.com>");
MODULE_DESCRIPTION("ld9040 LCD Driver");
MODULE_LICENSE("GPL");

