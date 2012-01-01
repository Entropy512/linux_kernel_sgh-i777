/* linux/drivers/video/samsung/s3cfb-main.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Core file for Samsung Display Controller (FIMD) driver
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
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/memory.h>
#include <linux/pm_runtime.h>
#include <linux/delay.h>
#include <plat/clock.h>
#include <plat/media.h>
#include <mach/media.h>
#include <mach/map.h>
#include <linux/serial_core.h>
#include <plat/regs-serial.h>
#include <plat/s5pv310.h>
#include "s3cfb.h"
#ifdef CONFIG_TARGET_LOCALE_KOR
#include <mach/regs-clock.h>
#endif
#ifdef CONFIG_FB_S3C_MDNIE
#include "s3cfb_mdnie.h"
#endif
#ifdef CONFIG_HAS_WAKELOCK
#include <linux/wakelock.h>
#include <linux/earlysuspend.h>
#include <linux/suspend.h>
#endif

#ifdef CONFIG_TARGET_LOCALE_KOR
#define DISPLAY_BOOT_PROGRESS
#define LOGO_MEM_SIZE		        800*480*4
#define LPDDR1_BASE_ADDR	        0x50000000
#define LOGO_MEM_BASE		        (LPDDR1_BASE_ADDR + 0x0EC00000)	/* 0x5EC00000 from Bootloader */
#endif

struct s3cfb_fimd_desc		*fbfimd;

inline struct s3cfb_global *get_fimd_global(int id)
{
	struct s3cfb_global *fbdev;

	if (id < 5)
		fbdev = fbfimd->fbdev[0];
	else
		fbdev = fbfimd->fbdev[1];

	return fbdev;
}

#ifdef DISPLAY_BOOT_PROGRESS
static int progress_flag = 0;
static int progress_pos;
static struct timer_list progress_timer;

#define PROGRESS_BAR_LEFT_POS	    54
#define PROGRESS_BAR_RIGHT_POS	    425
#define PROGRESS_BAR_START_Y	    576
#define PROGRESS_BAR_WIDTH	    4
#define PROGRESS_BAR_HEIGHT	    8

static unsigned char anycall_progress_bar_left[] =
{	
	0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00,
	0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00,
	0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00, 0xf3, 0xc5, 0x00, 0x00, 0xf3, 0xc5, 0x00, 0x00,
	0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00, 0xf3, 0xc5, 0x00, 0x00, 0xf3, 0xc5, 0x00, 0x00,
	0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00, 0xf3, 0xc5, 0x00, 0x00, 0xf3, 0xc5, 0x00, 0x00,
	0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00, 0xf3, 0xc5, 0x00, 0x00, 0xf3, 0xc5, 0x00, 0x00,
	0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00,
	0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00
};

static unsigned char anycall_progress_bar_right[] =
{	
	0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00,
	0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00,
	0xf3, 0xc5, 0x00, 0x00, 0xf3, 0xc5, 0x00, 0x00, 0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00, 
	0xf3, 0xc5, 0x00, 0x00, 0xf3, 0xc5, 0x00, 0x00, 0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00, 
	0xf3, 0xc5, 0x00, 0x00, 0xf3, 0xc5, 0x00, 0x00, 0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00, 
	0xf3, 0xc5, 0x00, 0x00, 0xf3, 0xc5, 0x00, 0x00, 0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00, 
	0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00,
	0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00
};

static unsigned char anycall_progress_bar_center[] =
{
	0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00, 0xf3, 0xc5, 0x00, 0x00, 0xf3, 0xc5, 0x00, 0x00,
	0xf3, 0xc5, 0x00, 0x00, 0xf3, 0xc5, 0x00, 0x00, 0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00
};

static unsigned char anycall_progress_bar[] = 
{
	0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00,
	0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00,
	0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00,
	0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00,
	0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00,
	0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00,
	0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00,
	0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x33, 0x00
};

static void s3cfb_start_progress(struct fb_info *fb);
static void s3cfb_stop_progress(void);
static void progress_timer_handler(unsigned long data);

static int show_progress = 1;
module_param_named(progress, show_progress, bool, 0);
#endif

