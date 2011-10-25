/* linux/drivers/video/samsung/s3cfb_mdnie.c
 *
 * Register interface file for Samsung MDNIE driver
 *
 * Copyright (c) 2009 Samsung Electronics
 * http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/irq.h>
#include <linux/mm.h>
#include <linux/fb.h>
#include <linux/ctype.h>
#include <linux/miscdevice.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fb.h>

#include <linux/io.h>
#include <mach/map.h>
#include <plat/clock.h>
#include <plat/fb.h>
#include <plat/regs-fb.h>
#include <linux/kobject.h>
#include <linux/sysdev.h>
#include "s3cfb.h"
#include "s3cfb_mdnie.h"
#include "s3cfb_ielcd.h"

static struct resource *s3c_mdnie_mem;
static void __iomem *s3c_mdnie_base;

#define s3c_mdnie_readl(addr)             __raw_readl((s3c_mdnie_base + addr))
#define s3c_mdnie_writel(val, addr)        __raw_writel(val, (s3c_mdnie_base + addr))

static char banner[] __initdata = KERN_INFO "S3C MDNIE Driver, (c) 2010 Samsung Electronics\n";

#define 	MDNIE_TUNING

//#define gprintk(fmt, x...) printk("%s(%d): " fmt, __FUNCTION__, __LINE__, ## x)
#define gprintk(x...) do { } while (0)

#define END_SEQ		0xffff

#define TRUE		1
#define FALSE		0

static u16 pre_0x0100 = 0;
static bool g_mdine_enable = 0;

static char * UI_MODE_FILE = NULL;
static char const*const VIDEO_MODE_FILE = VIDEO_MODE_PATH;
static char const*const VIDEO_WARM_MODE_FILE =VIDEO_WARM_MODE_PATH ;
static char const*const VIDEO_WARM_OUTDOOR_MODE_FILE =VIDEO_WARM_OUTDOOR_MODE_PATH ;
static char const*const VIDEO_COLD_MODE_FILE = VIDEO_COLD_MODE_PATH;
static char const*const VIDEO_COLD_OUTDOOR_MODE_FILE = VIDEO_COLD_OUTDOOR_MODE_PATH;
static char const*const CAMERA_MODE_FILE = CAMERA_MODE_PATH;
static char const*const CAMERA_OUTDOOR_MODE_FILE = CAMERA_OUTDOOR_MODE_PATH;
static char const*const GALLERY_MODE_FILE = GALLERY_MODE_PATH;
static char const*const OUTDOOR_MODE_FILE = OUTDOOR_MODE_PATH;
static char const*const BYPASS_MODE_FILE = BYPASS_MODE_PATH; 
static char const*const STANDARD_MODE_FILE = STANDARD_MODE_PATH; 
static char const*const MOVIE_MODE_FILE = MOVIE_MODE_PATH; 
static char const*const DYNAMIC_MODE_FILE = DYNAMIC_MODE_PATH; 
#ifdef CONFIG_TARGET_LOCALE_KOR
static char const*const DMB_MODE_FILE = DMB_MODE_PATH; 
static char const*const  DMB_OUTDOOR_MODE_FILE = DMB_OUTDOOR_MODE_PATH;
static char const*const DMB_WARM_MODE_FILE = DMB_WARM_MODE_PATH; 
static char const*const  DMB_WARM_OUTDOOR_MODE_FILE = DMB_WARM_OUTDOOR_MODE_PATH;
static char const*const DMB_COLD_MODE_FILE = DMB_COLD_MODE_PATH; 
static char const*const  DMB_COLD_OUTDOOR_MODE_FILE = DMB_COLD_OUTDOOR_MODE_PATH;
#endif	/* CONFIG_TARGET_LOCALE_KOR */  

#ifdef CONFIG_TARGET_LOCALE_NTT
static char const*const ISDBT_MODE_FILE = ISDBT_MODE_PATH; 
static char const*const ISDBT_OUTDOOR_MODE_FILE = ISDBT_OUTDOOR_MODE_PATH;
static char const*const ISDBT_WARM_MODE_FILE = ISDBT_WARM_MODE_PATH; 
static char const*const ISDBT_WARM_OUTDOOR_MODE_FILE = ISDBT_WARM_OUTDOOR_MODE_PATH;
static char const*const ISDBT_COLD_MODE_FILE = ISDBT_COLD_MODE_PATH; 
static char const*const ISDBT_COLD_OUTDOOR_MODE_FILE = ISDBT_COLD_OUTDOOR_MODE_PATH;
#endif


int mDNIe_txtbuf_to_parsing(char const*  pFilepath);

static DEFINE_MUTEX(mdnie_use);

typedef struct {
	u16 addr;
	u16 data;
} mDNIe_data_type;

typedef enum {
	mDNIe_UI_MODE,
	mDNIe_VIDEO_MODE,
	mDNIe_VIDEO_WARM_MODE,
	mDNIe_VIDEO_COLD_MODE,
	mDNIe_CAMERA_MODE,
	mDNIe_NAVI,
	mDNIe_GALLERY,
	mDNIe_BYPASS,  
#ifdef CONFIG_TARGET_LOCALE_KOR
	mDNIe_DMB_MODE = 20,
	mDNIe_DMB_WARM_MODE,
	mDNIe_DMB_COLD_MODE,
#endif	/* CONFIG_TARGET_LOCALE_KOR */
#ifdef CONFIG_TARGET_LOCALE_NTT
	mDNIe_ISDBT_MODE = 30,
	mDNIe_ISDBT_WARM_MODE,
	mDNIe_ISDBT_COLD_MODE,
#endif
} Lcd_mDNIe_UI;

typedef enum {
	mDNIe_DYNAMIC,
	mDNIe_STANDARD,
	mDNIe_MOVIE,
} Lcd_mDNIe_User_Set;



struct class *mdnieset_ui_class;
struct device *switch_mdnieset_ui_dev;
struct class *mdnieset_outdoor_class;
struct device *switch_mdnieset_outdoor_dev;

Lcd_mDNIe_UI current_mDNIe_Mode = mDNIe_UI_MODE; /* mDNIe Set Status Checking Value.*/
Lcd_mDNIe_User_Set current_mDNIe_user_mode = mDNIe_STANDARD; /*mDNIe_user Set Status Checking Value.*/

u8 current_mDNIe_OutDoor_OnOff = FALSE;

int mDNIe_Tuning_Mode = FALSE;

