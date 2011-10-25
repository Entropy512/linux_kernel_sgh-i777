/* linux/arch/arm/plat-s5pc1xx/setup-sdhci-gpio.c
 *
 * Copyright (c) 2009-2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S5PV310 - Helper functions for setting up SDHCI device(s) GPIO (HSMMC)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>

#include <mach/gpio.h>
#include <mach/map.h>
#include <plat/gpio-cfg.h>
#include <plat/mshci.h>

#define GPK0DRV	(S5PV310_VA_GPIO2 + 0x4C)
#define GPK1DRV	(S5PV310_VA_GPIO2 + 0x6C)
#define GPK2DRV	(S5PV310_VA_GPIO2 + 0x8C)
#define GPK3DRV	(S5PV310_VA_GPIO2 + 0xAC)

#if defined (CONFIG_S5PV310_MSHC_VPLL_46MHZ) || \
	defined (CONFIG_S5PV310_MSHC_EPLL_45MHZ)
#define DIV_FSYS3	(S5PV310_VA_CMU + 0x0C54C)
#endif


void s5pv310_setup_mshci_cfg_gpio(struct platform_device *dev, int width)
{
	unsigned int gpio;
	struct s3c_mshci_platdata *pdata = dev->dev.platform_data;

	early_printk("v310_setup_mshci_cfg_gpio\n");

	/* Set all the necessary GPG0/GPG1 pins to special-function 2 */
	for (gpio = S5PV310_GPK0(0); gpio < S5PV310_GPK0(2); gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
			s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	}

	/* if CDn pin is used as eMMC_EN pin, it might make a problem
	   So, a built-in type eMMC is embedded, it dose not set CDn pin */
	if ( pdata->cd_type != S3C_MSHCI_CD_PERMANENT ) {
		s3c_gpio_cfgpin(S5PV310_GPK0(2), S3C_GPIO_SFN(3));
		s3c_gpio_setpull(S5PV310_GPK0(2), S3C_GPIO_PULL_UP);
	}

	switch (width) {
	case 8:
		for (gpio = S5PV310_GPK1(3); gpio <= S5PV310_GPK1(6); gpio++) {
			s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(4));
			s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
		}
		__raw_writel(0x2AAA, GPK1DRV);
	case 4:
		/* GPK[3:6] special-funtion 2 */
		for (gpio = S5PV310_GPK0(3); gpio <= S5PV310_GPK0(6); gpio++) {
			s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
			s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
		}
		__raw_writel(0x2AAA, GPK0DRV);
		break;
	case 1:
		/* GPK[3] special-funtion 2 */
		for (gpio = S5PV310_GPK0(3); gpio < S5PV310_GPK0(4); gpio++) {
			s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
			s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
		}
		__raw_writel(0xAA, GPK0DRV);
	default:
		break;
	}
}

#if defined (CONFIG_S5PV310_MSHC_VPLL_46MHZ) || \
	defined (CONFIG_S5PV310_MSHC_EPLL_45MHZ)
void s5pv310_setup_mshci_cfg_ddr(struct platform_device *dev, int ddr)
{
		if (ddr) {
#ifdef CONFIG_S5PV310_MSHC_EPLL_45MHZ
			__raw_writel(0x00, DIV_FSYS3);
#else
			__raw_writel(0x01, DIV_FSYS3);
#endif
		} else {
#ifdef CONFIG_S5PV310_MSHC_EPLL_45MHZ
			__raw_writel(0x01, DIV_FSYS3);
#else
			__raw_writel(0x03, DIV_FSYS3);
#endif
		}
}
#endif

#ifdef CONFIG_MACH_C1
void s5pv310_setup_mshci_init_card(struct platform_device *dev)
{
	/*
	 * Reset moviNAND for re-init.
	 * output/low for eMMC_EN and input/pull-none for others
	 * and then wait 10ms.
	 */
	__raw_writel(0x100, S5PV310_VA_GPIO2 + 0x40);
	__raw_writel(0, S5PV310_VA_GPIO2 + 0x44);
	__raw_writel(0, S5PV310_VA_GPIO2 + 0x48);
	__raw_writel(0, S5PV310_VA_GPIO2 + 0x60);
	__raw_writel(0, S5PV310_VA_GPIO2 + 0x64);
	__raw_writel(0, S5PV310_VA_GPIO2 + 0x68);
	mdelay(100);

	/* set data buswidth 8 */
	s5pv310_setup_mshci_cfg_gpio(dev, 8);

	/* power to moviNAND on */
	gpio_set_value(S5PV310_GPK0(2), 1);

	/* to wait a pull-up resistance ready */
	mdelay(10);
}
#endif

#ifdef CONFIG_MACH_C1
void s5pv310_setup_mshci_set_power(struct platform_device *dev, int en)
{
	struct s3c_mshci_platdata *pdata = dev->dev.platform_data;

	if (pdata->int_power_gpio) {
		if (en) {
			gpio_set_value(pdata->int_power_gpio, 1);
			pr_info("%s : internal MMC Card ON samsung-mshc.\n",
					__func__);
		} else {
			gpio_set_value(pdata->int_power_gpio, 0);
			pr_info("%s : internal MMC Card OFF samsung-mshc.\n",
					__func__);
		}
	}
}
#endif

void s5pv310_setup_mshci_shutdown()
{
	/*
	 * Workaround for MoviNAND settle down time issue:
	 * output/low for eMMC_EN and input/pull-none for others
	 * and then wait 400ms before reset
	 */
	__raw_writel(0x100, S5PV310_VA_GPIO2 + 0x40);
	__raw_writel(0, S5PV310_VA_GPIO2 + 0x44);
	__raw_writel(0, S5PV310_VA_GPIO2 + 0x48);
	__raw_writel(0, S5PV310_VA_GPIO2 + 0x60);
	__raw_writel(0, S5PV310_VA_GPIO2 + 0x64);
	__raw_writel(0, S5PV310_VA_GPIO2 + 0x68);
	mdelay(400);
}
