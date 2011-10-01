/* linux/arch/arm/mach-s5pv310/setup-fimc0.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Base FIMC 0 gpio configuration
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
#include <mach/regs-gpio.h>
#include <plat/map-s5p.h>
#include <mach/map.h>

struct platform_device; /* don't need the contents */

void s3c_fimc0_cfg_gpio(struct platform_device *pdev)
{
	int i = 0;

	/* CAM A port(b0010) : PCLK, VSYNC, HREF, DATA[0-4] */
	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PV310_GPJ0(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PV310_GPJ0(i), S3C_GPIO_PULL_NONE);
	}
	/* CAM A port(b0010) : DATA[5-7], CLKOUT(MIPI CAM also), FIELD */
	for (i = 0; i < 5; i++) {
		s3c_gpio_cfgpin(S5PV310_GPJ1(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PV310_GPJ1(i), S3C_GPIO_PULL_NONE);
	}
#if defined(CONFIG_MACH_SMDKC210) || defined(CONFIG_MACH_SMDKV310)
	/* CAM B port(b0011) : DATA[0-7] */
	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PV310_GPE1(i), S3C_GPIO_SFN(3));
		s3c_gpio_setpull(S5PV310_GPE1(i), S3C_GPIO_PULL_NONE);
	}
	/* CAM B port(b0011) : PCLK, VSYNC, HREF, FIELD, CLCKOUT */
	for (i = 0; i < 5; i++) {
		s3c_gpio_cfgpin(S5PV310_GPE0(i), S3C_GPIO_SFN(3));
		s3c_gpio_setpull(S5PV310_GPE0(i), S3C_GPIO_PULL_NONE);
	}
#endif
	/* note : driver strength to max is unnecessary */
}

int s3c_fimc_clk_on(struct platform_device *pdev, struct clk **clk)
{
	struct clk *sclk_fimc_lclk = NULL;
	struct clk *mout_epll = NULL;

	sclk_fimc_lclk = clk_get(&pdev->dev, "sclk_fimc");
	if (IS_ERR(sclk_fimc_lclk)) {
		dev_err(&pdev->dev, "failed to get sclk_fimc_lclk\n");
		goto err_clk3;
	}

	/* be able to handle clock on/off only with this clock */
	*clk = clk_get(&pdev->dev, "fimc");
	if (IS_ERR(clk)) {
		dev_err(&pdev->dev, "failed to get interface clock\n");
		goto err_clk3;
	}
	mout_epll = clk_get(&pdev->dev, "mout_epll");

	clk_enable(*clk);
	clk_enable(sclk_fimc_lclk);

	return 0;

err_clk3:
	clk_put(sclk_fimc_lclk);

err_clk1:
	return -EINVAL;
}

int s3c_fimc_clk_off(struct platform_device *pdev, struct clk **clk)
{
	if (*clk != NULL) {
		clk_disable(*clk);
		clk_put(*clk);
		*clk = NULL;
	}

	return 0;
}