static irqreturn_t s3cfb_irq_frame(int irq, void *dev_id)
{
	struct s3cfb_global *fbdev[2];
	fbdev[0] = fbfimd->fbdev[0];

	s3cfb_clear_interrupt(fbdev[0]);

	fbdev[0]->wq_count++;
	wake_up(&fbdev[0]->wq);

	return IRQ_HANDLED;
}

#ifdef CONFIG_FB_S3C_TRACE_UNDERRUN
static irqreturn_t s3cfb_irq_fifo(int irq, void *dev_id)
{
	struct s3cfb_global *fbdev[2];
	fbdev[0] = fbfimd->fbdev[0];

	s3cfb_clear_interrupt(fbdev[0]);

	return IRQ_HANDLED;
}
#endif

int s3cfb_register_framebuffer(struct s3cfb_global *fbdev)
{
	struct s3c_platform_fb *pdata = to_fb_plat(fbdev->dev);
	int ret, i, j;

	/* on registering framebuffer, framebuffer of default window is registered at first. */
	for (i = pdata->default_win; i < pdata->nr_wins + pdata->default_win; i++) {
		j = i % pdata->nr_wins;
		ret = register_framebuffer(fbdev->fb[j]);
		if (ret) {
			dev_err(fbdev->dev, "failed to register	\
				framebuffer device\n");
			return -EINVAL;
		}
#ifndef CONFIG_FRAMEBUFFER_CONSOLE
		if (j == pdata->default_win) {
			s3cfb_check_var_window(fbdev, &fbdev->fb[j]->var,
					fbdev->fb[j]);
			s3cfb_set_par_window(fbdev, fbdev->fb[j]);
			s3cfb_draw_logo(fbdev->fb[j]);
		}
#endif
	}
	return 0;
}

static int s3cfb_sysfs_show_win_power(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct s3c_platform_fb *pdata = to_fb_plat(dev);
	struct s3cfb_window *win;
	char temp[16];
	int i;
	struct s3cfb_global *fbdev[1];
	fbdev[0] = fbfimd->fbdev[0];

	for (i = 0; i < pdata->nr_wins; i++) {
		win = fbdev[0]->fb[i]->par;
		sprintf(temp, "[fb%d] %s\n", i, win->enabled ? "on" : "off");
		strcat(buf, temp);
	}

	return strlen(buf);
}

#ifdef CONFIG_FB_S3C_MIPI_LCD
void s3cfb_trigger(void)
{
	struct s3cfb_global *fbdev = fbfimd->fbdev[0];
	struct s3c_platform_fb *pdata = to_fb_plat(fbdev->dev);

	if (pdata == NULL) {
	    dev_err(fbdev->dev, "failed to get defualt window number.\n");
	    return;
	}
	s3cfb_set_trigger(fbdev);
}
EXPORT_SYMBOL(s3cfb_trigger);
#endif

static int s3cfb_sysfs_store_win_power(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t len)
{
	struct s3c_platform_fb *pdata = to_fb_plat(dev);
	char temp[4] = { 0, };
	const char *p = buf;
	int id, to;
	struct s3cfb_global *fbdev[1];
	fbdev[0] = fbfimd->fbdev[0];

	while (*p != '\0') {
		if (!isspace(*p))
			strncat(temp, p, 1);
		p++;
	}

	if (strlen(temp) != 2)
		return -EINVAL;

	id = simple_strtoul(temp, NULL, 10) / 10;
	to = simple_strtoul(temp, NULL, 10) % 10;

	if (id < 0 || id > pdata->nr_wins)
		return -EINVAL;

	if (to != 0 && to != 1)
		return -EINVAL;

	if (to == 0)
		s3cfb_disable_window(fbdev[0], id);
	else
		s3cfb_enable_window(fbdev[0], id);

	return len;
}

static DEVICE_ATTR(win_power, 0664,
	s3cfb_sysfs_show_win_power, s3cfb_sysfs_store_win_power);


