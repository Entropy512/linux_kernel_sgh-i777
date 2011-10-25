/* linux/arch/arm/mach-s5pv210/power-domain.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S5PV210 - Power domain support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/regulator/samsung_pd.h>
#include <linux/regulator/machine.h>

#include <mach/regs-clock.h>

enum power_domain {
	PD_AUDIO,
	PD_CAM,
	PD_TV,
	PD_LCD,
	PD_G3D,
	PD_MFC
};

static void s5pv210_pd_enable(enum power_domain pd)
{
	u32 pd_reg = __raw_readl(S5P_NORMAL_CFG);

	switch (pd) {
	case PD_AUDIO:
		pd_reg |= 1 << 7;
		break;
	case PD_CAM:
		pd_reg |= 1 << 5;
		break;
	case PD_TV:
		pd_reg |= 1 << 4;
		break;
	case PD_LCD:
		pd_reg |= 1 << 3;
		break;
	case PD_G3D:
		pd_reg |= 1 << 2;
		break;
	case PD_MFC:
		pd_reg |= 1 << 1;
		break;
	default:
		printk(KERN_WARNING "Wrong power domain enable request.\n");
		return;
	}

	__raw_writel(pd_reg, S5P_NORMAL_CFG);
}

static void s5pv210_pd_disable(enum power_domain pd)
{
	u32 pd_reg = __raw_readl(S5P_NORMAL_CFG);

	switch (pd) {
	case PD_AUDIO:
		pd_reg &= ~(1 << 7);
		break;
	case PD_CAM:
		pd_reg &= ~(1 << 5);
		break;
	case PD_TV:
		pd_reg &= ~(1 << 4);
		break;
	case PD_LCD:
		pd_reg &= ~(1 << 3);
		break;
	case PD_G3D:
		pd_reg &= ~(1 << 2);
		break;
	case PD_MFC:
		pd_reg &= ~(1 << 1);
		break;
	default:
		printk(KERN_WARNING "Wrong power domain disable request.\n");
		return;
	}

	__raw_writel(pd_reg, S5P_NORMAL_CFG);
}

static void s5pv210_pd_audio_enable(void)
{
	s5pv210_pd_enable(PD_AUDIO);
}

static void s5pv210_pd_cam_enable(void)
{
	s5pv210_pd_enable(PD_CAM);
}

static void s5pv210_pd_tv_enable(void)
{
	s5pv210_pd_enable(PD_TV);
}

static void s5pv210_pd_lcd_enable(void)
{
	s5pv210_pd_enable(PD_LCD);
}

static void s5pv210_pd_g3d_enable(void)
{
	s5pv210_pd_enable(PD_G3D);
}

static void s5pv210_pd_mfc_enable(void)
{
	s5pv210_pd_enable(PD_MFC);
}

static void s5pv210_pd_audio_disable(void)
{
	s5pv210_pd_disable(PD_AUDIO);
}

static void s5pv210_pd_cam_disable(void)
{
	s5pv210_pd_disable(PD_CAM);
}

static void s5pv210_pd_tv_disable(void)
{
	s5pv210_pd_disable(PD_TV);
}

static void s5pv210_pd_lcd_disable(void)
{
	s5pv210_pd_disable(PD_LCD);
}

static void s5pv210_pd_g3d_disable(void)
{
	s5pv210_pd_disable(PD_G3D);
}

static void s5pv210_pd_mfc_disable(void)
{
	s5pv210_pd_disable(PD_MFC);
}

static struct regulator_consumer_supply s5pv210_pd_audio_supply[] = {
        REGULATOR_SUPPLY("PD", "s3c-ac97"),
};

static struct regulator_consumer_supply s5pv210_pd_cam_supply[] = {
        REGULATOR_SUPPLY("PD", "s3c-fimc"),
        REGULATOR_SUPPLY("PD", "s3c-jpg"),
        REGULATOR_SUPPLY("PD", "s3c-csis"),
        REGULATOR_SUPPLY("PD", "s5p-rotator"),
};

static struct regulator_consumer_supply s5pv210_pd_tv_supply[] = {
        REGULATOR_SUPPLY("PD", "s5p-tvout"),
};

static struct regulator_consumer_supply s5pv210_pd_lcd_supply[] = {
        REGULATOR_SUPPLY("PD", "s3cfb"),
};

static struct regulator_consumer_supply s5pv210_pd_g3d_supply[] = {
        REGULATOR_SUPPLY("PD", "g3d"),
};

static struct regulator_consumer_supply s5pv210_pd_mfc_supply[] = {
        REGULATOR_SUPPLY("PD", "mfc"),
};

static struct regulator_init_data s5pv210_pd_audio_data = {
	.constraints = {
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.valid_ops_mask		= REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= ARRAY_SIZE(s5pv210_pd_audio_supply),
	.consumer_supplies	= s5pv210_pd_audio_supply,
};

static struct regulator_init_data s5pv210_pd_cam_data = {
	.constraints = {
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.valid_ops_mask		= REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= ARRAY_SIZE(s5pv210_pd_cam_supply),
	.consumer_supplies	= s5pv210_pd_cam_supply,
};

static struct regulator_init_data s5pv210_pd_tv_data = {
	.constraints = {
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.valid_ops_mask		= REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= ARRAY_SIZE(s5pv210_pd_tv_supply),
	.consumer_supplies	= s5pv210_pd_tv_supply,
};

static struct regulator_init_data s5pv210_pd_lcd_data = {
	.constraints = {
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.valid_ops_mask		= REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= ARRAY_SIZE(s5pv210_pd_lcd_supply),
	.consumer_supplies	= s5pv210_pd_lcd_supply,
};

static struct regulator_init_data s5pv210_pd_g3d_data = {
	.constraints = {
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.valid_ops_mask		= REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= ARRAY_SIZE(s5pv210_pd_g3d_supply),
	.consumer_supplies	= s5pv210_pd_g3d_supply,
};

static struct regulator_init_data s5pv210_pd_mfc_data = {
	.constraints = {
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.valid_ops_mask		= REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= ARRAY_SIZE(s5pv210_pd_mfc_supply),
	.consumer_supplies	= s5pv210_pd_mfc_supply,
};

struct clk_should_be_running s5pv210_pd_audio_clk[] = {
	{
		.clk_name	= "i2scdclk",
		.id		= 0,
	}, {
		/* end of the clock array */
		.clk_name	= NULL,
		.id		= 0,
	},
};