#ifdef CONFIG_FB_S3C_MDNIE_TUNINGMODE_FOR_BACKLIGHT

extern int pre_val;

u16 mDNIe_data_ui[] = {
	0x0084, 0x0040,
	0x0090, 0x0000,
	0x0094, 0x0fff,
	0x0098, 0x005C,
	0x009C, 0x0ff0,
	0x00AC, 0x0080,
	0x00B4, 0x0180,
	0x00C0, 0x0400,
	0x00C4, 0x7200,
	0x00C8, 0x008D,
	0x00D0, 0x00C0,
	0x0100, 0x0000,
	END_SEQ, 0x0000,
};

u16 mDNIe_data_300cd_level1[] = {
	0x0084, 0x0080,
	0x0090, 0x0000,
	0x0094, 0x0fff,
	0x0098, 0x005C,
	0x009C, 0x0ff0,
	0x00AC, 0x0080,
	0x00B4, 0x0180,
	0x00C0, 0x0400,
	0x00C4, 0x7200,
	0x00C8, 0x008D,
	0x00D0, 0x00C0,
	0x0100, 0x4020,
	END_SEQ, 0x0000,
};

u16 mDNIe_data_ui_down[] = {
	0x0084, 0x0080,
	0x0090, 0x0000,
	0x0094, 0x0fff,
	0x0098, 0x005C,
	0x009C, 0x0ff0,
	0x00AC, 0x0080,
	0x00B4, 0x0180,
	0x00C0, 0x0400,
	0x00C4, 0x7200,
	0x00C8, 0x008D,
	0x00D0, 0x00C0,
	0x0100, 0x0000,
	0x0084, 0x0040,
	END_SEQ, 0x0000,
};

u16 *pmDNIe_Gamma_set[] = {
	mDNIe_data_ui,/*0*/
	mDNIe_data_300cd_level1,/*01*/
	/*mDNIe_data_300cd_level2,//02*/
	mDNIe_data_ui_down,/*02*/
};

#endif

#ifdef MDNIE_TUNING
u16 light_step;
u16 saturation_step;
u16 cs_step;

u16 adc_level_formDNIe[6] = {0};
u16 mDNIe_data_level0[100] = {0};
u16 mDNIe_data_level1[100] = {0};
u16 mDNIe_data_level2[100] = {0};
u16 mDNIe_data_level3[100] = {0};
u16 mDNIe_data_level4[100] = {0};
u16 mDNIe_data_level5[100] = {0};

int mDNIe_data_level0_cnt;
int mDNIe_data_level1_cnt;
int mDNIe_data_level2_cnt;
int mDNIe_data_level3_cnt;
int mDNIe_data_level4_cnt;
int mDNIe_data_level5_cnt;

u16 mDNIe_data[100] = {0};

unsigned short *test[1];
/*extern unsigned short *test[1];*/
int mdnie_tuning_load;
EXPORT_SYMBOL(mdnie_tuning_load);

void mDNIe_txtbuf_to_parsing_for_lightsensor(void)
{
	int i = 0;
	int cnt;

	light_step = test[0][0];
	saturation_step = test[0][1];
	cs_step = test[0][2];

	/*1 level */
	adc_level_formDNIe[0] = test[0][3];

	for (i = 0; (test[0][i+4] != END_SEQ); i++) {
		if (test[0][i+4] != END_SEQ)
			mDNIe_data_level0[i] = test[0][i+4];
	}
	mDNIe_data_level0[i] = END_SEQ;
	mDNIe_data_level0_cnt = i;
	cnt = i+5;

	/*2 level */
	adc_level_formDNIe[1] = test[0][cnt];
	cnt++;
	for (i = 0; (test[0][cnt+i] != END_SEQ); i++) {
		if (test[0][cnt+i] != END_SEQ)
			mDNIe_data_level1[i] = test[0][cnt+i];
	}
	mDNIe_data_level1[i] = END_SEQ;
	mDNIe_data_level1_cnt = i;
	cnt += i + 1;

	/*3 level */
	adc_level_formDNIe[2] = test[0][cnt];
	cnt++;
	for (i = 0; (test[0][cnt+i] != END_SEQ); i++) {
		if (test[0][cnt+i] != END_SEQ)
			mDNIe_data_level2[i] = test[0][cnt+i];
	}
	mDNIe_data_level2[i] = END_SEQ;
	mDNIe_data_level2_cnt = i;
	cnt += i + 1;

	/*4 level */
	adc_level_formDNIe[3] = test[0][cnt];
	cnt++;
	for (i = 0; (test[0][cnt+i] != END_SEQ); i++) {
		if (test[0][cnt+i] != END_SEQ)
			mDNIe_data_level3[i] = test[0][cnt+i];
	}
	mDNIe_data_level3[i] = END_SEQ;
	mDNIe_data_level3_cnt = i;
	cnt += i + 1;

	/*5level */
	adc_level_formDNIe[4] = test[0][cnt];
	cnt++;
	for (i = 0; (test[0][cnt+i] != END_SEQ); i++) {
		if (test[0][cnt+i] != END_SEQ)
			mDNIe_data_level4[i] = test[0][cnt+i];
	}
	mDNIe_data_level4[i] = END_SEQ;
	mDNIe_data_level4_cnt = i;
	cnt += i+1;

	/*6 level */
	adc_level_formDNIe[5] = test[0][cnt];
	cnt++;
	for (i = 0; (test[0][cnt+i] != END_SEQ); i++) {
		if (test[0][cnt+i] != END_SEQ)
			mDNIe_data_level5[i] = test[0][cnt+i];
	}
	mDNIe_data_level5[i] = END_SEQ;
	mDNIe_data_level5_cnt = i;
	cnt += i+1;

	mdnie_tuning_load = 1;
}
EXPORT_SYMBOL(mDNIe_txtbuf_to_parsing_for_lightsensor);

#endif

int s3c_mdnie_hw_init(void)
{
	printk(KERN_INFO "MDNIE  INIT ..........\n");

	printk(banner);

	s3c_mdnie_mem = request_mem_region(S3C_MDNIE_PHY_BASE, S3C_MDNIE_MAP_SIZE, "mdnie");
	if (s3c_mdnie_mem == NULL) {
		printk(KERN_ERR "MDNIE: failed to reserved memory region\n");
		return -ENOENT;
	}

	s3c_mdnie_base = ioremap(S3C_MDNIE_PHY_BASE, S3C_MDNIE_MAP_SIZE);
	if (s3c_mdnie_base == NULL) {
		printk(KERN_ERR "MDNIE failed ioremap\n");
		return -ENOENT;
	}

	printk(KERN_INFO "MDNIE  INIT SUCCESS Addr : 0x%p\n", s3c_mdnie_base);

	return 0;
}