#ifdef CONFIG_FB_S3C_MDNIE
static int s3cfb_sysfs_store_mdnie_power(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t len)
{
	u32 reg, i, enable;
	struct s3cfb_global *fbdev[2];

	sscanf(buf, "%d", &enable);

	if (enable) {
		for (i = 0; i < FIMD_MAX; i++) {
			fbdev[i] = fbfimd->fbdev[i];
			reg = readl(S3C_VA_SYS + 0x0210);
			reg &= ~(1<<13);
			reg &= ~(1<<12);
			reg &= ~(3<<10);
			reg |= (1<<0);
			reg &= ~(1<<1);
			writel(reg, S3C_VA_SYS + 0x0210);
			writel(3, fbdev[i]->regs + 0x27c);
			s3c_mdnie_init_global(fbdev[i]);
			s3c_mdnie_start(fbdev[i]);
		}
		printk("s3cfb_sysfs_store_mdnie_power() is called : mDNIE is ON \n");
	}
	else {
		for (i = 0; i < FIMD_MAX; i++) {
			fbdev[i] = fbfimd->fbdev[i];
			writel(0, fbdev[i]->regs + 0x27c);
			msleep(20);
			reg = readl(S3C_VA_SYS + 0x0210);
			reg |= (1<<1);
			writel(reg, S3C_VA_SYS + 0x0210);
			s3c_mdnie_stop();
			s3c_mdnie_off();
		}
		printk("s3cfb_sysfs_store_mdnie_power() is called : mDNIE is OFF \n");
	}

	return len;
}

static DEVICE_ATTR(mdnie_power, 0664, NULL, s3cfb_sysfs_store_mdnie_power);
#endif

#ifdef DISPLAY_BOOT_PROGRESS
static void s3cfb_update_framebuffer(struct fb_info *fb,int x, int y, void *buffer,int src_width, int src_height)
{
	struct s3cfb_global *fbdev =
			platform_get_drvdata(to_platform_device(fb->device));
	struct fb_fix_screeninfo *fix = &fb->fix;
	struct fb_var_screeninfo *var = &fb->var;
	int row;
	int bytes_per_pixel = (var->bits_per_pixel / 8 );
	
	unsigned char *pSrc = buffer;
	unsigned char *pDst = fb->screen_base;

	if (x+src_width > var->xres || y+src_height > var->yres){
		dev_err(fbdev->dev,"invalid destination coordinate or source size (%d, %d) (%d %d) \n", x, y, src_width, src_height);
		return;
	}	

	pDst += y * fix->line_length + x * bytes_per_pixel;	

	for (row = 0; row < src_height ; row++){		
		memcpy(pDst, pSrc, src_width * bytes_per_pixel);
		pSrc += src_width * bytes_per_pixel;
		pDst += fix->line_length;
	}
}

static void s3cfb_start_progress(struct fb_info *fb)
{	
	int x_pos;
	init_timer(&progress_timer);	

	progress_timer.expires  = (get_jiffies_64() + (HZ/20));	
	progress_timer.data     = (long)fb;	
	progress_timer.function = progress_timer_handler;	
	progress_pos = PROGRESS_BAR_LEFT_POS;	

	// draw progress background.
	for (x_pos = PROGRESS_BAR_LEFT_POS ; x_pos <= PROGRESS_BAR_RIGHT_POS ; x_pos += PROGRESS_BAR_WIDTH){
		s3cfb_update_framebuffer(fb,
			x_pos,
			PROGRESS_BAR_START_Y,
			(void*)anycall_progress_bar,					
			PROGRESS_BAR_WIDTH,
			PROGRESS_BAR_HEIGHT);
	}
	s3cfb_update_framebuffer(fb,
		PROGRESS_BAR_LEFT_POS,
		PROGRESS_BAR_START_Y,
		(void*)anycall_progress_bar_left,					
		PROGRESS_BAR_WIDTH,
		PROGRESS_BAR_HEIGHT);
	
	progress_pos += PROGRESS_BAR_WIDTH;	
	
	s3cfb_update_framebuffer(fb,		
		progress_pos,
		PROGRESS_BAR_START_Y,		
		(void*)anycall_progress_bar_right,				
		PROGRESS_BAR_WIDTH,
		PROGRESS_BAR_HEIGHT);
	
	add_timer(&progress_timer);	
	progress_flag = 1;

}

static void s3cfb_stop_progress(void)
{	
	if (progress_flag == 0)		
		return;	
	del_timer(&progress_timer);	
	progress_flag = 0;
}

