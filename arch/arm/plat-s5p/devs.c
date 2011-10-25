/* linux/arch/arm/plat-s5p/devs.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Base S5P platform device definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/host_notify.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>
#include <mach/hardware.h>
#include <mach/map.h>
#include <mach/dma.h>

#include <plat/devs.h>
#include <plat/gpio-cfg.h>
#include <plat/irqs.h>
#include <plat/fb.h>
#include <plat/fimc.h>
#include <plat/csis.h>
#include <plat/fimg2d.h>
#include <plat/tvout.h>
#include <plat/s5p-otghost.h>

#if defined(CONFIG_VIDEO_MFC5X) || defined(CONFIG_VIDEO_MFC50)
static struct resource s5p_mfc_resources[] = {
	[0] = {
		.start	= S5P_PA_MFC,
		.end	= S5P_PA_MFC + S5P_SZ_MFC - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_MFC,
		.end	= IRQ_MFC,
		.flags	= IORESOURCE_IRQ,
	}
};

struct platform_device s5p_device_mfc = {
	.name		= "s3c-mfc",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s5p_mfc_resources),
	.resource	= s5p_mfc_resources,
};
#endif

/* NAND Controller */
static struct resource s3c_nand_resource[] = {
	[0] = {
		.start = S5P_PA_NAND,
		.end   = S5P_PA_NAND + S5P_SZ_NAND - 1,
		.flags = IORESOURCE_MEM,
	}
};

struct platform_device s3c_device_nand = {
	.name		= "nand",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s3c_nand_resource),
	.resource	= s3c_nand_resource,
};

EXPORT_SYMBOL(s3c_device_nand);

/* OneNAND Controller */
static struct resource s5p_onenand_resource[] = {
	[0] = {
		.start = S5P_PA_ONENAND,
		.end   = S5P_PA_ONENAND + S5P_SZ_ONENAND - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = S5P_PA_ONENAND_CTL,
		.end   = S5P_PA_ONENAND_CTL + S5P_SZ_ONENAND_CTL - 1,
		.flags = IORESOURCE_MEM,
	}
};

struct platform_device s5p_device_onenand = {
	.name		= "s5p-onenand",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s5p_onenand_resource),
	.resource	= s5p_onenand_resource,
};

EXPORT_SYMBOL(s5p_device_onenand);

#ifdef CONFIG_FB_S3C
static struct resource s3cfb_resource[] = {
	[0] = {
		.start	= S5P_PA_LCD,
		.end	= S5P_PA_LCD + S5P_SZ_LCD - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_LCD1,
		.end	= IRQ_LCD1,
		.flags	= IORESOURCE_IRQ,
	},
	[2] = {
		.start	= IRQ_LCD0,
		.end	= IRQ_LCD0,
		.flags	= IORESOURCE_IRQ,
	},
};

static u64 fb_dma_mask = 0xffffffffUL;

struct platform_device s3c_device_fb = {
	.name		= "s3cfb",
#if defined(CONFIG_ARCH_S5PV310)
	.id		= 0,
#else
	.id		= -1,
#endif
	.num_resources	= ARRAY_SIZE(s3cfb_resource),
	.resource	= s3cfb_resource,
	.dev		= {
		.dma_mask		= &fb_dma_mask,
		.coherent_dma_mask	= 0xffffffffUL
	}
};

static struct s3c_platform_fb default_fb_data __initdata = {
#if defined(CONFIG_ARCH_S5PV310)
	.hw_ver = 0x70,
#else
	.hw_ver	= 0x62,
#endif
	.nr_wins	= 5,
#if defined(CONFIG_FB_S3C_DEFAULT_WINDOW)
	.default_win	= CONFIG_FB_S3C_DEFAULT_WINDOW,
#else
	.default_win	= 0,
#endif
	.swap		= FB_SWAP_WORD | FB_SWAP_HWORD,
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
	else {
		for (i = 0; i < npd->nr_wins; i++)
			npd->nr_buffers[i] = 1;

#if defined(CONFIG_FB_S3C_NR_BUFFERS)
		npd->nr_buffers[npd->default_win] = CONFIG_FB_S3C_NR_BUFFERS;
#else
		npd->nr_buffers[npd->default_win] = 1;
#endif

		s3cfb_get_clk_name(npd->clk_name);
		npd->cfg_gpio = s3cfb_cfg_gpio;
#ifndef CONFIG_FB_S3C_MIPI_LCD
		npd->cfg_gpio_sleep = s3cfb_cfg_gpio_sleep;
#endif
		npd->backlight_on = s3cfb_backlight_on;
		npd->backlight_off = s3cfb_backlight_off;
		npd->lcd_on = s3cfb_lcd_on;
		npd->lcd_off = s3cfb_lcd_off;
		npd->clk_on = s3cfb_clk_on;
		npd->clk_off = s3cfb_clk_off;

		s3c_device_fb.dev.platform_data = npd;
	}
}
#endif

#ifdef CONFIG_VIDEO_FIMC
static struct resource s3c_fimc0_resource[] = {
	[0] = {
		.start	= S5P_PA_FIMC0,
		.end	= S5P_PA_FIMC0 + S5P_SZ_FIMC0 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_FIMC0,
		.end	= IRQ_FIMC0,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device s3c_device_fimc0 = {
	.name		= "s3c-fimc",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(s3c_fimc0_resource),
	.resource	= s3c_fimc0_resource,
};

static struct s3c_platform_fimc default_fimc0_data __initdata = {
	.default_cam	= CAMERA_PAR_A,
#if defined(CONFIG_CPU_S5PV210_EVT1)
	.hw_ver	= 0x45,
#elif defined(CONFIG_CPU_S5P6450)
	.hw_ver = 0x50,
#elif defined(CONFIG_CPU_S5PV310)
	.hw_ver = 0x51,
#else
	.hw_ver	= 0x43,
#endif
};

void __init s3c_fimc0_set_platdata(struct s3c_platform_fimc *pd)
{
	struct s3c_platform_fimc *npd;

	if (!pd)
		pd = &default_fimc0_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_fimc), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);
	else {
		if (!npd->cfg_gpio)
			npd->cfg_gpio = s3c_fimc0_cfg_gpio;

		if (!npd->clk_on)
			npd->clk_on = s3c_fimc_clk_on;

		if (!npd->clk_off)
			npd->clk_off = s3c_fimc_clk_off;
#if defined(CONFIG_CPU_S5PV210_EVT1)
		npd->hw_ver = 0x45;
#elif defined(CONFIG_CPU_S5P6450)
		npd->hw_ver = 0x50;
#elif defined(CONFIG_CPU_S5PV310)
		npd->hw_ver = 0x51;
#else
		npd->hw_ver = 0x43;
#endif

		s3c_device_fimc0.dev.platform_data = npd;
	}
}

