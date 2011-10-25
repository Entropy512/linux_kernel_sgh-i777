/* arch/arm/plat-s5p/dev-mfc.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Device definition for MFC device
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <mach/map.h>
#include <asm/irq.h>
#include <plat/mfc.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <plat/media.h>

static struct resource s3c_mfc_resources[] = {
	[0] = {
		.start  = S5PV310_PA_MFC,
		.end    = S5PV310_PA_MFC + S5PV310_SZ_MFC - 1,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = IRQ_MFC,
		.end    = IRQ_MFC,
		.flags  = IORESOURCE_IRQ,
	}
};

struct platform_device s3c_device_mfc = {
	.name           = "s3c-mfc",
	.id             = -1,
	.num_resources  = ARRAY_SIZE(s3c_mfc_resources),
	.resource       = s3c_mfc_resources,
};

static struct s3c_platform_mfc default_mfc_data __initdata = {
	.buf_phy_base[0] = 0,
	.buf_phy_base[1] = 0,
	.buf_phy_size[0] = 0,
	.buf_phy_size[1] = 0,
};

void __init s3c_mfc_set_platdata(struct s3c_platform_mfc *pd)
{
	struct s3c_platform_mfc *npd;

	if (!pd)
		pd = &default_mfc_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_mfc), GFP_KERNEL);
	if (!npd) {
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);
		npd->buf_phy_base[0] = s5p_get_media_memory_bank(S3C_MDEV_MFC, 0);
	} else {
#if (defined(CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC) || defined(CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC0))
		npd->buf_phy_base[0] = s5p_get_media_memory_bank(S3C_MDEV_MFC, 0);
		npd->buf_phy_size[0] = s5p_get_media_memory_bank(S3C_MDEV_MFC, 0);
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC1
		npd->buf_phy_base[1] = s5p_get_media_memory_bank(S3C_MDEV_MFC, 1);
		npd->buf_phy_size[1] = s5p_get_media_memory_bank(S3C_MDEV_MFC, 1);
#endif
		s3c_device_mfc.dev.platform_data = npd;
	}
}