static void progress_timer_handler(unsigned long data)
{	
	int i;	
	for(i = 0; i < 4; i++){		
		s3cfb_update_framebuffer((struct fb_info *)data,
			progress_pos++,
			PROGRESS_BAR_START_Y,
			(void*)anycall_progress_bar_center,					
			1,
			PROGRESS_BAR_HEIGHT);	
	}	
	
	s3cfb_update_framebuffer((struct fb_info *)data,		
		progress_pos,
		PROGRESS_BAR_START_Y,
		(void*)anycall_progress_bar_right,		
		PROGRESS_BAR_WIDTH,
		PROGRESS_BAR_HEIGHT);    
	
	if (progress_pos + PROGRESS_BAR_WIDTH >= PROGRESS_BAR_RIGHT_POS )
	{        
		s3cfb_stop_progress();    
	}    
	else    
	{        
		progress_timer.expires = (get_jiffies_64() + (HZ/20));         
		progress_timer.function = progress_timer_handler;         
		add_timer(&progress_timer);    
	}
}
#endif

static int s3cfb_probe(struct platform_device *pdev)
{
	struct s3c_platform_fb *pdata = NULL;
	struct resource *res = NULL;
	struct s3cfb_global *fbdev[2];
	int ret = 0;
	int i = 0;
#ifdef CONFIG_FB_S3C_MDNIE
	u32 reg;
#endif

#ifdef CONFIG_S5PV310_DEV_PD
	/* to use the runtime PM helper functions */
	pm_runtime_enable(&pdev->dev);
	/* enable the power domain */
	pm_runtime_get_sync(&pdev->dev);
#endif
	fbfimd = kzalloc(sizeof(struct s3cfb_fimd_desc), GFP_KERNEL);

	if (FIMD_MAX == 2)
		fbfimd->dual = 1;
	else
		fbfimd->dual = 0;

	for (i = 0; i < FIMD_MAX; i++) {
		/* global structure */
		fbfimd->fbdev[i] = kzalloc(sizeof(struct s3cfb_global), GFP_KERNEL);
		fbdev[i] = fbfimd->fbdev[i];
		if (!fbdev[i]) {
			dev_err(fbdev[i]->dev, "failed to allocate for	\
				global fb structure fimd[%d]!\n", i);
			goto err0;
		}

		fbdev[i]->dev = &pdev->dev;

		/* platform_data*/
		pdata = to_fb_plat(&pdev->dev);
		fbdev[i]->lcd = (struct s3cfb_lcd *)pdata->lcd;

		if (pdata->cfg_gpio)
			pdata->cfg_gpio(pdev);

		if (pdata->clk_on)
			pdata->clk_on(pdev, &fbdev[i]->clock);

		/* io memory */
		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (!res) {
			dev_err(fbdev[i]->dev,
				"failed to get io memory region\n");
			ret = -EINVAL;
			goto err1;
		}
		res = request_mem_region(res->start,
					res->end - res->start + 1, pdev->name);
		if (!res) {
			dev_err(fbdev[i]->dev,
				"failed to request io memory region\n");
			ret = -EINVAL;
			goto err1;
		}
		fbdev[i]->regs = ioremap(res->start, res->end - res->start + 1);
		if (!fbdev[i]->regs) {
			dev_err(fbdev[i]->dev, "failed to remap io region\n");
			ret = -EINVAL;
			goto err1;
		}

		fbdev[i]->wq_count = 0;
		init_waitqueue_head(&fbdev[i]->wq);

		/* irq */
		fbdev[i]->irq = platform_get_irq(pdev, 0);
		if (request_irq(fbdev[i]->irq, s3cfb_irq_frame, IRQF_SHARED,
				pdev->name, fbdev[i])) {
			dev_err(fbdev[i]->dev, "request_irq failed\n");
			ret = -EINVAL;
			goto err2;
		}

#ifdef CONFIG_FB_S3C_TRACE_UNDERRUN
		if (request_irq(platform_get_irq(pdev, 1), s3cfb_irq_fifo,
				IRQF_DISABLED, pdev->name, fbdev[i])) {
			dev_err(fbdev[i]->dev, "request_irq failed\n");
			ret = -EINVAL;
			goto err2;
		}

		s3cfb_set_fifo_interrupt(fbdev[i], 1);
		dev_info(fbdev[i]->dev, "fifo underrun trace\n");
#endif
#ifdef CONFIG_FB_S3C_MDNIE
		/* only FIMD0 is supported */
		if (i == 0)
			s3c_mdnie_setup();
#endif
		/* hw setting */
		s3cfb_init_global(fbdev[i]);

		/* alloc fb_info */
		if (s3cfb_alloc_framebuffer(fbdev[i], i)) {
			dev_err(fbdev[i]->dev, "alloc error fimd[%d]\n", i);
			goto err3;
		}

		/* register fb_info */
		if (s3cfb_register_framebuffer(fbdev[i])) {
			dev_err(fbdev[i]->dev, "register error fimd[%d]\n", i);
			goto err3;
		}

		/* enable display */
		s3cfb_set_clock(fbdev[i]);

		/* Set Alpha value width to 8-bit alpha value
		 * 1 : 8bit mode
		 * 2 : 4bit mode
		 */
		s3cfb_set_alpha_value(fbdev[i], 1);

#ifdef CONFIG_FB_S3C_MDNIE
		/* only FIMD0 is supported */
		if (i == 0) {
			reg = readl(S3C_VA_SYS + 0x0210);
			reg &= ~(1<<13);
			reg &= ~(1<<12);
			reg &= ~(3<<10);
			reg |= (1<<0);
			reg &= ~(1<<1);
			writel(reg, S3C_VA_SYS + 0x0210);
			writel(3, fbdev[i]->regs + 0x27c);

			s3c_mdnie_init_global(fbdev[i]);
			s3c_mdnie_start(fbdev[i]);
		}
#endif
		s3cfb_enable_window(fbdev[0], pdata->default_win);
		s3cfb_update_power_state(fbdev[i], pdata->default_win,
					FB_BLANK_UNBLANK);
		s3cfb_display_on(fbdev[i]);

		fbdev[i]->system_state = POWER_ON;
#ifdef CONFIG_HAS_WAKELOCK
#ifdef CONFIG_HAS_EARLYSUSPEND
		fbdev[i]->early_suspend.suspend = s3cfb_early_suspend;
		fbdev[i]->early_suspend.resume = s3cfb_late_resume;
		fbdev[i]->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB;
		register_early_suspend(&fbdev[i]->early_suspend);
#endif
#endif
	}
#ifdef CONFIG_FB_S3C_LCD_INIT
	/* panel control */
	if (pdata->backlight_on)
		pdata->backlight_on(pdev);

	if (pdata->lcd_on)
		pdata->lcd_on(pdev);
#endif

	ret = device_create_file(&(pdev->dev), &dev_attr_win_power);
	if (ret < 0)
		dev_err(fbdev[0]->dev, "failed to add sysfs entries : win_power\n");

#ifdef CONFIG_FB_S3C_MDNIE
	ret = device_create_file(&(pdev->dev), &dev_attr_mdnie_power);
	if (ret < 0)
		dev_err(fbdev[0]->dev, "failed to add sysfs entries : mdnie_power\n");

	init_mdnie_class();
#endif

#ifdef DISPLAY_BOOT_PROGRESS
        if(!(readl(S5P_INFORM2)) && show_progress)    
        {
            s3cfb_start_progress(fbdev[0]->fb[pdata->default_win]);
        }
#endif

	dev_info(fbdev[0]->dev, "registered successfully\n");

	return 0;

err3:
	for (i = 0; i < FIMD_MAX; i++)
		free_irq(fbdev[i]->irq, fbdev[i]);
err2:
	for (i = 0; i < FIMD_MAX; i++)
		iounmap(fbdev[i]->regs);
err1:
	for (i = 0; i < FIMD_MAX; i++)
		pdata->clk_off(pdev, &fbdev[i]->clock);
err0:
	return ret;
}