int s3c_mdnie_unmask(void)
{
	unsigned int mask;

	s3c_mdnie_writel(0x0, S3C_MDNIE_rR40);

	return 0;
}

#if 0
int s3c_mdnie_select_mode(int algo, int mcm, int lpa)
{
	s3c_mdnie_writel(0x0000, S3C_MDNIE_rR1);
	s3c_mdnie_writel(0x0000, S3C_MDNIE_rR36);
	s3c_mdnie_writel(0x0FFF, S3C_MDNIE_rR37);
	s3c_mdnie_writel(0x005c, S3C_MDNIE_rR38);

	s3c_mdnie_writel(0x0ff0, S3C_MDNIE_rR39);
	s3c_mdnie_writel(0x0064, S3C_MDNIE_rR43);
	s3c_mdnie_writel(0x0364, S3C_MDNIE_rR45);

	return 0;
}
#endif

int s3c_mdnie_set_size(unsigned int hsize, unsigned int vsize)
{
	unsigned int size;

	size = s3c_mdnie_readl(S3C_MDNIE_rR34);
	size &= ~S3C_MDNIE_SIZE_MASK;
	size |= hsize;
	s3c_mdnie_writel(size, S3C_MDNIE_rR34);

	s3c_mdnie_unmask();

	size = s3c_mdnie_readl(S3C_MDNIE_rR35);
	size &= ~S3C_MDNIE_SIZE_MASK;
	size |= vsize;
	s3c_mdnie_writel(size, S3C_MDNIE_rR35);

	s3c_mdnie_unmask();

	return 0;
}

int s3c_mdnie_setup(void)
{
	s3c_mdnie_hw_init();
	s3c_ielcd_hw_init();

	return 0;

}
void mDNIe_Set_Mode(Lcd_mDNIe_UI mode, u8 mDNIe_Outdoor_OnOff)
{
	if(!g_mdine_enable) {
		printk(KERN_ERR"[mDNIE WARNING] mDNIE engine is OFF. So you cannot set mDnie Mode correctly.\n");
		return 0;
	}
	switch(current_mDNIe_user_mode){
		case  mDNIe_DYNAMIC:
			UI_MODE_FILE = UI_DYNAMIC_MODE_PATH;
			break;
		case  mDNIe_MOVIE:
			UI_MODE_FILE = UI_MOVIE_MODE_PATH;
			break;
		case  mDNIe_STANDARD:
			UI_MODE_FILE = UI_STANDARD_MODE_PATH;
			break;
		default:
			UI_MODE_FILE = UI_STANDARD_MODE_PATH;
			printk(KERN_ERR"[mDNIE WARNING] cannot UI_MODE_FILE path change.\n");
			break;
		}			
	if (mDNIe_Outdoor_OnOff) {
		switch (mode) {
		case mDNIe_UI_MODE:
			mDNIe_txtbuf_to_parsing(UI_MODE_FILE);
			break;

		case mDNIe_VIDEO_MODE:
			mDNIe_txtbuf_to_parsing(OUTDOOR_MODE_FILE);
			break;

		case mDNIe_VIDEO_WARM_MODE:
			mDNIe_txtbuf_to_parsing(VIDEO_WARM_OUTDOOR_MODE_FILE);
			break;

		case mDNIe_VIDEO_COLD_MODE:
			mDNIe_txtbuf_to_parsing(VIDEO_COLD_OUTDOOR_MODE_FILE);
			break;

		case mDNIe_CAMERA_MODE:
			mDNIe_txtbuf_to_parsing(CAMERA_OUTDOOR_MODE_FILE);
			break;

		case mDNIe_NAVI:
			mDNIe_txtbuf_to_parsing(OUTDOOR_MODE_FILE);
			break;

		case mDNIe_GALLERY:
			mDNIe_txtbuf_to_parsing(GALLERY_MODE_FILE);
			break;

		case mDNIe_BYPASS:  
			mDNIe_txtbuf_to_parsing(BYPASS_MODE_FILE);
			break;
		
#ifdef CONFIG_TARGET_LOCALE_KOR
		case mDNIe_DMB_MODE:
			mDNIe_txtbuf_to_parsing(DMB_OUTDOOR_MODE_FILE);
			break;

		case mDNIe_DMB_WARM_MODE:
			mDNIe_txtbuf_to_parsing(DMB_WARM_OUTDOOR_MODE_FILE);
			break;

		case mDNIe_DMB_COLD_MODE:
			mDNIe_txtbuf_to_parsing(DMB_COLD_OUTDOOR_MODE_FILE);
			break;
#endif /* CONFIG_TARGET_LOCALE_KOR */ 
#ifdef CONFIG_TARGET_LOCALE_NTT
		case mDNIe_ISDBT_MODE:
			mDNIe_txtbuf_to_parsing(ISDBT_OUTDOOR_MODE_FILE);
			break;

		case mDNIe_ISDBT_WARM_MODE:
			mDNIe_txtbuf_to_parsing(ISDBT_WARM_OUTDOOR_MODE_FILE);
			break;

		case mDNIe_ISDBT_COLD_MODE:
			mDNIe_txtbuf_to_parsing(ISDBT_COLD_OUTDOOR_MODE_FILE);
			break;
#endif

		}

		current_mDNIe_Mode = mode;

		if (current_mDNIe_Mode == mDNIe_UI_MODE)
			current_mDNIe_OutDoor_OnOff = FALSE;
		else
			current_mDNIe_OutDoor_OnOff = TRUE;
	} else {
		switch (mode) {
		case mDNIe_UI_MODE:
			mDNIe_txtbuf_to_parsing(UI_MODE_FILE);
			break;

		case mDNIe_VIDEO_MODE:
			mDNIe_txtbuf_to_parsing(VIDEO_MODE_FILE);
			break;

		case mDNIe_VIDEO_WARM_MODE:
			mDNIe_txtbuf_to_parsing(VIDEO_WARM_MODE_FILE);
			break;

		case mDNIe_VIDEO_COLD_MODE:
			mDNIe_txtbuf_to_parsing(VIDEO_COLD_MODE_FILE);
			break;

		case mDNIe_CAMERA_MODE:
			mDNIe_txtbuf_to_parsing(CAMERA_MODE_FILE);
			break;

		case mDNIe_NAVI:
			mDNIe_txtbuf_to_parsing(UI_MODE_FILE);
			break;
			
		case mDNIe_GALLERY:
			mDNIe_txtbuf_to_parsing(GALLERY_MODE_FILE);
			break;
		case mDNIe_BYPASS: 
			mDNIe_txtbuf_to_parsing(BYPASS_MODE_FILE);
			break;

#ifdef CONFIG_TARGET_LOCALE_KOR
		case mDNIe_DMB_MODE:
			mDNIe_txtbuf_to_parsing(DMB_MODE_FILE);
			break;

		case mDNIe_DMB_WARM_MODE:
			mDNIe_txtbuf_to_parsing(DMB_WARM_MODE_FILE);
			break;

		case mDNIe_DMB_COLD_MODE:
			mDNIe_txtbuf_to_parsing(DMB_COLD_MODE_FILE);
			break;
#endif /* CONFIG_TARGET_LOCALE_KOR */ 
#ifdef CONFIG_TARGET_LOCALE_NTT
		case mDNIe_ISDBT_MODE:
			mDNIe_txtbuf_to_parsing(ISDBT_MODE_FILE);
			break;

		case mDNIe_ISDBT_WARM_MODE:
			mDNIe_txtbuf_to_parsing(ISDBT_WARM_MODE_FILE);
			break;

		case mDNIe_ISDBT_COLD_MODE:
			mDNIe_txtbuf_to_parsing(ISDBT_COLD_MODE_FILE);
			break;
#endif
		}

		current_mDNIe_Mode = mode;
		current_mDNIe_OutDoor_OnOff = FALSE;
	}

	pre_0x0100 = 0;
#ifdef CONFIG_FB_S3C_MDNIE_TUNINGMODE_FOR_BACKLIGHT
	pre_val = -1;
#endif	/* CONFIG_FB_S3C_MDNIE_TUNINGMODE_FOR_BACKLIGHT */
	printk(KERN_ERR "[mDNIe] mDNIe_Set_Mode : Current_mDNIe_mode (%d), current_mDNIe_OutDoor_OnOff(%d)  \n", current_mDNIe_Mode, current_mDNIe_OutDoor_OnOff);  
}