#if !defined(CONFIG_CPU_S5P6450)
static struct resource s3c_fimc1_resource[] = {
	[0] = {
		.start	= S5P_PA_FIMC1,
		.end	= S5P_PA_FIMC1 + S5P_SZ_FIMC1 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_FIMC1,
		.end	= IRQ_FIMC1,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device s3c_device_fimc1 = {
	.name		= "s3c-fimc",
	.id		= 1,
	.num_resources	= ARRAY_SIZE(s3c_fimc1_resource),
	.resource	= s3c_fimc1_resource,
};

static struct s3c_platform_fimc default_fimc1_data __initdata = {
	.default_cam	= CAMERA_PAR_A,
#if defined(CONFIG_CPU_S5PV210_EVT1)
	.hw_ver	= 0x50,
#elif defined(CONFIG_CPU_S5PV310)
	.hw_ver = 0x51,
#else
	.hw_ver	= 0x43,
#endif
};

void __init s3c_fimc1_set_platdata(struct s3c_platform_fimc *pd)
{
	struct s3c_platform_fimc *npd;

	if (!pd)
		pd = &default_fimc1_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_fimc), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);
	else {
		if (!npd->cfg_gpio)
			npd->cfg_gpio = s3c_fimc1_cfg_gpio;

		if (!npd->clk_on)
			npd->clk_on = s3c_fimc_clk_on;

		if (!npd->clk_off)
			npd->clk_off = s3c_fimc_clk_off;
#if defined(CONFIG_CPU_S5PV210_EVT1)
		npd->hw_ver = 0x50;
#elif defined(CONFIG_CPU_S5PV310)
		npd->hw_ver = 0x51;
#else
		npd->hw_ver = 0x43;
#endif

		s3c_device_fimc1.dev.platform_data = npd;
	}
}

static struct resource s3c_fimc2_resource[] = {
	[0] = {
		.start	= S5P_PA_FIMC2,
		.end	= S5P_PA_FIMC2 + S5P_SZ_FIMC2 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_FIMC2,
		.end	= IRQ_FIMC2,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device s3c_device_fimc2 = {
	.name		= "s3c-fimc",
	.id		= 2,
	.num_resources	= ARRAY_SIZE(s3c_fimc2_resource),
	.resource	= s3c_fimc2_resource,
};

static struct s3c_platform_fimc default_fimc2_data __initdata = {
	.default_cam	= CAMERA_PAR_A,
#if defined(CONFIG_CPU_S5PV210_EVT1)
	.hw_ver	= 0x45,
#elif defined(CONFIG_CPU_S5PV310)
	.hw_ver	= 0x51,
#else
	.hw_ver	= 0x43,
#endif
};

void __init s3c_fimc2_set_platdata(struct s3c_platform_fimc *pd)
{
	struct s3c_platform_fimc *npd;

	if (!pd)
		pd = &default_fimc2_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_fimc), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);
	else {
		if (!npd->cfg_gpio)
			npd->cfg_gpio = s3c_fimc2_cfg_gpio;

		if (!npd->clk_on)
			npd->clk_on = s3c_fimc_clk_on;

		if (!npd->clk_off)
			npd->clk_off = s3c_fimc_clk_off;
#if defined(CONFIG_CPU_S5PV210_EVT1)
		npd->hw_ver = 0x45;
#elif defined(CONFIG_CPU_S5PV310)
		npd->hw_ver = 0x51;
#else
		npd->hw_ver = 0x43;
#endif

		s3c_device_fimc2.dev.platform_data = npd;
	}
}

#if defined(CONFIG_CPU_S5PV310)
static struct resource s3c_fimc3_resource[] = {
	[0] = {
		.start	= S5P_PA_FIMC3,
		.end	= S5P_PA_FIMC3 + S5P_SZ_FIMC3 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_FIMC3,
		.end	= IRQ_FIMC3,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device s3c_device_fimc3 = {
	.name		= "s3c-fimc",
	.id		= 3,
	.num_resources	= ARRAY_SIZE(s3c_fimc3_resource),
	.resource	= s3c_fimc3_resource,
};

static struct s3c_platform_fimc default_fimc3_data __initdata = {
	.default_cam	= CAMERA_PAR_A,
	.hw_ver	= 0x51,
};

void __init s3c_fimc3_set_platdata(struct s3c_platform_fimc *pd)
{
	struct s3c_platform_fimc *npd;

	if (!pd)
		pd = &default_fimc3_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_fimc), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);
	else {
		if (!npd->cfg_gpio)
			npd->cfg_gpio = s3c_fimc3_cfg_gpio;

		if (!npd->clk_on)
			npd->clk_on = s3c_fimc_clk_on;

		if (!npd->clk_off)
			npd->clk_off = s3c_fimc_clk_off;
		npd->hw_ver = 0x51;

		s3c_device_fimc3.dev.platform_data = npd;
	}
}
#endif /* CONFIG_ARCH_S5PV310 */
#endif /* CONFIG_CPU_S5P6450 */

#ifndef CONFIG_ARCH_S5PV310
static struct resource s3c_ipc_resource[] = {
	[0] = {
		.start	= S5P_PA_IPC,
		.end	= S5P_PA_IPC + S5P_SZ_IPC - 1,
		.flags	= IORESOURCE_MEM,
	},
};

struct platform_device s3c_device_ipc = {
	.name		= "s3c-ipc",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s3c_ipc_resource),
	.resource	= s3c_ipc_resource,
};
#endif