struct clk_should_be_running s5pv210_pd_cam_clk[] = {
	{
		.clk_name	= "fimc",
		.id		= 0,
	}, {
		.clk_name	= "fimc",
		.id		= 1,
	}, {
		.clk_name	= "fimc",
		.id		= 2,
	}, {
		.clk_name	= "sclk_csis",
		.id		= -1,
	}, {
		.clk_name	= "jpeg",
		.id		= -1,
	}, {
		.clk_name	= "rotator",
		.id		= -1,
	}, {
		/* end of the clock array */
		.clk_name	= NULL,
		.id		= 0,
	},
};

struct clk_should_be_running s5pv210_pd_tv_clk[] = {
	{
		.clk_name	= "vp",
		.id		= -1,
	}, {
		.clk_name	= "mixer",
		.id		= -1,
	}, {
		.clk_name	= "tvenc",
		.id		= -1,
	}, {
		.clk_name	= "hdmi",
		.id		= -1,
	}, {
		/* end of the clock array */
		.clk_name	= NULL,
		.id		= 0,
	},
};

struct clk_should_be_running s5pv210_pd_lcd_clk[] = {
	{
		.clk_name	= "lcd",
		.id		= -1,
	}, {
		.clk_name	= "dsim",
		.id		= -1,
	}, {
		.clk_name	= "sclk_g2d",
		.id		= -1,
	}, {
		/* end of the clock array */
		.clk_name	= NULL,
		.id		= 0,
	},
};

