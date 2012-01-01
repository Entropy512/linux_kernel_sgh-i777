/* linux/drivers/video/samsung/s3cfb_s6e8aa0.c
 *
 * MIPI-DSI based AMS397G201 AMOLED lcd panel driver.
 *
 * InKi Dae, <inki.dae@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Modified by Samsung Electronics (UK) on May 2010
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
#include "AMS403GF01_gamma_1_9.h"
#include "AMS403GF01_gamma_2_2.h"

/*********** for debug **********************************************************/
#if 1
#define gprintk(fmt, x... ) printk( "%s(%d): " fmt, __FUNCTION__ ,__LINE__, ## x)
#else
#define gprintk(x...) do { } while (0)
#endif
/*******************************************************************************/


#define GAMMASET_CONTROL //for 1.9/2.2 gamma control from platform
#define ACL_ENABLE

/* add lcd partial constants */
#define DCS_PARTIAL_MODE_ON	0X12
#define DCS_NORMAL_MODE_ON	0X13
#define DCS_PARTIAL_AREA	0X30

#define DIM_BL			20
#define MIN_BL			30
#define MAX_BL			255
#define MAX_GAMMA_VALUE		24
#define MIN_BRIGHTNESS		0
#define MAX_BRIGHTNESS		255
#define MAX_GAMMA_LEVEL		25

#define AMS403GF01_GAMMA_PARAM_SET_DATA_COUNT       26

/* Indicates the state of the device */
int bd_brightness = 0;

static struct s5p_lcd lcd;
struct s5p_lcd{
	unsigned int			bl;
	unsigned int 			acl_enable;
	unsigned int 			cur_acl;
	unsigned int			ldi_enable;
	unsigned int			panel_initialized;
	unsigned int			partial_mode;
	unsigned int			gamma_mode;
	
	struct device			*dev;
	struct lcd_device		*ld;
	struct backlight_device		*bd;
	struct early_suspend    early_suspend;
};

static struct mipi_ddi_platform_data *ddi_pd;
extern unsigned char s5p_dsim_set_hs_enable(unsigned int dsim_base);
extern unsigned char s5p_dsim_set_stopstate(unsigned int dsim_base);

extern irqreturn_t s5p_dsim_intr(struct platform_device *pdev);
extern u8 s3cfb_frame_done(void);
extern void s3cfb_trigger(void);

#ifdef GAMMASET_CONTROL
struct class *gammaset_class;
struct device *switch_gammaset_dev;
#endif

char image_update = 1;
EXPORT_SYMBOL(image_update);

char EGL_ready = 0;
EXPORT_SYMBOL(EGL_ready);

#ifdef ACL_ENABLE
#include "AMS403GF01_acl.h"

#define ARRAY_SIZE(arr)	(s32)(sizeof(arr) / sizeof((arr)[0]))

#define ACL_CUTOFF_PARAM_SET_DATA_COUNT     29
#define ACL_OFF     0x0
#define ACL_ON      0x1
#define AWON        0x2
#define AWINOUT     0x4

static struct class *acl_class;
static struct device *switch_aclset_dev;
#endif

static void s6e8aa0_write_0(unsigned char dcs_cmd)
{
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_NO_PARA, dcs_cmd, 0);
}

static void s6e8aa0_write_1(unsigned char dcs_cmd, unsigned char param1)
{
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_1_PARA, dcs_cmd, param1);
}

static void s6e8aa0_write_2(unsigned char dcs_cmd, unsigned char param1,
		unsigned char param2)
{
       unsigned char buf[3];
	buf[0] = dcs_cmd;
	buf[1] = param1;
	buf[2] = param2;	

	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, 3);
}

/* add lcd partial and dim mode functionalies */
static void s6e8aa0_write_4(unsigned char dcs_cmd, unsigned char param1, unsigned char param2, unsigned char param3, unsigned char param4)
{
       unsigned char buf[5];
	buf[0] = dcs_cmd;
	buf[1] = param1;
	buf[2] = param2;	
	buf[3] = param3;
	buf[4] = param4;	

	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, 5);
      
}

static void s6e8aa0_write(const unsigned char * param_set, int size)
{
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,
			(unsigned int) param_set, size);
      
}

static void s6e8aa0_display_off(struct device *dev)
{
	s6e8aa0_write_0(0x28);
}