void mDNIe_User_Select_Mode(Lcd_mDNIe_User_Set mode)
{
	if(!g_mdine_enable) {
		printk(KERN_ERR"[mDNIE WARNING] mDNIE engine is OFF. So you cannot set mDnie Mode correctly.\n");
		return 0;
	}
	switch (mode) {
	case mDNIe_DYNAMIC:  
		mDNIe_txtbuf_to_parsing(DYNAMIC_MODE_FILE);
		break;

	case mDNIe_STANDARD:  
		mDNIe_txtbuf_to_parsing(STANDARD_MODE_FILE);
		break;

	case mDNIe_MOVIE:  
		mDNIe_txtbuf_to_parsing(MOVIE_MODE_FILE);
		break;
	}
	current_mDNIe_user_mode = mode;
	printk(KERN_ERR "[mDNIe] mDNIe_user_select_Mode : User_mDNIe_Setting_Mode (%d), Current_mDNIe_mode(%d) \n", current_mDNIe_user_mode,current_mDNIe_Mode);  

}

void mDNIe_init_Mode_Set(Lcd_mDNIe_User_Set mode)
{
	if(!g_mdine_enable) {
		printk(KERN_ERR" [mDNIE WARNING] mDNIE engine is OFF. So you cannot set mDnie Mode correctly.\n");
		return 0;
	}
	mDNIe_User_Select_Mode(current_mDNIe_user_mode);
	mDNIe_Set_Mode(current_mDNIe_Mode, current_mDNIe_OutDoor_OnOff);

}


void mDNIe_Init_Set_Mode(void)
{
	mDNIe_init_Mode_Set(current_mDNIe_user_mode);
}

void mDNIe_Mode_Set(void)
{
	mDNIe_Set_Mode(current_mDNIe_Mode, current_mDNIe_OutDoor_OnOff);
}

static ssize_t mdnieset_ui_file_cmd_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int mdnie_ui = 0;

	printk(KERN_INFO "called %s \n", __func__);

	switch (current_mDNIe_Mode) {
	case mDNIe_UI_MODE:
	default:
		mdnie_ui = 0;
		break;

	case mDNIe_VIDEO_MODE:
		mdnie_ui = 1;
		break;

	case mDNIe_VIDEO_WARM_MODE:
		mdnie_ui = 2;
		break;

	case mDNIe_VIDEO_COLD_MODE:
		mdnie_ui = 3;
		break;

	case mDNIe_CAMERA_MODE:
		mdnie_ui = 4;
		break;

	case mDNIe_NAVI:
		mdnie_ui = 5;
		break;

	case mDNIe_GALLERY:
		mdnie_ui = 6;
		break;

	case mDNIe_BYPASS:  
		mdnie_ui = 7;
		break;
		
#ifdef CONFIG_TARGET_LOCALE_KOR
	case mDNIe_DMB_MODE:
		mdnie_ui = mDNIe_DMB_MODE;
		break;

	case mDNIe_DMB_WARM_MODE:
		mdnie_ui = mDNIe_DMB_WARM_MODE;
		break;

	case mDNIe_DMB_COLD_MODE:
		mdnie_ui = mDNIe_DMB_COLD_MODE;
		break;
#endif /* CONFIG_TARGET_LOCALE_KOR */ 		
#ifdef CONFIG_TARGET_LOCALE_NTT
	case mDNIe_ISDBT_MODE:
		mdnie_ui = mDNIe_ISDBT_MODE;
		break;

	case mDNIe_ISDBT_WARM_MODE:
		mdnie_ui = mDNIe_ISDBT_WARM_MODE;
		break;

	case mDNIe_ISDBT_COLD_MODE:
		mdnie_ui = mDNIe_ISDBT_COLD_MODE;
		break;
#endif
	}
	return sprintf(buf, "%u\n", mdnie_ui);
}