static int s3cfb_remove(struct platform_device *pdev)
{
	struct s3c_platform_fb *pdata = to_fb_plat(&pdev->dev);
	struct s3cfb_window *win;
	struct fb_info *fb;
	struct s3cfb_global *fbdev[2];
	int i;
	int j;

	for (i = 0; i < FIMD_MAX; i++) {
		fbdev[i] = fbfimd->fbdev[i];

#ifdef CONFIG_HAS_WAKELOCK
#ifdef CONFIG_HAS_EARLYSUSPEND
		unregister_early_suspend(&fbdev[i]->early_suspend);
#endif
#endif
		free_irq(fbdev[i]->irq, fbdev[i]);
		iounmap(fbdev[i]->regs);
		pdata->clk_off(pdev, &fbdev[i]->clock);

		for (j = 0; j < pdata->nr_wins; j++) {
			fb = fbdev[i]->fb[j];

			/* free if exists */
			if (fb) {
				win = fb->par;
				if (win->id == pdata->default_win)
					s3cfb_unmap_default_video_memory(fbdev[i], fb);
				else
					s3cfb_unmap_video_memory(fbdev[i], fb);

				s3cfb_set_buffer_address(fbdev[i], j);
				framebuffer_release(fb);
			}
		}

		kfree(fbdev[i]->fb);
		kfree(fbdev[i]);
	}
#ifdef CONFIG_S5PV310_DEV_PD
	/* disable the power domain */
	pm_runtime_put(&pdev->dev);
	pm_runtime_disable(&pdev->dev);
#endif
	return 0;
}