static void s6e8aa0_sleep_in(struct device *dev)
{
	s6e8aa0_write_0(0x10);
}

static void s6e8aa0_sleep_out(struct device *dev)
{
	s6e8aa0_write_0(0x11);
}

static void s6e8aa0_display_on(struct device *dev)
{
	s6e8aa0_write_0(0x29);
}

static void s6e8aa0_set_tear_off(void)
{
	s6e8aa0_write_0(0x34);
}

static void s6e8aa0_set_tear_on(void)
{
	s6e8aa0_write_0(0x35);
}

static void ETC_CONDITION_SET_1(void)
{
	unsigned char buf[3];

	buf[0] = 0xF0;
	buf[1] = 0x5A;
	buf[2] = 0x5A;

	s6e8aa0_write(buf, 3);

}

void GAMMA_CONDITION_SET(void)
{
	unsigned char buf[26];

	/* GAMMA SET */
	buf[0]  = 0xFA;   
	buf[1]  = 0x01;
	buf[2]  = 0x2F;
	buf[3]  = 0x2F;
	buf[4]  = 0x2F;
	buf[5]  = 0xE1;
	buf[6]  = 0xD2;
	buf[7]  = 0xD4;
	buf[8]  = 0xC6;
	buf[9]  = 0xC4;
	buf[10] = 0xC0;
	buf[11] = 0xCF;
	buf[12] = 0xD0;
	buf[13] = 0xCC;
	buf[14] = 0xA5;
	buf[15] = 0xA6;
	buf[16] = 0xA0;
	buf[17] = 0xB8;
	buf[18] = 0xB9;
	buf[19] = 0xB3;
	buf[20] = 0x00;
	buf[21] = 0xA5;
	buf[22] = 0x00;
	buf[23] = 0xA5;
	buf[24] = 0x00;
	buf[25] = 0xD1;

	s6e8aa0_write(buf, 26);

	/* GAMMA UPDATE */
	s6e8aa0_write_1(0xF7, 0x01);

}

static void PANEL_CONDITION_SET(void)
{
	s6e8aa0_write_1(0xf8, 0x25); 
}

static void DISPLAY_CONDITION_SET(void)
{
	unsigned char buf[4];

	buf[0]  = 0xF2;   
	buf[1]  = 0x80;
	buf[2]  = 0x03;
	buf[3]  = 0x0D;
	
	s6e8aa0_write(buf, 4); 
}

static void ETC_CONDITION_SET_2(void)
{
	unsigned char buf[20];

	buf[0]  = 0xF6;
	buf[1]  = 0x00;
	buf[2]  = 0x02;
	buf[3]  = 0x00;
	
	s6e8aa0_write(buf, 4);  

	buf[0]  = 0xB6;
	buf[1]  = 0x0C;
	buf[2]  = 0x02;
	buf[3]  = 0x03;
	buf[4]  = 0x32;
	buf[5]  = 0xFF;
	buf[6]  = 0x44;
	buf[7]  = 0x44;
	buf[8]  = 0xC0;
	buf[9]  = 0x00;

	s6e8aa0_write(buf, 10);  

	buf[0]  = 0xD9;
	buf[1]  = 0x14;
	buf[2]  = 0x40;
	buf[3]  = 0x0C;
	buf[4]  = 0xCB;
	buf[5]  = 0xCE;
	buf[6]  = 0x6E;
	buf[7]  = 0xC4;
	buf[8]  = 0x0F;
	buf[9]  = 0x40;
	buf[10]  = 0x40;
	buf[11]  = 0xCE;
	buf[12]  = 0x00;
	buf[13]  = 0x60;
	buf[14]  = 0x19;

	s6e8aa0_write(buf, 15); 

	buf[0]  = 0xE1;
	buf[1]  = 0x10;
	buf[2]  = 0x1C;
	buf[3]  = 0x17;
	buf[4]  = 0x08;
	buf[5]  = 0x1D;

	s6e8aa0_write(buf, 6); 

	buf[0]  = 0xE2;
	buf[1]  = 0xED;
	buf[2]  = 0x07;
	buf[3]  = 0xC3;
	buf[4]  = 0x13;
	buf[5]  = 0x0D;
	buf[6]  = 0x03;

	s6e8aa0_write(buf, 7); 

	s6e8aa0_write_1(0xE3, 0x40);

	buf[0]  = 0xE4;
	buf[1]  = 0x00;
	buf[2]  = 0x00;
	buf[3]  = 0x14;
	buf[4]  = 0x80;
	buf[5]  = 0x00;
	buf[6]  = 0x00;
	buf[7]  = 0x00;
	
	s6e8aa0_write(buf, 8); 
 
}