static ssize_t mdnieset_ui_file_cmd_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int value;

	sscanf(buf, "%d", &value);

	/*printk(KERN_INFO "[mdnie set] in mdnieset_ui_file_cmd_store, input value = %d \n",value);*/

	switch (value) {
	case SIG_MDNIE_UI_MODE:
		current_mDNIe_Mode = mDNIe_UI_MODE;
		break;

	case SIG_MDNIE_VIDEO_MODE:
		current_mDNIe_Mode = mDNIe_VIDEO_MODE;
		break;

	case SIG_MDNIE_VIDEO_WARM_MODE:
		current_mDNIe_Mode = mDNIe_VIDEO_WARM_MODE;
		break;

	case SIG_MDNIE_VIDEO_COLD_MODE:
		current_mDNIe_Mode = mDNIe_VIDEO_COLD_MODE;
		break;

	case SIG_MDNIE_CAMERA_MODE:
		current_mDNIe_Mode = mDNIe_CAMERA_MODE;
		break;

	case SIG_MDNIE_NAVI:
		current_mDNIe_Mode = mDNIe_NAVI;
		break;

	case SIG_MDNIE_GALLERY:
		current_mDNIe_Mode = mDNIe_GALLERY;
		break;

	case SIG_MDNIE_BYPASS:  
		current_mDNIe_Mode = mDNIe_BYPASS;
		break;

#ifdef CONFIG_TARGET_LOCALE_KOR
	case SIG_MDNIE_DMB_MODE:
		current_mDNIe_Mode = mDNIe_DMB_MODE;
		break;

	case SIG_MDNIE_DMB_WARM_MODE:
		current_mDNIe_Mode = mDNIe_DMB_WARM_MODE;
		break;

	case SIG_MDNIE_DMB_COLD_MODE:
		current_mDNIe_Mode = mDNIe_DMB_COLD_MODE;
		break;
#endif /* CONFIG_TARGET_LOCALE_KOR */ 
#ifdef CONFIG_TARGET_LOCALE_NTT
	case SIG_MDNIE_ISDBT_MODE:
		current_mDNIe_Mode = mDNIe_ISDBT_MODE;
		break;

	case SIG_MDNIE_ISDBT_WARM_MODE:
		current_mDNIe_Mode = mDNIe_ISDBT_WARM_MODE;
		break;

	case SIG_MDNIE_ISDBT_COLD_MODE:
		current_mDNIe_Mode = mDNIe_ISDBT_COLD_MODE;
		break;
#endif

	default:
		printk(KERN_ERR "\nmdnieset_ui_file_cmd_store value is wrong : value(%d)\n", value);
		break;
	}
	mDNIe_User_Select_Mode(current_mDNIe_user_mode);
	mDNIe_Set_Mode(current_mDNIe_Mode, current_mDNIe_OutDoor_OnOff);

	return size;
}

static DEVICE_ATTR(mdnieset_ui_file_cmd, 0664, mdnieset_ui_file_cmd_show, mdnieset_ui_file_cmd_store);

static ssize_t mdnieset_user_select_file_cmd_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int mdnie_ui = 0;

	printk(KERN_INFO "called %s \n", __func__);

	switch (current_mDNIe_user_mode) {
	case mDNIe_DYNAMIC:  
		mdnie_ui = 0;
		break;
	case mDNIe_STANDARD:  
		mdnie_ui = 1;
		break;
	case mDNIe_MOVIE:  
		mdnie_ui = 2;
		break;
	}
	return sprintf(buf, "%u\n", mdnie_ui);
	
}

static ssize_t mdnieset_user_select_file_cmd_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int value;

	sscanf(buf, "%d", &value);
	/*printk(KERN_INFO "[mdnie set] in mdnieset_ui_file_cmd_store, input value = %d \n",value);*/
	switch (value) {
	case SIG_MDNIE_DYNAMIC:  
		current_mDNIe_user_mode = mDNIe_DYNAMIC;
		break;

	case SIG_MDNIE_STANDARD:  
		current_mDNIe_user_mode = mDNIe_STANDARD;
		break;

	case SIG_MDNIE_MOVIE:  
		current_mDNIe_user_mode = mDNIe_MOVIE;
		break;


	default:
		printk(KERN_ERR "\mdnieset_user_select_file_cmd_store value is wrong : value(%d)\n", value);
		break;
	}
	mDNIe_User_Select_Mode(current_mDNIe_user_mode);
	mDNIe_Set_Mode(current_mDNIe_Mode, current_mDNIe_OutDoor_OnOff);

	return size;
}

static DEVICE_ATTR(mdnieset_user_select_file_cmd, 0664, mdnieset_user_select_file_cmd_show, mdnieset_user_select_file_cmd_store);


static ssize_t mdnieset_init_file_cmd_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char temp[15];
	
	printk(KERN_INFO "called %s \n", __func__);
	sprintf(temp, "mdnieset_init_file_cmd_show \n");
	strcat(buf, temp);
	
	return strlen(buf);
}

static ssize_t mdnieset_init_file_cmd_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int value;

	sscanf(buf, "%d", &value);
	switch (value) {
	case 0:  
		current_mDNIe_user_mode = mDNIe_STANDARD;
		current_mDNIe_Mode =mDNIe_UI_MODE;
		break;
		
	default:
		printk(KERN_ERR "\mdnieset_init_file_cmd_store value is wrong : value(%d)\n", value);
		break;
	}
	mDNIe_User_Select_Mode(current_mDNIe_user_mode);
	mDNIe_Set_Mode(current_mDNIe_Mode, current_mDNIe_OutDoor_OnOff);

	return size;
}

static DEVICE_ATTR(mdnieset_init_file_cmd, 0664, mdnieset_init_file_cmd_show, mdnieset_init_file_cmd_store);


static ssize_t mdnieset_outdoor_file_cmd_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	printk(KERN_INFO "called %s \n", __func__);

	return sprintf(buf, "%u\n", current_mDNIe_OutDoor_OnOff);
}

static ssize_t mdnieset_outdoor_file_cmd_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int value;

	sscanf(buf, "%d", &value);

	/*
		printk(KERN_INFO "[mdnie set] in mdnieset_outdoor_file_cmd_store, input value = %d \n",value);
	 */

	if (value)
		current_mDNIe_OutDoor_OnOff = TRUE;
	else
		current_mDNIe_OutDoor_OnOff = FALSE;

	mDNIe_Set_Mode(current_mDNIe_Mode, current_mDNIe_OutDoor_OnOff);

	return size;
}

static DEVICE_ATTR(mdnieset_outdoor_file_cmd, 0664, mdnieset_outdoor_file_cmd_show, mdnieset_outdoor_file_cmd_store);