#ifdef CONFIG_VIDEO_FIMC_MIPI
#if defined(CONFIG_CPU_S5PV310)
static struct resource s3c_csis0_resource[] = {
	[0] = {
		.start	= S5P_PA_CSIS0,
		.end	= S5P_PA_CSIS0 + S5P_SZ_CSIS0 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_MIPICSI0,
		.end	= IRQ_MIPICSI0,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device s3c_device_csis0 = {
	.name		= "s3c-csis",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(s3c_csis0_resource),
	.resource	= s3c_csis0_resource,
};

static struct s3c_platform_csis default_csis0_data __initdata = {
	.srclk_name	= "mout_mpll",
	.clk_name	= "sclk_csis",
	.clk_rate	= 166000000,
};

void __init s3c_csis0_set_platdata(struct s3c_platform_csis *pd)
{
	struct s3c_platform_csis *npd;

	if (!pd)
		pd = &default_csis0_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_csis), GFP_KERNEL);
	if (!npd) {
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);
		return;
	}

	npd->cfg_gpio = s3c_csis0_cfg_gpio;
	npd->cfg_phy_global = s3c_csis0_cfg_phy_global;
	npd->clk_on = s3c_csis_clk_on;
	npd->clk_off = s3c_csis_clk_off;
	s3c_device_csis0.dev.platform_data = npd;
}
static struct resource s3c_csis1_resource[] = {
	[0] = {
		.start	= S5P_PA_CSIS1,
		.end	= S5P_PA_CSIS1 + S5P_SZ_CSIS1 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_MIPICSI1,
		.end	= IRQ_MIPICSI1,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device s3c_device_csis1 = {
	.name		= "s3c-csis",
	.id		= 1,
	.num_resources	= ARRAY_SIZE(s3c_csis1_resource),
	.resource	= s3c_csis1_resource,
};

static struct s3c_platform_csis default_csis1_data __initdata = {
	.srclk_name	= "mout_mpll",
	.clk_name	= "sclk_csis",
	.clk_rate	= 166000000,
};

void __init s3c_csis1_set_platdata(struct s3c_platform_csis *pd)
{
	struct s3c_platform_csis *npd;

	if (!pd)
		pd = &default_csis1_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_csis), GFP_KERNEL);
	if (!npd) {
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);
		return;
	}

	npd->cfg_gpio = s3c_csis1_cfg_gpio;
	npd->cfg_phy_global = s3c_csis1_cfg_phy_global;
	npd->clk_on = s3c_csis_clk_on;
	npd->clk_off = s3c_csis_clk_off;
	s3c_device_csis1.dev.platform_data = npd;
}
#else
static struct resource s3c_csis_resource[] = {
	[0] = {
		.start	= S5P_PA_CSIS,
		.end	= S5P_PA_CSIS + S5P_SZ_CSIS - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_MIPICSI,
		.end	= IRQ_MIPICSI,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device s3c_device_csis = {
	.name		= "s3c-csis",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(s3c_csis_resource),
	.resource	= s3c_csis_resource,
};

static struct s3c_platform_csis default_csis_data __initdata = {
	.srclk_name	= "mout_mpll",
	.clk_name	= "sclk_csis",
	.clk_rate	= 166000000,
};

void __init s3c_csis_set_platdata(struct s3c_platform_csis *pd)
{
	struct s3c_platform_csis *npd;

	if (!pd)
		pd = &default_csis_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_csis), GFP_KERNEL);
	if (!npd) {
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);
		return;
	}

	npd->cfg_gpio = s3c_csis_cfg_gpio;
	npd->cfg_phy_global = s3c_csis_cfg_phy_global;
	npd->clk_on = s3c_csis_clk_on;
	npd->clk_off = s3c_csis_clk_off;

	s3c_device_csis.dev.platform_data = npd;
}
#endif /* CONFIG_ARCH_S5PV310 */
#endif /* CONFIG_VIDEO_FIMC_MIPI */
#endif /* CONFIG_VIDEO_FIMC */

#ifdef CONFIG_VIDEO_JPEG
/* JPEG controller  */
static struct resource s5p_jpeg_resource[] = {
	[0] = {
		.start = S5P_PA_JPEG,
		.end   = S5P_PA_JPEG + S5P_SZ_JPEG - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_JPEG,
		.end   = IRQ_JPEG,
		.flags = IORESOURCE_IRQ,
	}
};

struct platform_device s5p_device_jpeg = {
	.name             = "s5p-jpeg",
	.id               = -1,
	.num_resources    = ARRAY_SIZE(s5p_jpeg_resource),
	.resource         = s5p_jpeg_resource,
};
EXPORT_SYMBOL(s5p_device_jpeg);
#endif /* CONFIG_VIDEO_JPEG */

#ifdef CONFIG_VIDEO_ROTATOR
/* rotator interface */
static struct resource s5p_rotator_resource[] = {
	[0] = {
		.start = S5P_PA_ROTATOR,
		.end   = S5P_PA_ROTATOR + S5P_SZ_ROTATOR - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_ROTATOR,
		.end   = IRQ_ROTATOR,
		.flags = IORESOURCE_IRQ,
	}
};

struct platform_device s5p_device_rotator = {
	.name		= "s5p-rotator",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s5p_rotator_resource),
	.resource	= s5p_rotator_resource
};
EXPORT_SYMBOL(s5p_device_rotator);
#endif

#ifdef CONFIG_KEYPAD_S3C
/* Keypad interface */
static struct resource s3c_keypad_resource[] = {
        [0] = {
                .start = S3C_PA_KEYPAD,
                .end   = S3C_PA_KEYPAD+ S3C_SZ_KEYPAD - 1,
                .flags = IORESOURCE_MEM,
        },
        [1] = {
                .start = IRQ_KEYPAD,
                .end   = IRQ_KEYPAD,
                .flags = IORESOURCE_IRQ,
        }
};

struct platform_device s3c_device_keypad = {
        .name             = "s3c-keypad",
        .id               = -1,
        .num_resources    = ARRAY_SIZE(s3c_keypad_resource),
        .resource         = s3c_keypad_resource,
};

EXPORT_SYMBOL(s3c_device_keypad);
#endif

#if defined(CONFIG_VIDEO_TVOUT)
/* TVOUT interface */
static struct resource s5p_tvout_resources[] = {
	[0] = {
		.start  = S5P_PA_TVENC,
		.end    = S5P_PA_TVENC + S5P_SZ_TVENC - 1,
		.flags  = IORESOURCE_MEM,
		.name	= "s5p-sdo"
	},
	[1] = {
		.start  = S5P_PA_VP,
		.end    = S5P_PA_VP + S5P_SZ_VP - 1,
		.flags  = IORESOURCE_MEM,
		.name	= "s5p-vp"
	},
	[2] = {
		.start  = S5P_PA_MIXER,
		.end    = S5P_PA_MIXER + S5P_SZ_MIXER - 1,
		.flags  = IORESOURCE_MEM,
		.name	= "s5p-mixer"
	},
	[3] = {
		.start  = S5P_PA_HDMI,
		.end    = S5P_PA_HDMI + S5P_SZ_HDMI - 1,
		.flags  = IORESOURCE_MEM,
		.name	= "s5p-hdmi"
	},
	[4] = {
		.start  = S5P_I2C_HDMI_PHY,
		.end    = S5P_I2C_HDMI_PHY + S5P_I2C_HDMI_SZ_PHY - 1,
		.flags  = IORESOURCE_MEM,
		.name	= "s5p-i2c-hdmi-phy"
	},
	[5] = {
		.start  = IRQ_MIXER,
		.end    = IRQ_MIXER,
		.flags  = IORESOURCE_IRQ,
		.name	= "s5p-mixer"
	},
	[6] = {
		.start  = IRQ_HDMI,
		.end    = IRQ_HDMI,
		.flags  = IORESOURCE_IRQ,
		.name	= "s5p-hdmi"
	},
	[7] = {
		.start  = IRQ_TVENC,
		.end    = IRQ_TVENC,
		.flags  = IORESOURCE_IRQ,
		.name	= "s5p-sdo"
	},
};