void lcd_pannel_on(void)
{        
	s6e8aa0_write_1(0xf8, 0x25);
	/* Set Display ON */
	s6e8aa0_write_0(0x29);
}

void lcd_panel_init(void)
{
	printk("%s:: \n", __func__);

	ETC_CONDITION_SET_1();

	s6e8aa0_write_0(0x11);

	PANEL_CONDITION_SET();
	DISPLAY_CONDITION_SET();

	GAMMA_CONDITION_SET();

	ETC_CONDITION_SET_2();

	msleep(120);
	lcd_pannel_on(); 
}

static int s6e8aa0_gamma_ctl(void)
{
	int ret = 0;

	if(!lcd.ldi_enable) {
		ret = -1;
		goto gamma_err;
	}
	
	if(lcd.gamma_mode)
		s6e8aa0_write(ams403gf01_19gamma_set_tbl[lcd.bl], AMS403GF01_GAMMA_PARAM_SET_DATA_COUNT);
	else
		s6e8aa0_write(ams403gf01_22gamma_set_tbl[lcd.bl], AMS403GF01_GAMMA_PARAM_SET_DATA_COUNT);

	/* GAMMA Update */
	s6e8aa0_write_1(0xF7, 0x01);

gamma_err:
	return ret;
}

static int update_brightness(void)
{
	int ret;

	ret = s6e8aa0_gamma_ctl();
	if (ret) {
		return -1;
	}

	return 0;
}


/* add lcd partial mode functions */
static bool s6e8aa0_partial_mode_status(struct device *dev)
{
	return false;
}

static int s6e8aa0_partial_mode_on(struct device *dev, u8 display_area)
{
	int r=0;
	
	lcd.partial_mode = display_area;
	if(!lcd.panel_initialized) {
		/* panel is not yet initialized, so return.
		 * stored partial_mode will be used
		 * when initializing the panel
		 */
		gprintk("panel not yet initialized, returning");
		return r;
	}
	gprintk("partial mode req being sent to screen: %d\r\n", display_area);

	switch (display_area){
	case 1: /* main screen: from row 1 to 799 (0x31F) */
		s6e8aa0_write_4(DCS_PARTIAL_AREA, 0, 2, 0x3, 0x20);
		break;

	case 2: /* softkeys screen: from row 799 (0x31F) to 895 (0x37F) */
		s6e8aa0_write_4(DCS_PARTIAL_AREA, 0x3, 0x20, 0x3, 0x80);
		break;

	case 3: /* main + softkeys screen */
		s6e8aa0_write_4(DCS_PARTIAL_AREA, 0, 2, 0x3, 0x80);
		break;

	case 4: /* ticker screen */
		s6e8aa0_write_4(DCS_PARTIAL_AREA, 0x3, 0x80, 0x3, 0xE0);
		break;

	case 6: /* softkeys + ticker screen */
		s6e8aa0_write_4(DCS_PARTIAL_AREA, 0x3, 0x20, 0x3, 0xE0);
		break;

	case 7: /* full screen */
		s6e8aa0_write_4(DCS_PARTIAL_AREA, 0, 2, 0x3, 0xE0);
		break;

	default:
		return -EINVAL;

	}
	
	/* Uncomment the msleep() below if there is any issue with the panel
	 * entering partial mode usually once the panel has entered into
	 * partial mode, sometimes it will not enter into partial mode again
	 * without this msleep. Fortunately, in our use cases the panel might
	 * enter into partial mode only once each time after it is switched on.
	 * So, no need to use this msleep() for Garnett.
	 */
	
	s6e8aa0_write_0(DCS_PARTIAL_MODE_ON);	

	return r;

}

static int s6e8aa0_partial_mode_off(struct device *dev)
{
	lcd.partial_mode = 0;

	if (!lcd.panel_initialized) {
		gprintk("panel not yet initialized, returning\r\n");
		return 0;
	}
	s6e8aa0_write_0(DCS_NORMAL_MODE_ON);
	
	return 0;
}