void init_mdnie_class(void)
{
	mdnieset_ui_class = class_create(THIS_MODULE, "mdnieset_ui");
	if (IS_ERR(mdnieset_ui_class))
		pr_err("Failed to create class(mdnieset_ui_class)!\n");

	switch_mdnieset_ui_dev = device_create(mdnieset_ui_class, NULL, 0, NULL, "switch_mdnieset_ui");
	if (IS_ERR(switch_mdnieset_ui_dev))
		pr_err("Failed to create device(switch_mdnieset_ui_dev)!\n");

	if (device_create_file(switch_mdnieset_ui_dev, &dev_attr_mdnieset_ui_file_cmd) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_mdnieset_ui_file_cmd.attr.name);

	if (device_create_file(switch_mdnieset_ui_dev, &dev_attr_mdnieset_user_select_file_cmd) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_mdnieset_user_select_file_cmd.attr.name);

	if (device_create_file(switch_mdnieset_ui_dev, &dev_attr_mdnieset_init_file_cmd) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_mdnieset_init_file_cmd.attr.name);

	mdnieset_outdoor_class = class_create(THIS_MODULE, "mdnieset_outdoor");
	if (IS_ERR(mdnieset_outdoor_class))
		pr_err("Failed to create class(mdnieset_outdoor_class)!\n");

	switch_mdnieset_outdoor_dev = device_create(mdnieset_outdoor_class, NULL, 0, NULL, "switch_mdnieset_outdoor");
	if (IS_ERR(switch_mdnieset_outdoor_dev))
		pr_err("Failed to create device(switch_mdnieset_outdoor_dev)!\n");

	if (device_create_file(switch_mdnieset_outdoor_dev, &dev_attr_mdnieset_outdoor_file_cmd) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_mdnieset_outdoor_file_cmd.attr.name);
}
EXPORT_SYMBOL(init_mdnie_class);

#ifdef MDNIE_TUNING
static u16 pre_0x00AC;

static int pre_adc_level;
static int cur_adc_level;


static int mdnie_level;
static int init_mdnie;

void mDNIe_Mode_set_for_lightsensor(u16 *buf)
{
	u32 i = 0;
	int cnt = 0;

	if (cur_adc_level >= pre_adc_level) {	/*0 => END_SEQ*/
		while ((*(buf+i)) != END_SEQ) {
			if ((*(buf+i)) == 0x0100) {
				if (init_mdnie == 0)
					pre_0x0100 = (*(buf+(i+1)));
				if (pre_0x0100 < (*(buf+(i+1)))) {
					while ((pre_0x0100 < (*(buf+(i+1)))) && (pre_0x0100 <= 0x8080) && (pre_0x0100 >= 0x0000)) {
						s3c_mdnie_writel(pre_0x0100, (*(buf+i)));
						printk("[mDNIe] mDNIe_tuning_initialize: addr(0x%x), data(0x%x)  \n", (*(buf+i)), pre_0x0100);
						pre_0x0100 = ((pre_0x0100 & 0xff00) + (light_step<<8)) | ((pre_0x0100 & 0x00ff) + (saturation_step));
					}
				} else if (pre_0x0100 > (*(buf+(i+1)))) {
					while (pre_0x0100 > (*(buf+(i+1))) && (pre_0x0100 >= 0x0000) && (pre_0x0100 <= 0x8080)) {
						s3c_mdnie_writel(pre_0x0100, (*(buf+i)));
						printk("[mDNIe] mDNIe_tuning_initialize: addr(0x%x), data(0x%x)  \n", (*(buf+i)), pre_0x0100);
						pre_0x0100 = ((pre_0x0100 & 0xff00) - (light_step<<8)) | ((pre_0x0100 & 0x00ff) - (saturation_step));
					}
				}
				s3c_mdnie_writel((*(buf+i+1)), (*(buf+i)));
				pre_0x0100 = (*(buf+i+1));
			} else if ((*(buf+i)) == 0x00AC) {
				if (init_mdnie == 0)
					pre_0x00AC = (*(buf+(i+1)));
				if (pre_0x00AC < (*(buf+(i+1)))) {
					while (pre_0x00AC < (*(buf+(i+1))) && (pre_0x00AC <= 0x03ff) && (pre_0x00AC >= 0x0000)) {
						s3c_mdnie_writel(pre_0x00AC, (*(buf+i)));
						printk("[mDNIe] mDNIe_tuning_initialize: addr(0x%x), data(0x%x)  \n", (*(buf+i)), pre_0x00AC);
						pre_0x00AC += (cs_step);
					}
				} else if (pre_0x00AC > (*(buf+(i+1)))) {
					while (pre_0x00AC > (*(buf+(i+1))) && (pre_0x00AC >= 0x0000) && (pre_0x00AC <= 0x03ff)) {
						s3c_mdnie_writel(pre_0x00AC, (*(buf+i)));
						printk("[mDNIe] mDNIe_tuning_initialize: addr(0x%x), data(0x%x)  \n", (*(buf+i)), pre_0x00AC);
						pre_0x00AC -= (cs_step);
					}
				}
				s3c_mdnie_writel((*(buf+i+1)), (*(buf+i)));
				pre_0x00AC = (*(buf+i+1));
			} else
				s3c_mdnie_writel((*(buf+i+1)), (*(buf+i)));
			printk(KERN_INFO "[mDNIe] mDNIe_tuning_initialize: addr(0x%x), data(0x%x)  \n", (*(buf+i)), (*(buf+(i+1))));
			i += 2;
		}
	} else { /* if (cur_adc_level < pre_adc_level)  //END_SEQ => 0 */
		switch (cur_adc_level) {
		case 0:
			cnt = mDNIe_data_level0_cnt;
			break;
		case 1:
			cnt = mDNIe_data_level1_cnt;
			break;
		case 2:
			cnt = mDNIe_data_level2_cnt;
			break;
		case 3:
		default:
			cnt = mDNIe_data_level3_cnt;
			break;
		case 4:
			cnt = mDNIe_data_level4_cnt;
			break;
		case 5:
			cnt = mDNIe_data_level5_cnt;
			break;
		}

		cnt--;	/*remove END_SEQ*/

		while (cnt > 0)	{
			if ((*(buf+cnt-1)) == 0x0100) {
				if (init_mdnie == 0)
					pre_0x0100 = (*(buf+cnt));
				if (pre_0x0100 < (*(buf+cnt))) {
					while ((pre_0x0100 < (*(buf+cnt))) && (pre_0x0100 <= 0x8080) && (pre_0x0100 >= 0x0000)) {
						s3c_mdnie_writel(pre_0x0100, (*(buf+cnt-1)));
						printk("[mDNIe] mDNIe_tuning_initialize: addr(0x%x), data(0x%x)  \n", (*(buf+cnt-1)), pre_0x0100);
						pre_0x0100 = ((pre_0x0100 & 0xff00) + (light_step<<8)) | ((pre_0x0100 & 0x00ff) + (saturation_step));
					}
				} else if (pre_0x0100 > (*(buf+cnt))) {
					while (pre_0x0100 > (*(buf+cnt)) && (pre_0x0100 >= 0x0000) && (pre_0x0100 <= 0x8080)) {
						s3c_mdnie_writel(pre_0x0100, (*(buf+cnt-1)));
						printk("[mDNIe] mDNIe_tuning_initialize: addr(0x%x), data(0x%x)  \n", (*(buf+cnt-1)), pre_0x0100);
						pre_0x0100 = ((pre_0x0100 & 0xff00) - (light_step<<8)) | ((pre_0x0100 & 0x00ff) - (saturation_step));
					}
				}
				s3c_mdnie_writel((*(buf+cnt)), (*(buf+cnt-1)));
				pre_0x0100 = (*(buf+cnt));
			} else if ((*(buf+cnt-1)) == 0x00AC) {
				if (init_mdnie == 0)
					pre_0x00AC = (*(buf+cnt));
				if (pre_0x00AC < (*(buf+cnt))) {
					while (pre_0x00AC < (*(buf+cnt)) && (pre_0x00AC <= 0x03ff) && (pre_0x00AC >= 0x0000)) {
						s3c_mdnie_writel(pre_0x00AC, (*(buf+cnt-1)));
						printk("[mDNIe] mDNIe_tuning_initialize: addr(0x%x), data(0x%x)  \n", (*(buf+cnt-1)), pre_0x00AC);
						pre_0x00AC += (cs_step);
					}
				} else if (pre_0x00AC > (*(buf+cnt))) {
					while (pre_0x00AC > (*(buf+cnt)) && (pre_0x00AC >= 0x0000) && (pre_0x00AC <= 0x03ff)) {
						s3c_mdnie_writel(pre_0x00AC, (*(buf+cnt-1)));
						printk("[mDNIe] mDNIe_tuning_initialize: addr(0x%x), data(0x%x)  \n", (*(buf+cnt-1)), pre_0x00AC);
						pre_0x00AC -= (cs_step);
					}
				}
				s3c_mdnie_writel((*(buf+cnt)), (*(buf+cnt-1)));
				pre_0x00AC = (*(buf+cnt));
			} else {
				/*s3c_mdnie_writel((*(buf+i+1)), (*(buf+i)));*/
				s3c_mdnie_writel((*(buf+cnt)), (*(buf+cnt-1)));
			}

			printk(KERN_INFO "[mDNIe] mDNIe_tuning_initialize: addr(0x%x), data(0x%x)  \n", (*(buf+cnt-1)), (*(buf+cnt)));

			cnt -= 2;
		}
	}
	s3c_mdnie_unmask();
}

