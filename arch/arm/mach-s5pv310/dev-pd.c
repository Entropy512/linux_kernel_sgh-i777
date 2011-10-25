/* linux/arch/arm/mach-s5pv310/dev-pd.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * S5PV310 - Power Domain support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/delay.h>

#include <plat/pd.h>

#include <mach/regs-pmu.h>
#include <mach/regs-clock.h>

#define POWERDOMAIN_NAME_LEN 16

static int s5pv310_pd_init(struct device *dev)
{
	struct samsung_pd_info *pdata =  dev->platform_data;
	struct s5pv310_pd_data *data = (struct s5pv310_pd_data *) pdata->data;

	if (data->read_phy_addr) {
		data->read_base = ioremap(data->read_phy_addr, SZ_4K);
		if (!data->read_base)
			return -ENOMEM;
	}

	return 0;
}

int s5pv310_pd_enable(struct device *dev)
{
	struct samsung_pd_info *pdata =  dev->platform_data;
	struct s5pv310_pd_data *data = (struct s5pv310_pd_data *) pdata->data;
	u32 timeout;
	u32 tmp = 0;

	if (data->read_base)
		/*  save IP clock gating register */
		tmp = __raw_readl(data->clk_base);

	/*  enable all the clocks of IPs in the power domain */
	__raw_writel(0xffffffff, data->clk_base);

	__raw_writel(S5P_INT_LOCAL_PWR_EN, pdata->base);

	/* Wait max 1ms */
	timeout = 1000;
	while ((__raw_readl(pdata->base + 0x4) & S5P_INT_LOCAL_PWR_EN)
		!= S5P_INT_LOCAL_PWR_EN) {
		if (timeout == 0) {
			printk(KERN_ERR "Power domain %s enable failed.\n",
				dev_name(dev));
			break;
		}
		timeout--;
		udelay(1);
	}

	if (timeout ==0) {
		timeout = 1000;
		__raw_writel(0x1, pdata->base + 0x8);
		__raw_writel(S5P_INT_LOCAL_PWR_EN, pdata->base);
		while ((__raw_readl(pdata->base + 0x4) & S5P_INT_LOCAL_PWR_EN)
			!= S5P_INT_LOCAL_PWR_EN) {
			if (timeout == 0) {
				printk(KERN_ERR "Power domain %s enable failed 2nd.\n",
					dev_name(dev));
				BUG();
				return -ETIMEDOUT;
			}
			timeout--;
			udelay(1);
		}
		__raw_writel(0x2, pdata->base + 0x8);
	}

	if (data->read_base) {
		/* dummy read to check the completion of power-on sequence */
		__raw_readl(data->read_base);

		/* restore IP clock gating register */
		__raw_writel(tmp, data->clk_base);
	}

	return 0;
}

static int s5pv310_pd_disable(struct device *dev)
{
	struct samsung_pd_info *pdata =  dev->platform_data;
	u32 timeout;

	static int boot_lcd0 = 1;
	if (boot_lcd0) {
		if(!strncmp(dev_name(dev), "samsung-pd.2", POWERDOMAIN_NAME_LEN)) {
			printk("ldo0 not disable only initial booting");
			boot_lcd0 = 0;
			return 0;
		}
		printk("try to disable lcd0 in booting");
	}

	__raw_writel(0, pdata->base);

	/* Wait max 1ms */
	timeout = 1000;
	while (__raw_readl(pdata->base + 0x4) & S5P_INT_LOCAL_PWR_EN) {
		if (timeout == 0) {
			printk(KERN_ERR "Power domain %s disable failed.\n",
				dev_name(dev));
			break;
		}
		timeout--;
		udelay(1);
	}

	if (timeout ==0) {
		timeout = 1000;
		__raw_writel(0x1, pdata->base + 0x8);
		__raw_writel(0, pdata->base);
		while (__raw_readl(pdata->base + 0x4) & S5P_INT_LOCAL_PWR_EN) {
			if (timeout == 0) {
				printk(KERN_ERR "Power domain %s disable failed 2nd.\n",
					dev_name(dev));
				BUG();
				return -ETIMEDOUT;
			}
			timeout--;
			udelay(1);
		}
		__raw_writel(0x2, pdata->base + 0x8);
	}

	return 0;
}

