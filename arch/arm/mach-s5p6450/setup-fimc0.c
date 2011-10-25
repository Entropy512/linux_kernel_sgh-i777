/* linux/arch/arm/mach-s5p6450/setup-fimc.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Base FIMC gpio configuration
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <plat/clock.h>
#include <plat/gpio-cfg.h>
#include <plat/map-s5p.h>
#include <mach/regs-gpio.h>
#include <mach/map.h>

struct platform_device; /* don't need the contents */

void s3c_fimc0_cfg_gpio(struct platform_device *pdev)
{
	int i = 0;

	/* CAM A port(b0010) : PCLK, VSYNC, HREF, DATA[0-4] */
	for (i=0; i < 14; i++) {
		s3c_gpio_cfgpin(S5P6450_GPQ(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5P6450_GPQ(i), S3C_GPIO_PULL_NONE);
	}
	/* note : driver strength to max is unnecessary */
}
int s3c_fimc_clk_on(struct platform_device *pdev, struct clk **clk)
{
	struct clk *sclk_fimc = NULL;
	struct clk *dout_mpll = NULL;
	struct clk *hclk_low = NULL;
	
	/* fimc core clock(166Mhz) */

	dout_mpll = clk_get(&pdev->dev, "dout_mpll");
	if (IS_ERR(dout_mpll)) {
		dev_err(&pdev->dev, "failed to get dout_mpll\n");
		goto err_clk2;
	}

	sclk_fimc = clk_get(&pdev->dev, "sclk_fimc");
	if (IS_ERR(sclk_fimc)) {
		dev_err(&pdev->dev, "failed to get sclk_fimc\n");
		goto err_clk3;
	}

	clk_set_parent(sclk_fimc, dout_mpll);
	clk_set_rate(sclk_fimc, 200000000);

	clk_enable(sclk_fimc);
	/* bus clock(133Mhz) */
	hclk_low = clk_get(&pdev->dev, "fimc");
	if (IS_ERR(hclk_low)) {
		dev_err(&pdev->dev, "failed to get hclk_low\n");
		goto err_clk4;
	}
	
	clk_enable(hclk_low);
	
	return 0;

err_clk4:
	clk_put(hclk_low);
err_clk3:
	clk_put(sclk_fimc);
err_clk2:
	return -EINVAL;
}

int s3c_fimc_clk_off(struct platform_device *pdev, struct clk **clk)
{
	struct clk *sclk_fimc = NULL;
	struct clk *hclk_low = NULL;

	sclk_fimc = clk_get(&pdev->dev, "sclk_fimc");
	if (IS_ERR(sclk_fimc)) {
		dev_err(&pdev->dev, "failed to get sclk_fimc\n");
		goto err_clk1;
	}
	clk_disable(sclk_fimc);
	clk_put(sclk_fimc);

	/* bus clock(133Mhz) */
	hclk_low = clk_get(&pdev->dev, "fimc");
	if (IS_ERR(hclk_low)) {
		dev_err(&pdev->dev, "failed to get hclk_low\n");
		goto err_clk2;
	}
	clk_disable(hclk_low);
	clk_put(hclk_low);

	return 0;

err_clk1:
	clk_put(sclk_fimc);
err_clk2:
	clk_put(hclk_low);
	return -EINVAL;
}