#ifdef CONFIG_PM
#ifdef CONFIG_HAS_EARLYSUSPEND

void s3cfb_early_suspend(struct early_suspend *h)
{
	struct s3cfb_global *info = container_of(h, struct s3cfb_global, early_suspend);
	struct s3c_platform_fb *pdata = to_fb_plat(info->dev);
	struct platform_device *pdev = to_platform_device(info->dev);
	struct s3cfb_global *fbdev[2];
	int i;
#ifdef CONFIG_FB_S3C_MDNIE
	u32 reg;
#endif

	printk("+s3cfb_early_suspend()\n");

	info->system_state = POWER_OFF;

	for (i = 0; i < FIMD_MAX; i++) {
		fbdev[i] = fbfimd->fbdev[i];

		if (pdata->backlight_off)
			pdata->backlight_off(pdev);
#if 0
		if (pdata->cfg_gpio_sleep)
			pdata->cfg_gpio_sleep(pdev);
#endif

#ifdef CONFIG_FB_S3C_MDNIE
		writel(0, fbdev[i]->regs + 0x27c);
		msleep(20);
		reg = readl(S3C_VA_SYS + 0x0210);
		reg |= (1<<1);
		writel(reg, S3C_VA_SYS + 0x0210);
		s3c_mdnie_stop();
#endif
		s3cfb_display_off(fbdev[i]);
#ifdef CONFIG_FB_S3C_MDNIE
		s3c_mdnie_off();
#endif
		if (pdata->clk_off)
			pdata->clk_off(pdev, &fbdev[i]->clock);
	}
#ifdef CONFIG_S5PV310_DEV_PD
	/* disable the power domain */
	//printk(KERN_DEBUG "s3cfb - disable power domain\n");
	pm_runtime_put_sync(&pdev->dev);
#endif
	printk("-s3cfb_early_suspend()\n");
	return ;
}