struct platform_device s5p_device_tvout = {
	.name           = "s5p-tvout",
	.id             = -1,
	.num_resources  = ARRAY_SIZE(s5p_tvout_resources),
	.resource       = s5p_tvout_resources,
};
EXPORT_SYMBOL(s5p_device_tvout);

/* CEC */
static struct resource s5p_cec_resources[] = {
	[0] = {
		.start  = S5P_PA_CEC,
		.end    = S5P_PA_CEC + S5P_SZ_CEC - 1,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = IRQ_CEC,
		.end    = IRQ_CEC,
		.flags  = IORESOURCE_IRQ,
	},
};

struct platform_device s5p_device_cec = {
	.name           = "s5p-tvout-cec",
	.id             = -1,
	.num_resources  = ARRAY_SIZE(s5p_cec_resources),
	.resource       = s5p_cec_resources,
};
EXPORT_SYMBOL(s5p_device_cec);

/* HPD */
static struct resource s5p_hpd_resources[] = {
	[0] = {
		.start  = IRQ_TVOUT_HPD,
		.end    = IRQ_TVOUT_HPD,
		.flags  = IORESOURCE_IRQ,
	},
};

struct platform_device s5p_device_hpd = {
	.name           = "s5p-tvout-hpd",
	.id             = -1,
	.num_resources  = ARRAY_SIZE(s5p_hpd_resources),
	.resource       = s5p_hpd_resources,
};
EXPORT_SYMBOL(s5p_device_hpd);

void __init s5p_hdmi_hpd_set_platdata(struct s5p_platform_hpd *pd)
{
	struct s5p_platform_hpd *npd;

	npd = kmemdup(pd, sizeof(struct s5p_platform_hpd), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);
	else {
		if (!npd->int_src_ext_hpd)
			npd->int_src_ext_hpd = s5p_int_src_ext_hpd;
		if (!npd->int_src_hdmi_hpd)
			npd->int_src_hdmi_hpd = s5p_int_src_hdmi_hpd;
		if (!npd->read_gpio)
			npd->read_gpio = s5p_hpd_read_gpio;

		s5p_device_hpd.dev.platform_data = npd;
	}
}

void __init s5p_hdmi_cec_set_platdata(struct s5p_platform_cec *pd)
{
	struct s5p_platform_cec *npd;

	npd = kmemdup(pd, sizeof(struct s5p_platform_cec), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);
	else {
		if (!npd->cfg_gpio)
			npd->cfg_gpio = s5p_cec_cfg_gpio;

		s5p_device_cec.dev.platform_data = npd;
	}
}
#endif

#ifdef CONFIG_USB_SUPPORT
#ifdef CONFIG_USB_ARCH_HAS_EHCI
 /* USB Host Controlle EHCI registrations */
static struct resource s3c_usb__ehci_resource[] = {
	[0] = {
		.start = S5P_PA_USB_EHCI,
		.end   = S5P_PA_USB_EHCI  + S5P_SZ_USB_EHCI - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_UHOST,
		.end   = IRQ_UHOST,
		.flags = IORESOURCE_IRQ,
	}
};

static u64 s3c_device_usb_ehci_dmamask = 0xffffffffUL;

struct platform_device s3c_device_usb_ehci = {
	.name		= "s5p-ehci",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s3c_usb__ehci_resource),
	.resource	= s3c_usb__ehci_resource,
	.dev		= {
		.dma_mask = &s3c_device_usb_ehci_dmamask,
		.coherent_dma_mask = 0xffffffffUL
	}
};
EXPORT_SYMBOL(s3c_device_usb_ehci);
#endif /* CONFIG_USB_ARCH_HAS_EHCI */

#ifdef CONFIG_USB_ARCH_HAS_OHCI
/* USB Host Controlle OHCI registrations */
static struct resource s3c_usb__ohci_resource[] = {
	[0] = {
		.start = S5P_PA_USB_OHCI,
		.end   = S5P_PA_USB_OHCI  + S5P_SZ_USB_OHCI - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_UHOST,
		.end   = IRQ_UHOST,
		.flags = IORESOURCE_IRQ,
	}
};

static u64 s3c_device_usb_ohci_dmamask = 0xffffffffUL;

struct platform_device s3c_device_usb_ohci = {
	.name		= "s5p-ohci",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s3c_usb__ohci_resource),
	.resource	= s3c_usb__ohci_resource,
	.dev		= {
		.dma_mask = &s3c_device_usb_ohci_dmamask,
		.coherent_dma_mask = 0xffffffffUL
	}
};
EXPORT_SYMBOL(s3c_device_usb_ohci);
#endif /* CONFIG_USB_ARCH_HAS_EHCI */

/* we don't use OTGDRVVBUS pin for powering up otg charging pump */
static void otg_power_cb(int enable)
{
#ifdef GPIO_USB_OTG_EN
	u8 on = (u8)!!enable;
	gpio_request(GPIO_USB_OTG_EN, "USB_OTG_EN");
	gpio_direction_output(GPIO_USB_OTG_EN, on);
	gpio_free(GPIO_USB_OTG_EN);
#endif
	pr_info("%s: otg power = %d\n", __func__, on);
}

static struct host_notify_dev otg_ndev = {
	.name = "usb_otg",
	.set_booster = otg_power_cb,
};

static struct sec_otghost_data otghost_data = {
	.clk_usage = 0,
	.set_pwr_cb = otg_power_cb,
	.host_notify = 1,
	.sec_whlist_table_num = 1,
};

/* USB Device (OTG hcd)*/
static struct resource s3c_usb_otghcd_resource[] = {
    [0] = {
        .start = S5P_PA_OTG,
        .end   = S5P_PA_OTG + S5P_SZ_OTG - 1,
        .flags = IORESOURCE_MEM,
    },
    [1] = {
        .start = IRQ_OTG,
        .end   = IRQ_OTG,
        .flags = IORESOURCE_IRQ,
    }
};