int mdnie_lock;

void mDNIe_Set_Register_for_lightsensor(int adc)
{

	if (init_mdnie == 0)
		pre_adc_level = cur_adc_level;

	if (!mdnie_lock) {
		mdnie_lock = 1;

		if ((adc >= adc_level_formDNIe[0]) && (adc < adc_level_formDNIe[1])) {
			cur_adc_level = 0;

			if (mdnie_level != 0)
				mDNIe_Mode_set_for_lightsensor(mDNIe_data_level0);

			mdnie_level = 0;
		} else if ((adc >= adc_level_formDNIe[1]) && (adc < adc_level_formDNIe[2])) {
			cur_adc_level = 1;

			if (mdnie_level != 1)
				mDNIe_Mode_set_for_lightsensor(mDNIe_data_level1);

			mdnie_level = 1;
		} else if ((adc >= adc_level_formDNIe[2]) && (adc < adc_level_formDNIe[3])) {
			cur_adc_level = 2;

			if (mdnie_level != 2)
				mDNIe_Mode_set_for_lightsensor(mDNIe_data_level2);

			mdnie_level = 2;
		} else if ((adc >= adc_level_formDNIe[3]) && (adc < adc_level_formDNIe[4])) {
			cur_adc_level = 3;

			if (mdnie_level != 3)
				mDNIe_Mode_set_for_lightsensor(mDNIe_data_level3);

			mdnie_level = 3;
		} else if ((adc >= adc_level_formDNIe[4]) && (adc < adc_level_formDNIe[5])) {
			cur_adc_level = 4;

			if (mdnie_level != 4)
				mDNIe_Mode_set_for_lightsensor(mDNIe_data_level4);

			mdnie_level = 4;
		} else if (adc >= adc_level_formDNIe[5]) {
			cur_adc_level = 5;

			if (mdnie_level != 5)
				mDNIe_Mode_set_for_lightsensor(mDNIe_data_level5);

			mdnie_level = 5;
		}

		pre_adc_level = cur_adc_level;

		init_mdnie = 1;

		mdnie_lock = 0;
	} else
		printk("[mDNIe] mDNIe_tuning -  mdnie_lock(%d) \n", mdnie_lock);
}
EXPORT_SYMBOL(mDNIe_Set_Register_for_lightsensor);


void mDNIe_tuning_set(void)
{
	u32 i = 0;

	while (mDNIe_data[i] != END_SEQ) {
		s3c_mdnie_writel(mDNIe_data[i+1], mDNIe_data[i]);
		//printk(KERN_INFO "[mDNIe] mDNIe_tuning_initialize: addr(0x%x), data(0x%x)  \n", mDNIe_data[i], mDNIe_data[i+1]);
		i += 2;
	}
	s3c_mdnie_unmask();
}

static int parse_text(char *src, int len)
{
	int i, count, ret;
	int index = 0;
	char *str_line[100];
	char *sstart;
	char *c;
	unsigned int data1, data2;

	c = src;
	count = 0;
	sstart = c;

	for (i = 0; i < len; i++, c++) {
		char a = *c;
		if (a == '\r' || a == '\n') {
			if (c > sstart) {
				str_line[count] = sstart;
				count++;
			}
			*c = '\0';
			sstart = c+1;
		}
	}

	if (c > sstart) {
		str_line[count] = sstart;
		count++;
	}

	//printk(KERN_INFO "----------------------------- Total number of lines:%d\n", count);

	for (i = 0; i < count; i++) {
		//printk(KERN_INFO "line:%d, [start]%s[end]\n", i, str_line[i]);
		ret = sscanf(str_line[i], "0x%x,0x%x\n", &data1, &data2);
		//printk(KERN_INFO "Result => [0x%2x 0x%4x] %s\n", data1, data2, (ret == 2) ? "Ok" : "Not available");
		if (ret == 2) {
			mDNIe_data[index++] = (u16)data1 * 4;
			mDNIe_data[index++]  = (u16)data2;
		}
	}
	return index;
}