void s3cfb_late_resume(struct early_suspend *h)
{
	struct s3cfb_global *info = container_of(h, struct s3cfb_global, early_suspend);
	struct s3c_platform_fb *pdata = to_fb_plat(info->dev);
	struct fb_info *fb;
	struct s3cfb_window *win;
	struct s3cfb_global *fbdev[2];
	int i, j;
	struct platform_device *pdev = to_platform_device(info->dev);
	u32 reg;

	printk("+s3cfb_late_resume()\n");
	//dev_dbg(info->dev, "wake up from suspend\n");

#ifdef CONFIG_S5PV310_DEV_PD
	/* enable the power domain */
	//printk(KERN_DEBUG "s3cfb - enable power domain\n");
	pm_runtime_get_sync(&pdev->dev);
#endif

	info->system_state = POWER_ON;

	for (i = 0; i < FIMD_MAX; i++) {
		fbdev[i] = fbfimd->fbdev[i];
		if (pdata->cfg_gpio)
			pdata->cfg_gpio(pdev);

		if (pdata->backlight_on)
			pdata->backlight_on(pdev);

		if (pdata->lcd_on)
			pdata->lcd_on(pdev);

#if defined(CONFIG_FB_S3C_DUMMYLCD)
		max8698_ldo_enable_direct(MAX8698_LDO4);
#endif

		if (info->lcd->init_ldi)
			fbdev[i]->lcd->init_ldi();
		else
			printk("no init_ldi\n");

		if (pdata->clk_on)
			pdata->clk_on(pdev, &fbdev[i]->clock);

#ifdef CONFIG_FB_S3C_MDNIE
		reg = readl(S3C_VA_SYS + 0x0210);
		reg &= ~(1<<13);
		reg &= ~(1<<12);
		reg &= ~(3<<10);
		reg |= (1<<0);
		reg &= ~(1<<1);
		writel(reg, S3C_VA_SYS + 0x0210);
		writel(3, fbdev[i]->regs + 0x27c);
#endif
		s3cfb_init_global(fbdev[i]);
		s3cfb_set_clock(fbdev[i]);
#ifdef CONFIG_FB_S3C_MDNIE
		s3c_mdnie_init_global(fbdev[i]);
		s3c_mdnie_start(fbdev[i]);
		mDNIe_Init_Set_Mode();
#endif
		/* Set Alpha value width to 8-bit alpha value
		 * 1 : 8bit mode
		 * 2 : 4bit mode
		 */
		s3cfb_set_alpha_value(fbdev[i], 1);

		s3cfb_display_on(fbdev[i]);

		for (j = 0; j < pdata->nr_wins; j++) {
			fb = fbdev[i]->fb[j];
			win = fb->par;
			if ((win->path == DATA_PATH_DMA) && (win->enabled)) {
				s3cfb_set_par(fb);
				s3cfb_set_buffer_address(fbdev[i], win->id);
				s3cfb_enable_window(fbdev[i], win->id);
			}
		}
		if (pdata->backlight_on)
			pdata->backlight_on(pdev);
	}

	printk("-s3cfb_late_resume()\n");

	return;
}
#else /* else !CONFIG_HAS_EARLYSUSPEND */

int s3cfb_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct s3c_platform_fb *pdata = to_fb_plat(&pdev->dev);
	struct s3cfb_global *fbdev[2];
	int i;
	u32 reg;

	for (i = 0; i < FIMD_MAX; i++) {
		fbdev[i] = fbfimd->fbdev[i];

#ifdef CONFIG_FB_S3C_MDNIE
		writel(0, fbdev[i]->regs + 0x27c);
		msleep(20);
		reg = readl(S3C_VA_SYS + 0x0210);
		reg |= (1<<1);
		writel(reg, S3C_VA_SYS + 0x0210);
		s3c_mdnie_stop();
#endif
		if (atomic_read(&fbdev[i]->enabled_win) > 0) {
			/* lcd_off and backlight_off isn't needed. */
			if (fbdev[i]->lcd->deinit_ldi)
				fbdev[i]->lcd->deinit_ldi();

			s3cfb_display_off(fbdev[i]);
			pdata->clk_off(pdev, &fbdev[i]->clock);
		}
#ifdef CONFIG_FB_S3C_MDNIE
		s3c_mdnie_off();
#endif
	}

	return 0;
}