struct platform_device s5pv310_device_pd[] = {
	{
		.name		= "samsung-pd",
		.id		= 0,
		.dev = {
			.platform_data = &(struct samsung_pd_info) {
				.init		= s5pv310_pd_init,
				.enable		= s5pv310_pd_enable,
				.disable	= s5pv310_pd_disable,
				.base		= S5P_PMU_MFC_CONF,
				.data		= &(struct s5pv310_pd_data) {
					.clk_base	= S5P_CLKGATE_IP_MFC,
				},
			},
		},
	}, {
		.name		= "samsung-pd",
		.id		= 1,
		.dev = {
			.platform_data = &(struct samsung_pd_info) {
				.init		= s5pv310_pd_init,
				.enable		= s5pv310_pd_enable,
				.disable	= s5pv310_pd_disable,
				.base		= S5P_PMU_G3D_CONF,
				.data		= &(struct s5pv310_pd_data) {
					.clk_base	= S5P_CLKGATE_IP_G3D,
				},
			},
		},
	}, {
		.name		= "samsung-pd",
		.id		= 2,
		.dev = {
			.platform_data = &(struct samsung_pd_info) {
				.init		= s5pv310_pd_init,
				.enable		= s5pv310_pd_enable,
				.disable	= s5pv310_pd_disable,
				.base		= S5P_PMU_LCD0_CONF,
				.data		= &(struct s5pv310_pd_data) {
					.clk_base	= S5P_CLKGATE_IP_LCD0,
					.read_phy_addr	= S5PV310_PA_LCD0,
				},
			},
		},
	}, {
		.name		= "samsung-pd",
		.id		= 3,
		.dev = {
			.platform_data = &(struct samsung_pd_info) {
				.init		= s5pv310_pd_init,
				.enable		= s5pv310_pd_enable,
				.disable	= s5pv310_pd_disable,
				.base		= S5P_PMU_LCD1_CONF,
				.data		= &(struct s5pv310_pd_data) {
					.clk_base	= S5P_CLKGATE_IP_LCD1,
					.read_phy_addr	= S5PV310_PA_LCD1,
				},
			},
		},
	}, {
		.name		= "samsung-pd",
		.id		= 4,
		.dev = {
			.platform_data = &(struct samsung_pd_info) {
				.init		= s5pv310_pd_init,
				.enable		= s5pv310_pd_enable,
				.disable	= s5pv310_pd_disable,
				.base		= S5P_PMU_TV_CONF,
				.data		= &(struct s5pv310_pd_data) {
					.clk_base	= S5P_CLKGATE_IP_TV,
					.read_phy_addr	= S5PV310_PA_VP,
				},
			},
		},
	}, {
		.name		= "samsung-pd",
		.id		= 5,
		.dev = {
			.platform_data = &(struct samsung_pd_info) {
				.init		= s5pv310_pd_init,
				.enable		= s5pv310_pd_enable,
				.disable	= s5pv310_pd_disable,
				.base		= S5P_PMU_CAM_CONF,
				.data		= &(struct s5pv310_pd_data) {
					.clk_base	= S5P_CLKGATE_IP_CAM,
					.read_phy_addr	= S5PV310_PA_FIMC0,
				},
			},
		},
	}, {
		.name		= "samsung-pd",
		.id		= 6,
		.dev = {
			.platform_data = &(struct samsung_pd_info) {
				.init		= s5pv310_pd_init,
				.enable		= s5pv310_pd_enable,
				.disable	= s5pv310_pd_disable,
				.base		= S5P_PMU_GPS_CONF,
				.data		= &(struct s5pv310_pd_data) {
					.clk_base	= S5P_CLKGATE_IP_GPS,
				},
			},
		},
	},
};
EXPORT_SYMBOL(s5pv310_device_pd);