struct clk_should_be_running s5pv210_pd_g3d_clk[] = {
	{
		.clk_name	= "sclk_g3d",
		.id		= 0,
	}, {
		/* end of the clock array */
		.clk_name	= NULL,
		.id		= 0,
	},
};

struct clk_should_be_running s5pv210_pd_mfc_clk[] = {
	{
		.clk_name	= "mfc",
		.id		= 0,
	}, {
		/* end of the clock array */
		.clk_name	= NULL,
		.id		= 0,
	},
};

static struct samsung_pd_config s5pv210_pd_audio_pdata = {
	.supply_name = "pd_audio_supply",
	.microvolts = 5000000,
	.init_data = &s5pv210_pd_audio_data,
	.clk_run = s5pv210_pd_audio_clk,
	.enable = s5pv210_pd_audio_enable,
	.disable = s5pv210_pd_audio_disable,
};

static struct samsung_pd_config s5pv210_pd_cam_pdata = {
	.supply_name = "pd_cam_supply",
	.microvolts = 5000000,
	.init_data = &s5pv210_pd_cam_data,
	.clk_run = s5pv210_pd_cam_clk,
	.enable = s5pv210_pd_cam_enable,
	.disable = s5pv210_pd_cam_disable,
};

static struct samsung_pd_config s5pv210_pd_tv_pdata = {
	.supply_name = "pd_tv_supply",
	.microvolts = 5000000,
	.init_data = &s5pv210_pd_tv_data,
	.clk_run = s5pv210_pd_tv_clk,
	.enable = s5pv210_pd_tv_enable,
	.disable = s5pv210_pd_tv_disable,
};

static struct samsung_pd_config s5pv210_pd_lcd_pdata = {
	.supply_name = "pd_lcd_supply",
	.microvolts = 5000000,
	.init_data = &s5pv210_pd_lcd_data,
	.clk_run = s5pv210_pd_lcd_clk,
	.enable = s5pv210_pd_lcd_enable,
	.disable = s5pv210_pd_lcd_disable,
};

static struct samsung_pd_config s5pv210_pd_g3d_pdata = {
	.supply_name = "pd_g3d_supply",
	.microvolts = 5000000,
	.init_data = &s5pv210_pd_g3d_data,
	.clk_run = s5pv210_pd_g3d_clk,
	.enable = s5pv210_pd_g3d_enable,
	.disable = s5pv210_pd_g3d_disable,
};

static struct samsung_pd_config s5pv210_pd_mfc_pdata = {
	.supply_name = "pd_mfc_supply",
	.microvolts = 5000000,
	.init_data = &s5pv210_pd_mfc_data,
	.clk_run = s5pv210_pd_mfc_clk,
	.enable = s5pv210_pd_mfc_enable,
	.disable = s5pv210_pd_mfc_disable,
};

struct platform_device s5pv210_pd_audio = {
	.name          = "reg-samsung-pd",
	.id            = 0,
	.dev = {
		.platform_data = &s5pv210_pd_audio_pdata,
	},
};

struct platform_device s5pv210_pd_cam = {
	.name          = "reg-samsung-pd",
	.id            = 1,
	.dev = {
		.platform_data = &s5pv210_pd_cam_pdata,
	},
};

struct platform_device s5pv210_pd_tv = {
	.name          = "reg-samsung-pd",
	.id            = 2,
	.dev = {
		.platform_data = &s5pv210_pd_tv_pdata,
	},
};

struct platform_device s5pv210_pd_lcd = {
	.name          = "reg-samsung-pd",
	.id            = 3,
	.dev = {
		.platform_data = &s5pv210_pd_lcd_pdata,
	},
};

struct platform_device s5pv210_pd_g3d = {
	.name          = "reg-samsung-pd",
	.id            = 4,
	.dev = {
		.platform_data = &s5pv210_pd_g3d_pdata,
	},
};

struct platform_device s5pv210_pd_mfc = {
	.name          = "reg-samsung-pd",
	.id            = 5,
	.dev = {
		.platform_data = &s5pv210_pd_mfc_pdata,
	},
};