static u64 s3c_device_usb_otghcd_dmamask = 0xffffffffUL;

struct platform_device s3c_device_usb_otghcd = {
	.name		=	"s3c_otghcd",
	.id		=	-1,
	.num_resources	=	ARRAY_SIZE(s3c_usb_otghcd_resource),
	.resource	=	s3c_usb_otghcd_resource,
	.dev = {
		.platform_data	=	&otghost_data,
		.dma_mask	=	&s3c_device_usb_otghcd_dmamask,
		.coherent_dma_mask	=	0xffffffffUL
	}
};
EXPORT_SYMBOL(s3c_device_usb_otghcd);

/* Android USB OTG Gadget */
#include <linux/usb/android_composite.h>
#define S3C_VENDOR_ID		0x18d1
#define S3C_PRODUCT_ID		0x0001
#define S3C_ADB_PRODUCT_ID	0x0005
#define MAX_USB_SERIAL_NUM	17

static char *usb_functions_ums[] = {
	"usb_mass_storage",
};
#ifdef CONFIG_USB_ANDROID_RNDIS
static char *usb_functions_rndis[] = {
	"rndis",
};
#endif
#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
/* Do not use below compoiste */
#else
static char *usb_functions_ums_adb[] = {
	"usb_mass_storage",
	"adb",
};
#endif

#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
#  ifdef CONFIG_USB_ANDROID_SAMSUNG_ESCAPE /* USE DEVGURU HOST DRIVER */
/* kies mode : using MS Composite*/
#    ifdef CONFIG_USB_ANDROID_SAMSUNG_KIES_UMS
static char *usb_functions_ums_acm[] = {
	"usb_mass_storage",
	"acm",
};
#    else
static char *usb_functions_mtp_acm[] = {
	"mtp",
	"acm",
};
#    endif
/* debug mode : using MS Composite*/
static char *usb_functions_ums_acm_adb[] = {
	"usb_mass_storage",
	"acm",
	"adb",
};
#  else /* USE MCCI HOST DRIVER */
/* kies mode */
static char *usb_functions_acm_mtp[] = {
	"acm",
	"mtp",
};
/* debug mode */
static char *usb_functions_acm_ums_adb[] = {
	"acm",
	"usb_mass_storage",
	"adb",
};
#  endif
/* mtp only mode */
static char *usb_functions_mtp[] = {
	"mtp",
};
#endif

static char *usb_functions_all[] = {
#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
#  ifdef CONFIG_USB_ANDROID_SAMSUNG_ESCAPE /* USE DEVGURU HOST DRIVER */
	"usb_mass_storage",
	"acm",
	"adb",
#    ifdef CONFIG_USB_ANDROID_RNDIS
	"rndis",
#    endif
#    ifndef CONFIG_USB_ANDROID_SAMSUNG_KIES_UMS
	"mtp",
#    endif
#  else /* USE MCCI HOST DRIVER */
	"acm",
	"usb_mass_storage",
	"adb",
#    ifdef CONFIG_USB_ANDROID_RNDIS
	"rndis",
#    endif
	"mtp",
#  endif
#else /* original */
#  ifdef CONFIG_USB_ANDROID_RNDIS
	"rndis",
#  endif
	"usb_mass_storage",
	"adb",
#  ifdef CONFIG_USB_ANDROID_ACM
	"acm",
#  endif
#endif /* CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE */
};


static struct android_usb_product usb_products[] = {
#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
#  ifdef CONFIG_USB_ANDROID_SAMSUNG_ESCAPE /* USE DEVGURU HOST DRIVER */
#    ifdef CONFIG_USB_ANDROID_SAMSUNG_KIES_UMS
	{
		.product_id	= SAMSUNG_DEBUG_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_ums_acm_adb),
		.functions	= usb_functions_ums_acm_adb,
		.bDeviceClass	= 0xEF,
		.bDeviceSubClass= 0x02,
		.bDeviceProtocol= 0x01,
		.s		= ANDROID_DEBUG_CONFIG_STRING,
		.mode		= USBSTATUS_ADB,
	},
	{
		.product_id	= SAMSUNG_KIES_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_ums_acm),
		.functions	= usb_functions_ums_acm,
		.bDeviceClass	= 0xEF,
		.bDeviceSubClass= 0x02,
		.bDeviceProtocol= 0x01,
		.s		= ANDROID_KIES_CONFIG_STRING,
		.mode		= USBSTATUS_SAMSUNG_KIES,
	},
	{
		.product_id	= SAMSUNG_UMS_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_ums),
		.functions	= usb_functions_ums,
		.bDeviceClass	= USB_CLASS_PER_INTERFACE,
		.bDeviceSubClass= 0,
		.bDeviceProtocol= 0,
		.s		= ANDROID_UMS_CONFIG_STRING,
		.mode		= USBSTATUS_UMS,
	},
#      ifdef CONFIG_USB_ANDROID_RNDIS
	{
		.product_id	= SAMSUNG_RNDIS_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_rndis),
		.functions	= usb_functions_rndis,
#        ifdef CONFIG_USB_ANDROID_SAMSUNG_RNDIS_WITH_MS_COMPOSITE
		.bDeviceClass	= 0xEF,
		.bDeviceSubClass= 0x02,
		.bDeviceProtocol= 0x01,
#        else
#          ifdef CONFIG_USB_ANDROID_RNDIS_WCEIS
		.bDeviceClass	= USB_CLASS_WIRELESS_CONTROLLER,
#          else
		.bDeviceClass	= USB_CLASS_COMM,
#          endif
		.bDeviceSubClass= 0,
		.bDeviceProtocol= 0,
#        endif
		.s		= ANDROID_RNDIS_CONFIG_STRING,
		.mode		= USBSTATUS_VTP,
	},