static int s6e8aa0_panel_init(struct device *dev)
{
	msleep(5);
	
	lcd.panel_initialized = 1;
	lcd.ldi_enable= 1;
	
	lcd_panel_init();

	update_brightness();

	if(lcd.partial_mode)
		s6e8aa0_partial_mode_on(dev, lcd.partial_mode);

	/* this function should be called after partial mode call(if any)*/
	return 0;
}

static int s6e8aa0_set_link(void *pd, unsigned int dsim_base,
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

#ifdef ACL_ENABLE 
static void s6e8aa0_acl_onoff(unsigned int isOn)
{
	if (isOn)
		/* ACL FRAME_SIZE = 480 * 992 */
		s6e8aa0_write_2(0xC0, AWINOUT|AWON|ACL_ON, 0x01);
	else
		s6e8aa0_write_2(0xC0, AWINOUT|AWON|ACL_OFF, 0x01);        
}

static void panel_acl_send_sequence(const unsigned char * acl_param_tbl, unsigned int count)
{
	/* Level 2 key command */
	s6e8aa0_write_2(0xF0, 0x5A, 0x5A);

	s6e8aa0_write(acl_param_tbl, count);

	s6e8aa0_acl_onoff(1);                
}

static int s6e8aa0_set_acl(void)
{
	int ret = 0;

	if(!lcd.ldi_enable) {
		ret = -1;
		goto acl_err;
	}

	if (lcd.acl_enable) {
		if(lcd.cur_acl == 0)  {
			s6e8aa0_acl_onoff(1);
		}
		switch (lcd.bl) {
		case 0 ... 1: /* 30cd ~ 40cd */			
			if (lcd.cur_acl != 0) {
				panel_acl_send_sequence(acl_cutoff_param_set_tbl[0], ACL_CUTOFF_PARAM_SET_DATA_COUNT);
				dev_dbg(lcd.dev, "ACL_cutoff_set Percentage : off!!\n");
				lcd.cur_acl = 0;
			}
			break;
		case 2 ... 12: /* 70cd ~ 180cd */
			if (lcd.cur_acl != 40) {
				panel_acl_send_sequence(acl_cutoff_param_set_tbl[1], ACL_CUTOFF_PARAM_SET_DATA_COUNT);
				dev_dbg(lcd.dev, "ACL_cutoff_set Percentage : 40!!\n");
				lcd.cur_acl = 40;
			}
			break;
		case 13: /* 190cd */
			if (lcd.cur_acl != 43) {
				panel_acl_send_sequence(acl_cutoff_param_set_tbl[2], ACL_CUTOFF_PARAM_SET_DATA_COUNT);
				dev_dbg(lcd.dev, "ACL_cutoff_set Percentage : 43!!\n");
				lcd.cur_acl = 43;
			}
			break;
		case 14: /* 200cd */			
			if (lcd.cur_acl != 45) {
				panel_acl_send_sequence(acl_cutoff_param_set_tbl[3], ACL_CUTOFF_PARAM_SET_DATA_COUNT);
				dev_dbg(lcd.dev, "ACL_cutoff_set Percentage : 45!!\n");
				lcd.cur_acl = 45;
			}
			break;
		case 15: /* 210cd */
			if (lcd.cur_acl != 47) {
				panel_acl_send_sequence(acl_cutoff_param_set_tbl[4], ACL_CUTOFF_PARAM_SET_DATA_COUNT);
				dev_dbg(lcd.dev, "ACL_cutoff_set Percentage : 47!!\n");
				lcd.cur_acl = 47;
			}
			break;
		case 16: /* 220cd */			
			if (lcd.cur_acl != 48) {
				panel_acl_send_sequence(acl_cutoff_param_set_tbl[5], ACL_CUTOFF_PARAM_SET_DATA_COUNT);
				dev_dbg(lcd.dev, "ACL_cutoff_set Percentage : 48!!\n");
				lcd.cur_acl = 48;
			}
			break;
		default:
			if (lcd.cur_acl != 50) {
				panel_acl_send_sequence(acl_cutoff_param_set_tbl[6], ACL_CUTOFF_PARAM_SET_DATA_COUNT);
				dev_dbg(lcd.dev, "ACL_cutoff_set Percentage : 50!!\n");
				lcd.cur_acl = 50;
			}
			break;
		}
	}
	else{
			panel_acl_send_sequence(acl_cutoff_param_set_tbl[0], ACL_CUTOFF_PARAM_SET_DATA_COUNT);
			lcd.cur_acl = 0;
			dev_dbg(lcd.dev, "ACL_cutoff_set Percentage : off!!\n");	
	}

	if (ret) {
		ret = -1;
		goto acl_err;
	}

acl_err:
	return ret;
}
#endif

static int get_gamma_value_from_bl(int bl)
{
	int gamma_value =0;
	int gamma_val_x10 =0;

	if(bl >= MIN_BL){
		gamma_val_x10 = 10 *(MAX_GAMMA_VALUE-1)*bl/(MAX_BL-MIN_BL) + (10 - 10*(MAX_GAMMA_VALUE-1)*(MIN_BL)/(MAX_BL-MIN_BL));
		gamma_value=(gamma_val_x10 +5)/10;
	}else{
		gamma_value =0;
	}

	return gamma_value;
}

static int s6e8aa0_set_brightness(struct backlight_device* bd)
{
	int bl = bd->props.brightness;
	int ret;

	if (bl < MIN_BRIGHTNESS ||
		bl > bd->props.max_brightness) {
		dev_err(&bd->dev, "lcd brightness should be %d to %d. now %d\n",
			MIN_BRIGHTNESS, MAX_BRIGHTNESS, bl);
		return -EINVAL;
	}

	lcd.bl = get_gamma_value_from_bl(bl);

	ret = update_brightness();
	if (ret < 0) {
	/*
		dev_err(&bd->dev, "skip update brightness. because ld9040 is on suspend state...\n");
	*/
		return -EINVAL;
	}

	return ret;
}

static int s6e8aa0_get_brightness(struct backlight_device* bd)
{
	gprintk("called %s \n",__func__);
	return bd_brightness;
}

static const struct backlight_ops s6e8aa0_backlight_ops  = {
	.get_brightness = s6e8aa0_get_brightness,
	.update_status = s6e8aa0_set_brightness,
};

#ifdef GAMMASET_CONTROL /* for 1.9/2.2 gamma control from platform */
static ssize_t gammaset_file_cmd_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
	gprintk("called %s \n",__func__);

	return sprintf(buf,"%u\n", bd_brightness);
}

