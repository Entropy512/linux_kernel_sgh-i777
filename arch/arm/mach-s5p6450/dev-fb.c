/* linux/arch/arm/mach-s5p6450/dev-fb.c
 *
 * Copyright 2009 Samsung Electronics
 *	Jinsung Yang <jsgood.yang@samsung.com>
 *	http://samsungsemi.com/
 *
 * S5P64XX series device definition for Samsung Display Controller (FIMD)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/platform_device.h>

#include <mach/map.h>

#include <plat/fb.h>
#include <plat/devs.h>

#include <mach/irqs.h>

static struct resource s3cfb_resource[] = {
	[0] = {
		.start = S5P6450_PA_LCD,
		.end   = S5P6450_PA_LCD + S5P6450_SZ_LCD - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_DISPCON1,
		.end   = IRQ_DISPCON1,
		.flags = IORESOURCE_IRQ,
	},
	[2] = {
		.start = IRQ_DISPCON0,
		.end   = IRQ_DISPCON0,
		.flags = IORESOURCE_IRQ,
	},
};

static u64 fb_dma_mask = 0xffffffffUL;

struct platform_device s3c_device_fb = {
	.name		  = "s3cfb",
	.id		  = -1,
	.num_resources	  = ARRAY_SIZE(s3cfb_resource),
	.resource	  = s3cfb_resource,
	.dev              = {
		.dma_mask		= &fb_dma_mask,
		.coherent_dma_mask	= 0xffffffffUL
	}
};

static struct s3c_platform_fb default_fb_data __initdata = {
	.hw_ver	= 0x50,
	.clk_name = "lcd",
	.nr_wins = 3,
	.default_win = CONFIG_FB_S3C_V2_DEFAULT_WINDOW,
	.swap = FB_SWAP_WORD | FB_SWAP_HWORD,
};

void __init s3cfb_set_platdata(struct s3c_platform_fb *pd)
{
	struct s3c_platform_fb *npd;
	int i;

	if (!pd)
		pd = &default_fb_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_fb), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);

	for (i = 0; i < npd->nr_wins; i++)
		npd->nr_buffers[i] = 1;

	npd->nr_buffers[npd->default_win] = CONFIG_FB_S3C_V2_YPANSTEP + 1;

	npd->cfg_gpio = s3cfb_cfg_gpio;
	npd->backlight_on = s3cfb_backlight_on;
	npd->lcd_on = s3cfb_lcd_on;

	s3c_device_fb.dev.platform_data = npd;
}