#      endif
#    else /* Not used KIES_UMS */
	{
		.product_id	= SAMSUNG_DEBUG_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_ums_acm_adb),
		.functions	= usb_functions_ums_acm_adb,
		.bDeviceClass	= 0xEF,
		.bDeviceSubClass= 0x02,
		.bDeviceProtocol= 0x01,
		.s		= ANDROID_DEBUG_CONFIG_STRING,
		.mode		= USBSTATUS_ADB,
	},
	{
		.product_id	= SAMSUNG_KIES_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_mtp_acm),
		.functions	= usb_functions_mtp_acm,
		.bDeviceClass	= 0xEF,
		.bDeviceSubClass= 0x02,
		.bDeviceProtocol= 0x01,
		.s		= ANDROID_KIES_CONFIG_STRING,
		.mode		= USBSTATUS_SAMSUNG_KIES,
		.multi_conf_functions[0] = usb_functions_mtp,
		.multi_conf_functions[1] = usb_functions_mtp_acm,
	},
	{
		.product_id	= SAMSUNG_UMS_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_ums),
		.functions	= usb_functions_ums,
		.bDeviceClass	= USB_CLASS_PER_INTERFACE,
		.bDeviceSubClass= 0,
		.bDeviceProtocol= 0,
		.s		= ANDROID_UMS_CONFIG_STRING,
		.mode		= USBSTATUS_UMS,
	},
#      ifdef CONFIG_USB_ANDROID_RNDIS
	{
		.product_id	= SAMSUNG_RNDIS_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_rndis),
		.functions	= usb_functions_rndis,
#        ifdef CONFIG_USB_ANDROID_SAMSUNG_RNDIS_WITH_MS_COMPOSITE
		.bDeviceClass	= 0xEF,
		.bDeviceSubClass= 0x02,
		.bDeviceProtocol= 0x01,
#        else
#          ifdef CONFIG_USB_ANDROID_RNDIS_WCEIS
		.bDeviceClass	= USB_CLASS_WIRELESS_CONTROLLER,
#          else
		.bDeviceClass	= USB_CLASS_COMM,
#          endif
		.bDeviceSubClass= 0,
		.bDeviceProtocol= 0,
#        endif
		.s		= ANDROID_RNDIS_CONFIG_STRING,
		.mode		= USBSTATUS_VTP,
	},
#      endif
	{
		.product_id	= SAMSUNG_MTP_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_mtp),
		.functions	= usb_functions_mtp,
		.bDeviceClass	= USB_CLASS_PER_INTERFACE,
		.bDeviceSubClass= 0,
		.bDeviceProtocol= 0x01,
		.s		= ANDROID_MTP_CONFIG_STRING,
		.mode		= USBSTATUS_MTPONLY,
	},
#    endif
#  else  /* USE MCCI HOST DRIVER */
	{
		.product_id	= SAMSUNG_DEBUG_PRODUCT_ID, /* change sequence */
		.num_functions	= ARRAY_SIZE(usb_functions_acm_ums_adb),
		.functions	= usb_functions_acm_ums_adb,
		.bDeviceClass	= USB_CLASS_COMM,
		.bDeviceSubClass= 0,
		.bDeviceProtocol= 0,
		.s		= ANDROID_DEBUG_CONFIG_STRING,
		.mode		= USBSTATUS_ADB,
	},
	{
		.product_id	= SAMSUNG_KIES_PRODUCT_ID, /* change sequence */
		.num_functions	= ARRAY_SIZE(usb_functions_acm_mtp),
		.functions	= usb_functions_acm_mtp,
		.bDeviceClass	= USB_CLASS_COMM,
		.bDeviceSubClass= 0,
		.bDeviceProtocol= 0,
		.s		= ANDROID_KIES_CONFIG_STRING,
		.mode		= USBSTATUS_SAMSUNG_KIES,

	},
	{
		.product_id	= SAMSUNG_UMS_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_ums),
		.functions	= usb_functions_ums,
		.bDeviceClass	= USB_CLASS_PER_INTERFACE,
		.bDeviceSubClass= 0,
		.bDeviceProtocol= 0,
		.s		= ANDROID_UMS_CONFIG_STRING,
		.mode		= USBSTATUS_UMS,
	},
#    ifdef CONFIG_USB_ANDROID_RNDIS
	{
		.product_id	= SAMSUNG_RNDIS_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_rndis),
		.functions	= usb_functions_rndis,
#      ifdef CONFIG_USB_ANDROID_RNDIS_WCEIS
		.bDeviceClass	= USB_CLASS_WIRELESS_CONTROLLER,
#      else
		.bDeviceClass	= USB_CLASS_COMM,
#      endif
		.bDeviceSubClass= 0,
		.bDeviceProtocol= 0,
		.s		= ANDROID_RNDIS_CONFIG_STRING,
		.mode		= USBSTATUS_VTP,
	},
#    endif
	{
		.product_id	= SAMSUNG_MTP_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_mtp),
		.functions	= usb_functions_mtp,
		.bDeviceClass	= USB_CLASS_PER_INTERFACE,
		.bDeviceSubClass= 0,
		.bDeviceProtocol= 0x01,
		.s		= ANDROID_MTP_CONFIG_STRING,
		.mode		= USBSTATUS_MTPONLY,
	},
#  endif
#else /* original */
	{
		.product_id	= S3C_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_ums),
		.functions	= usb_functions_ums,
	},
	{
		.product_id	= S3C_ADB_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_ums_adb),
		.functions	= usb_functions_ums_adb,
	},
#endif
};
// serial number should be changed as real device for commercial release
static char device_serial[MAX_USB_SERIAL_NUM]="0123456789ABCDEF";
/* standard android USB platform data */

// Information should be changed as real product for commercial release
static struct android_usb_platform_data android_usb_pdata = {
#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
	.vendor_id		= SAMSUNG_VENDOR_ID,
	.product_id		= SAMSUNG_KIES_PRODUCT_ID,
	.manufacturer_name	= "SAMSUNG",
	.product_name		= "SAMSUNG_Android",
#else
	.vendor_id		= S3C_VENDOR_ID,
	.product_id		= S3C_PRODUCT_ID,
	.manufacturer_name	= "Android",//"Samsung",
	.product_name		= "Android",//"Samsung SMDKV210",
#endif
	.serial_number		= device_serial,
	.num_products		= ARRAY_SIZE(usb_products),
	.products		= usb_products,
	.num_functions		= ARRAY_SIZE(usb_functions_all),
	.functions		= usb_functions_all,
};

struct platform_device s3c_device_android_usb = {
	.name	= "android_usb",
	.id	= -1,
	.dev	= {
		.platform_data	= &android_usb_pdata,
	},
};
EXPORT_SYMBOL(s3c_device_android_usb);