static ssize_t gammaset_file_cmd_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t size)
{
	int ret;
	
	gprintk("called %s \n",__func__);
	
	sscanf(buf, "%d", &lcd.gamma_mode);

	ret = update_brightness();
	if (ret < 0)
		return ret;

	return size;
}

static DEVICE_ATTR(gammaset_file_cmd,0664, gammaset_file_cmd_show, gammaset_file_cmd_store);
#endif

#ifdef ACL_ENABLE
static ssize_t aclset_file_cmd_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	char temp[3];

	gprintk("called %s \n",__func__);

	sprintf(temp, "%d\n", lcd.acl_enable);
	strcpy(buf, temp);

	return strlen(buf);
}

static ssize_t aclset_file_cmd_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int value, rc;

	gprintk("called %s \n",__func__);

	rc = strict_strtoul(buf, (unsigned int) 0, (unsigned long *)&value);
	if (rc < 0)
		return rc;
	else{
		if (lcd.acl_enable != value) {
			lcd.acl_enable = value;
			s6e8aa0_set_acl();
		}
		return size;
	}
}

static DEVICE_ATTR(aclset_file_cmd,0664, aclset_file_cmd_show,
		aclset_file_cmd_store);
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
void s6e8aa0_early_suspend(struct early_suspend *h)
{
	printk("+s6e8aa0_early_suspend()\n");
	lcd.ldi_enable= 0;
	msleep(60);
	s6e8aa0_display_off(lcd.dev);
	s6e8aa0_sleep_in(lcd.dev);
	msleep(160);
	printk("-s6e8aa0_early_suspend()\n");
	return ;
}

void s6e8aa0_late_resume(struct early_suspend *h)
{
	printk("+s6e8aa0_late_resume()\n");
	s6e8aa0_panel_init(lcd.dev);
	printk("-s6e8aa0_late_resume()\n");

	return ;
}
#endif

/* sysfs export of baclight control */
static int s3cfb_sysfs_show_lcd_update(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return 0;
}