int mDNIe_txtbuf_to_parsing(char const*  pFilepath)
{
	struct file *filp;
	char	*dp;
	long	l;
	loff_t  pos;
	int     ret, num;
	mm_segment_t fs;

	mutex_lock(&mdnie_use);

	fs = get_fs();
	set_fs(get_ds());

	if(!pFilepath){
		printk("Error : mDNIe_txtbuf_to_parsing has invalid filepath. \n");
		goto parse_err;
	}

	filp = filp_open(pFilepath, O_RDONLY, 0);

	if (IS_ERR(filp)) {
		printk("file open error:%d\n", (s32)filp);
		goto parse_err;
	}

	l = filp->f_path.dentry->d_inode->i_size;
	dp = kmalloc(l+10, GFP_KERNEL);		/* add cushion : origianl code is 'dp = kmalloc(l, GFP_KERNEL);' */
	if (dp == NULL) {
		printk(KERN_INFO "Out of Memory!\n");
		filp_close(filp, current->files);
		goto parse_err;
	}
	pos = 0;
	memset(dp, 0, l);
	ret = vfs_read(filp, (char __user *)dp, l, &pos);   /* P1_LSJ : DE08 : died at here  */

	if (ret != l) {
		printk(KERN_INFO "<CMC623> Failed to read file (ret = %d)\n", ret);
		kfree(dp);
		filp_close(filp, current->files);
		goto parse_err;
	}

	filp_close(filp, current->files);
	set_fs(fs);
	num = parse_text(dp, l);
	if (!num) {
		printk("Nothing to parse!\n");
		kfree(dp);
		goto parse_err;
	}

	mDNIe_data[num] = END_SEQ;
	mDNIe_Tuning_Mode = TRUE;
	mDNIe_tuning_set();

	kfree(dp);

	mutex_unlock(&mdnie_use);
	return (num / 2);

parse_err:
	mutex_unlock(&mdnie_use);
	return -1;
}
EXPORT_SYMBOL(mDNIe_txtbuf_to_parsing);
#endif

#ifdef CONFIG_FB_S3C_MDNIE_TUNINGMODE_FOR_BACKLIGHT
int mdnie_tuning_backlight;

extern int IsLDIEnabled(void);
void mDNIe_Mode_set_for_backlight(u16 *buf)
{
	u32 i = 0;
	int cnt = 0;

	if (IsLDIEnabled()) {
		mutex_lock(&mdnie_use);

		/*if (mdnie_tuning_backlight)*/
		{
			while ((*(buf+i)) != END_SEQ) {
				if ((*(buf+i)) == 0x0100) {
					if (pre_0x0100 < (*(buf+(i+1)))) {
						while ((pre_0x0100 < (*(buf+(i+1)))) && (pre_0x0100 <= 0x4020) && (pre_0x0100 >= 0x0000)) {
							s3c_mdnie_writel(pre_0x0100, (*(buf+i)));
							gprintk("[mDNIe] mDNIe_tuning_initialize: addr(0x%x), data(0x%x)  \n", (*(buf+i)), pre_0x0100);

							if ((pre_0x0100 & 0x00ff) == 0x20)
								pre_0x0100 = ((pre_0x0100 & 0xff00) + (0x8<<8)) | (0x20);
							else
								pre_0x0100 = ((pre_0x0100 & 0xff00) + (0x8<<8)) | ((pre_0x0100 & 0x00ff) + (0x4));
							msleep(20);
						}
					} else if (pre_0x0100 > (*(buf+(i+1)))) {
						while (pre_0x0100 > (*(buf+(i+1))) && (pre_0x0100 >= 0x0000) && (pre_0x0100 <= 0x4020)) {
							s3c_mdnie_writel(pre_0x0100, (*(buf+i)));
							gprintk("[mDNIe] mDNIe_tuning_initialize: addr(0x%x), data(0x%x)  \n", (*(buf+i)), pre_0x0100);

							if ((pre_0x0100 & 0x00ff) == 0x00)
								pre_0x0100 = ((pre_0x0100 & 0xff00) - (0x8<<8)) | (0x00);
							else
								pre_0x0100 = ((pre_0x0100 & 0xff00) - (0x8<<8)) | ((pre_0x0100 & 0x00ff) - (0x4));
							msleep(20);
						}
					}
					s3c_mdnie_writel((*(buf+i+1)), (*(buf+i)));
					pre_0x0100 = (*(buf+i+1));
				} else {
					s3c_mdnie_writel((*(buf+i+1)), (*(buf+i)));
				}
				gprintk("[mDNIe] mDNIe_Mode_set_for_backlight : addr(0x%x), data(0x%x)  \n", (*(buf+i)), (*(buf+(i+1))));
				i += 2;
			}

			s3c_mdnie_unmask();
		}

		mutex_unlock(&mdnie_use);
	}
}
EXPORT_SYMBOL(mDNIe_Mode_set_for_backlight);
#endif

int s3c_mdnie_init_global(struct s3cfb_global *s3cfb_ctrl)
{
	s3c_mdnie_set_size(s3cfb_ctrl->lcd->width, s3cfb_ctrl->lcd->height);
	s3c_ielcd_logic_start();
	s3c_ielcd_init_global(s3cfb_ctrl);

	return 0;
}

int s3c_mdnie_start(struct s3cfb_global *ctrl)
{
	/* s3c_ielcd_set_clock(ctrl); */
	s3c_ielcd_start();

	g_mdine_enable = 1;

	return 0;
}

int s3c_mdnie_off(void)
{
	g_mdine_enable = 0;

	s3c_ielcd_logic_stop();

	return 0;
}


int s3c_mdnie_stop(void)
{
	s3c_ielcd_stop();

	return 0;
}


MODULE_AUTHOR("lsi");
MODULE_DESCRIPTION("S3C MDNIE Device Driver");
MODULE_LICENSE("GPL");