static struct usb_mass_storage_platform_data ums_pdata = {
#ifdef CONFIG_TARGET_LOCALE_KOR
	.vendor			= "SAMSUNG",
	.product		= "SHW-M250S",
#else
	.vendor			= "Android   ",//"Samsung",
	.product		= "UMS Composite",//"SMDKV210",
#endif
	.release		= 1,
	.nluns			= 1,
};
struct platform_device s3c_device_usb_mass_storage= {
	.name	= "usb_mass_storage",
	.id	= -1,
	.dev	= {
		.platform_data = &ums_pdata,
	},
};
EXPORT_SYMBOL(s3c_device_usb_mass_storage);
#ifdef CONFIG_USB_ANDROID_RNDIS
static struct usb_ether_platform_data rndis_pdata = {
/* ethaddr is filled by board_serialno_setup */
	.vendorID       = S3C_VENDOR_ID,
	.vendorDescr    = "Samsung",
};
struct platform_device s3c_device_rndis= {
	.name   = "rndis",
	.id     = -1,
	.dev    = {
		.platform_data = &rndis_pdata,
	},
};
EXPORT_SYMBOL(s3c_device_rndis);
#endif

/* USB Device (Gadget)*/
static struct resource s3c_usbgadget_resource[] = {
	[0] = {
		.start = S5P_PA_OTG,
		.end   = S5P_PA_OTG+S5P_SZ_OTG-1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_OTG,
		.end   = IRQ_OTG,
		.flags = IORESOURCE_IRQ,
	}
};

struct platform_device s3c_device_usbgadget = {
	.name           = "s3c-usbgadget",
	.id             = -1,
	.num_resources  = ARRAY_SIZE(s3c_usbgadget_resource),
	.resource       = s3c_usbgadget_resource,
	.dev = {
		.platform_data	=	&otg_ndev,
	}
};
EXPORT_SYMBOL(s3c_device_usbgadget);

void __init s3c_usb_otg_composite_pdata(struct s3c_platform_fb *pd)
{
#ifdef CONFIG_USB_ANDROID_RNDIS
	int i;
	char *src;
	/* create a fake MAC address from our serial number.
	 * first byte is 0x02 to signify locally administered.
	 */
	rndis_pdata.ethaddr[0] = 0x02;
	src = device_serial;
	for (i = 0; *src; i++) {
		/* XOR the USB serial across the remaining bytes */
		rndis_pdata.ethaddr[i % (ETH_ALEN - 1) + 1] ^= *src++;
	}
#endif
}

#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
void __init s3c_usb_set_serial(void)
{
#  ifdef CONFIG_USB_ANDROID_RNDIS
	int i;
	char *src;
#  endif
	sprintf(device_serial, "%08X%08X", system_serial_high, system_serial_low);
#  ifdef CONFIG_USB_ANDROID_RNDIS
	/* create a fake MAC address from our serial number.
	 * first byte is 0x02 to signify locally administered.
	 */
	rndis_pdata.ethaddr[0] = 0x02;
	src = device_serial;
	for (i = 0; *src; i++) {
		/* XOR the USB serial across the remaining bytes */
		rndis_pdata.ethaddr[i % (ETH_ALEN - 1) + 1] ^= *src++;
	}
#  endif
}
#endif /* CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE */

#endif

#ifdef CONFIG_VIDEO_TSI
/*TSI Interface*/
static u64 tsi_dma_mask = 0xffffffffUL;

static struct resource s3c_tsi_resource[] = {
        [0] = {
                .start = S5P_PA_TSI,
                .end   = S5P_PA_TSI + S5P_SZ_TSI - 1,
                .flags = IORESOURCE_MEM,
        },
        [1] = {
                .start = IRQ_TSI,
                .end   = IRQ_TSI,
                .flags = IORESOURCE_IRQ,
        }
};

struct platform_device s3c_device_tsi = {
        .name             = "s3c-tsi",
        .id               = -1,
        .num_resources    = ARRAY_SIZE(s3c_tsi_resource),
        .resource         = s3c_tsi_resource,
	.dev              = {
		.dma_mask		= &tsi_dma_mask,
		.coherent_dma_mask	= 0xffffffffUL
	}


};
EXPORT_SYMBOL(s3c_device_tsi);

#endif

#ifdef CONFIG_VIDEO_FIMG2D
static struct resource s5p_fimg2d_resource[] = {
	[0] = {
		.start	= S5P_PA_FIMG2D,
		.end	= S5P_PA_FIMG2D + S5P_SZ_FIMG2D - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_FIMG2D,
		.end	= IRQ_FIMG2D,
		.flags	= IORESOURCE_IRQ,
	}
};

struct platform_device s5p_device_fimg2d = {
	.name		= "s5p-fimg2d",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s5p_fimg2d_resource),
	.resource	= s5p_fimg2d_resource
};
EXPORT_SYMBOL(s5p_device_fimg2d);

static struct fimg2d_platdata default_fimg2d_data __initdata = {
	.parent_clkname = "mout_g2d0",
	.clkname = "sclk_fimg2d",
	.gate_clkname = "fimg2d",
	.clkrate = 250 * 1000000,
};

void __init s5p_fimg2d_set_platdata(struct fimg2d_platdata *pd)
{
	struct fimg2d_platdata *npd;

	if (!pd)
		pd = &default_fimg2d_data;

	npd = kmemdup(pd, sizeof(*pd), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "no memory for fimg2d platform data\n");
	else
		s5p_device_fimg2d.dev.platform_data = npd;
}
#endif