static int s3cfb_sysfs_store_lcd_update(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	int value;

	sscanf(buf, "%d", &value);

	if(value==1 )
	       image_update = 1;

	return len;
}

static DEVICE_ATTR(lcd_update, 0777, s3cfb_sysfs_show_lcd_update, s3cfb_sysfs_store_lcd_update);

static int s6e8aa0_probe(struct device *dev)
{
	int ret = 0;

	lcd.bd = backlight_device_register("pwm-backlight", dev,
		&lcd, &s6e8aa0_backlight_ops, NULL);
	if (IS_ERR(lcd.bd)) {
		ret = PTR_ERR(lcd.bd);
		goto err_te_irq;
	}
	
#ifdef GAMMASET_CONTROL /* for 1.9/2.2 gamma control from platform */
	gammaset_class = class_create(THIS_MODULE, "gammaset");
	if (IS_ERR(gammaset_class))
		pr_err("Failed to create class(gammaset_class)!\n");

	switch_gammaset_dev = device_create(gammaset_class, NULL, 0, NULL,
			"switch_gammaset");
	if (IS_ERR(switch_gammaset_dev))
		pr_err("Failed to create device(switch_gammaset_dev)!\n");

	if (device_create_file(switch_gammaset_dev,
				&dev_attr_gammaset_file_cmd) < 0)
		pr_err("Failed to create device file(%s)!\n",
				dev_attr_gammaset_file_cmd.attr.name);

	if (device_create_file(switch_gammaset_dev, &dev_attr_lcd_update) < 0)
		pr_err("Failed to create device file(%s)!\n",
				dev_attr_lcd_update);
#endif

#ifdef ACL_ENABLE //ACL On,Off
	acl_class = class_create(THIS_MODULE, "aclset");
	if (IS_ERR(acl_class))
		pr_err("Failed to create class(acl_class)!\n");

	switch_aclset_dev = device_create(acl_class, NULL, 0, NULL,
			"switch_aclset");
	if (IS_ERR(switch_aclset_dev))
		pr_err("Failed to create device(switch_aclset_dev)!\n");

	if (device_create_file(switch_aclset_dev, &dev_attr_aclset_file_cmd) < 0)
		pr_err("Failed to create device file(%s)!\n",
				dev_attr_aclset_file_cmd.attr.name);
#endif   
	lcd.dev = dev;
	lcd.bd->props.max_brightness = MAX_BRIGHTNESS;
	lcd.bd->props.brightness = MAX_BRIGHTNESS;
	lcd.bl = MAX_GAMMA_LEVEL - 1;
	
	lcd.acl_enable = 1;
	lcd.cur_acl = 0;
	
	lcd.panel_initialized=0;
	lcd.partial_mode = 0;
	lcd.gamma_mode = 0;

#ifdef CONFIG_HAS_EARLYSUSPEND
	lcd.early_suspend.suspend = s6e8aa0_early_suspend;
	lcd.early_suspend.resume = s6e8aa0_late_resume;
	lcd.early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1;
	register_early_suspend(&lcd.early_suspend);
#endif

	dev_info(dev, "s6e8aa0 lcd panel driver based on mipi-dsi"
			" has been probed.\n");

err_te_irq:

	return 0;
}

static struct mipi_lcd_driver s6e8aa0_mipi_driver = {
	.name = "s6e8aa0",
	.init = s6e8aa0_panel_init,
	.display_on = s6e8aa0_display_on,
	.set_link = s6e8aa0_set_link,
	.probe = s6e8aa0_probe,
	/* add lcd partial and dim mode functionalies */
	.partial_mode_status = s6e8aa0_partial_mode_status,
	.partial_mode_on = s6e8aa0_partial_mode_on,
	.partial_mode_off = s6e8aa0_partial_mode_off,
	.display_off = s6e8aa0_display_off,
};

static int s6e8aa0_init(void)
{
	s5p_dsim_register_lcd_driver(&s6e8aa0_mipi_driver);

	return 0;
}

static void s6e8aa0_exit(void)
{
	return;
}

module_init(s6e8aa0_init);
module_exit(s6e8aa0_exit);

MODULE_AUTHOR("InKi Dae <inki.dae@samsung.com>");
MODULE_DESCRIPTION("MIPI-DSI based S6D6AA0 Panel Driver");
MODULE_LICENSE("GPL");