int s3cfb_resume(struct platform_device *pdev)
{
	struct s3c_platform_fb *pdata = to_fb_plat(&pdev->dev);
	struct fb_info *fb;
	struct s3cfb_window *win;
	struct s3cfb_global *fbdev[2];
	int i;
	int j;
	u32 reg;

	for (i = 0; i < FIMD_MAX; i++) {
		fbdev[i] = fbfimd->fbdev[i];
		dev_dbg(fbdev[i]->dev, "wake up from suspend fimd[%d]\n", i);

		if (pdata->cfg_gpio)
			pdata->cfg_gpio(pdev);

		if (pdata->backlight_on)
			pdata->backlight_on(pdev);
		if (pdata->lcd_on)
			pdata->lcd_on(pdev);
		if (fbdev[i]->lcd->init_ldi)
			fbdev[i]->lcd->init_ldi();

		if (pdata->backlight_off)
			pdata->backlight_off(pdev);
		if (pdata->lcd_off)
			pdata->lcd_off(pdev);
		if (fbdev[i]->lcd->deinit_ldi)
			fbdev[i]->lcd->deinit_ldi();

		if (atomic_read(&fbdev[i]->enabled_win) > 0) {
#ifdef CONFIG_FB_S3C_MDNIE
			reg = readl(S3C_VA_SYS + 0x0210);
			reg &= ~(1<<13);
			reg &= ~(1<<12);
			reg &= ~(3<<10);
			reg |= (1<<0);
			reg &= ~(1<<1);
			writel(reg, S3C_VA_SYS + 0x0210);
			writel(3, fbdev[i]->regs + 0x27c);
#endif
			pdata->clk_on(pdev, &fbdev[i]->clock);
			s3cfb_init_global(fbdev[i]);
			s3cfb_set_clock(fbdev[i]);
#ifdef CONFIG_FB_S3C_MDNIE
			s3c_mdnie_init_global(fbdev[i]);
			s3c_mdnie_start(fbdev[i]);
#endif

			for (j = 0; j < pdata->nr_wins; j++) {
				fb = fbdev[i]->fb[j];
				win = fb->par;
				if (win->owner == DMA_MEM_FIMD) {
					s3cfb_set_win_params(fbdev[i], win->id);
					if (win->enabled) {
						if (win->power_state == FB_BLANK_NORMAL)
							s3cfb_win_map_on(fbdev[i], win->id, 0x0);

						s3cfb_enable_window(fbdev[i], win->id);
					}
				}
			}

			s3cfb_display_on(fbdev[i]);

			if (pdata->backlight_on)
				pdata->backlight_on(pdev);

			if (pdata->lcd_on)
				pdata->lcd_on(pdev);

			if (fbdev[i]->lcd->init_ldi)
				fbdev[i]->lcd->init_ldi();
		}
	}

	return 0;
}
#endif
#else
#define s3cfb_suspend NULL
#define s3cfb_resume NULL
#endif

#ifdef CONFIG_S5PV310_DEV_PD
static int s3cfb_runtime_suspend(struct device *dev)
{
	return 0;
}

static int s3cfb_runtime_resume(struct device *dev)
{
	return 0;
}

static const struct dev_pm_ops s3cfb_pm_ops = {
	.runtime_suspend = s3cfb_runtime_suspend,
	.runtime_resume = s3cfb_runtime_resume,
};
#endif

static struct platform_driver s3cfb_driver = {
	.probe		= s3cfb_probe,
	.remove		= s3cfb_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend	= s3cfb_suspend,
	.resume		= s3cfb_resume,
#endif
	.driver		= {
		.name	= S3CFB_NAME,
		.owner	= THIS_MODULE,
#ifdef CONFIG_S5PV310_DEV_PD
		.pm	= &s3cfb_pm_ops,
#endif
	},
};

struct fb_ops s3cfb_ops = {
	.owner		= THIS_MODULE,
	.fb_open	= s3cfb_open,
	.fb_release	= s3cfb_release,
	.fb_check_var	= s3cfb_check_var,
	.fb_set_par	= s3cfb_set_par,
	.fb_setcolreg	= s3cfb_setcolreg,
	.fb_blank	= s3cfb_blank,
	.fb_pan_display	= s3cfb_pan_display,
	.fb_fillrect	= cfb_fillrect,
	.fb_copyarea	= cfb_copyarea,
	.fb_imageblit	= cfb_imageblit,
	.fb_cursor	= s3cfb_cursor,
	.fb_ioctl	= s3cfb_ioctl,
};

static int s3cfb_register(void)
{
	platform_driver_register(&s3cfb_driver);
	return 0;
}

static void s3cfb_unregister(void)
{
	platform_driver_unregister(&s3cfb_driver);
}

module_init(s3cfb_register);
module_exit(s3cfb_unregister);

MODULE_AUTHOR("Jingoo Han <jg1.han@samsung.com>");
MODULE_AUTHOR("Jonghun, Han <jonghun.han@samsung.com>");
MODULE_AUTHOR("Jinsung, Yang <jsgood.yang@samsung.com>");
MODULE_DESCRIPTION("Samsung Display Controller (FIMD) driver");
MODULE_LICENSE("GPL");