#ifdef CONFIG_S5P_SYSMMU
/* SYSMMU Device */
static struct resource s5p_sysmmu_resource[] = {
	[0] = {
		.start	= S5P_PA_SYSMMU_MDMA,
		.end	= S5P_PA_SYSMMU_MDMA + S5P_SZ_SYSMMU - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_SYSMMU_MDMA0_0,
		.end	= IRQ_SYSMMU_MDMA0_0,
		.flags	= IORESOURCE_IRQ,
	},
	[2] = {
		.start	= S5P_PA_SYSMMU_SSS,
		.end	= S5P_PA_SYSMMU_SSS + S5P_SZ_SYSMMU - 1,
		.flags	= IORESOURCE_MEM,
	},
	[3] = {
		.start	= IRQ_SYSMMU_SSS_0,
		.end	= IRQ_SYSMMU_SSS_0,
		.flags	= IORESOURCE_IRQ,
	},
	[4] = {
		.start	= S5P_PA_SYSMMU_FIMC0,
		.end	= S5P_PA_SYSMMU_FIMC0 + S5P_SZ_SYSMMU - 1,
		.flags	= IORESOURCE_MEM,
	},
	[5] = {
		.start	= IRQ_SYSMMU_FIMC0_0,
		.end	= IRQ_SYSMMU_FIMC0_0,
		.flags	= IORESOURCE_IRQ,
	},
	[6] = {
		.start	= S5P_PA_SYSMMU_FIMC1,
		.end	= S5P_PA_SYSMMU_FIMC1 + S5P_SZ_SYSMMU - 1,
		.flags	= IORESOURCE_MEM,
	},
	[7] = {
		.start	= IRQ_SYSMMU_FIMC1_0,
		.end	= IRQ_SYSMMU_FIMC1_0,
		.flags	= IORESOURCE_IRQ,
	},
	[8] = {
		.start	= S5P_PA_SYSMMU_FIMC2,
		.end	= S5P_PA_SYSMMU_FIMC2 + S5P_SZ_SYSMMU - 1,
		.flags	= IORESOURCE_MEM,
	},
	[9] = {
		.start	= IRQ_SYSMMU_FIMC2_0,
		.end	= IRQ_SYSMMU_FIMC2_0,
		.flags	= IORESOURCE_IRQ,
	},
	[10] = {
		.start	= S5P_PA_SYSMMU_FIMC3,
		.end	= S5P_PA_SYSMMU_FIMC3 + S5P_SZ_SYSMMU - 1,
		.flags	= IORESOURCE_MEM,
	},
	[11] = {
		.start	= IRQ_SYSMMU_FIMC3_0,
		.end	= IRQ_SYSMMU_FIMC3_0,
		.flags	= IORESOURCE_IRQ,
	},
	[12] = {
		.start	= S5P_PA_SYSMMU_JPEG,
		.end	= S5P_PA_SYSMMU_JPEG + S5P_SZ_SYSMMU - 1,
		.flags	= IORESOURCE_MEM,
	},
	[13] = {
		.start	= IRQ_SYSMMU_JPEG_0,
		.end	= IRQ_SYSMMU_JPEG_0,
		.flags	= IORESOURCE_IRQ,
	},
	[14] = {
		.start	= S5P_PA_SYSMMU_FIMD0,
		.end	= S5P_PA_SYSMMU_FIMD0 + S5P_SZ_SYSMMU - 1,
		.flags	= IORESOURCE_MEM,
	},
	[15] = {
		.start	= IRQ_SYSMMU_LCD0_M0_0,
		.end	= IRQ_SYSMMU_LCD0_M0_0,
		.flags	= IORESOURCE_IRQ,
	},
	[16] = {
		.start	= S5P_PA_SYSMMU_FIMD1,
		.end	= S5P_PA_SYSMMU_FIMD1 + S5P_SZ_SYSMMU - 1,
		.flags	= IORESOURCE_MEM,
	},
	[17] = {
		.start	= IRQ_SYSMMU_LCD1_M1_0,
		.end	= IRQ_SYSMMU_LCD1_M1_0,
		.flags	= IORESOURCE_IRQ,
	},
	[18] = {
		.start	= S5P_PA_SYSMMU_PCIe,
		.end	= S5P_PA_SYSMMU_PCIe + S5P_SZ_SYSMMU - 1,
		.flags	= IORESOURCE_MEM,
	},
	[19] = {
		.start	= IRQ_SYSMMU_PCIE_0,
		.end	= IRQ_SYSMMU_PCIE_0,
		.flags	= IORESOURCE_IRQ,
	},
	[20] = {
		.start	= S5P_PA_SYSMMU_G2D,
		.end	= S5P_PA_SYSMMU_G2D + S5P_SZ_SYSMMU - 1,
		.flags	= IORESOURCE_MEM,
	},
	[21] = {
		.start	= IRQ_SYSMMU_2D_0,
		.end	= IRQ_SYSMMU_2D_0,
		.flags	= IORESOURCE_IRQ,
	},
	[22] = {
		.start	= S5P_PA_SYSMMU_ROTATOR,
		.end	= S5P_PA_SYSMMU_ROTATOR + S5P_SZ_SYSMMU - 1,
		.flags	= IORESOURCE_MEM,
	},
	[23] = {
		.start	= IRQ_SYSMMU_ROTATOR_0,
		.end	= IRQ_SYSMMU_ROTATOR_0,
		.flags	= IORESOURCE_IRQ,
	},
	[24] = {
		.start	= S5P_PA_SYSMMU_MDMA2,
		.end	= S5P_PA_SYSMMU_MDMA2 + S5P_SZ_SYSMMU - 1,
		.flags	= IORESOURCE_MEM,
	},
	[25] = {
		.start	= IRQ_SYSMMU_MDMA1_0,
		.end	= IRQ_SYSMMU_MDMA1_0,
		.flags	= IORESOURCE_IRQ,
	},
	[26] = {
		.start	= S5P_PA_SYSMMU_TV,
		.end	= S5P_PA_SYSMMU_TV + S5P_SZ_SYSMMU - 1,
		.flags	= IORESOURCE_MEM,
	},
	[27] = {
		.start	= IRQ_SYSMMU_TV_M0_0,
		.end	= IRQ_SYSMMU_TV_M0_0,
		.flags	= IORESOURCE_IRQ,
	},
	[28] = {
		.start	= S5P_PA_SYSMMU_MFC_L,
		.end	= S5P_PA_SYSMMU_MFC_L + S5P_SZ_SYSMMU - 1,
		.flags	= IORESOURCE_MEM,
	},
	[29] = {
		.start	= IRQ_SYSMMU_MFC_M0_0,
		.end	= IRQ_SYSMMU_MFC_M0_0,
		.flags	= IORESOURCE_IRQ,
	},
	[30] = {
		.start	= S5P_PA_SYSMMU_MFC_R,
		.end	= S5P_PA_SYSMMU_MFC_R + S5P_SZ_SYSMMU - 1,
		.flags	= IORESOURCE_MEM,
	},
	[31] = {
		.start	= IRQ_SYSMMU_MFC_M1_0,
		.end	= IRQ_SYSMMU_MFC_M1_0,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device s5p_device_sysmmu = {
	.name		= "s5p-sysmmu",
	.id		= 32,
	.num_resources	= ARRAY_SIZE(s5p_sysmmu_resource),
	.resource	= s5p_sysmmu_resource,
};
EXPORT_SYMBOL(s5p_device_sysmmu);
#endif

static struct resource s5p_ace_resource[] = {
	[0] = {
		.start = S5P_PA_ACE,
		.end   = S5P_PA_ACE + S5P_SZ_ACE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_INTFEEDCTRL_SSS,
		.end   = IRQ_INTFEEDCTRL_SSS,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device s5p_device_ace = {
	.name		= "s5p-ace",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(s5p_ace_resource),
	.resource	= s5p_ace_resource,
};
EXPORT_SYMBOL(s5p_device_ace);
