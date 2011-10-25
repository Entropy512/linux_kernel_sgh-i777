/* linux/arch/arm/plat-s3c2416/setup-sdhci-gpio.c
 *
 * Copyright 2010 Promwad Innovation Company
 *	Yauhen Kharuzhy <yauhen.kharuzhy@xxxxxxxxxxx>
 *
 * S3C2416 - Helper functions for setting up SDHCI device(s) GPIO (HSMMC)
 *
 * Based on mach-s3c64xx/setup-sdhci-gpio.c
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

#include <mach/gpio.h>
#include <mach/regs-gpio.h>
#include <plat/gpio-cfg.h>

/* Note: hsmmc1 and hsmmc0 are swapped versus datasheet */

void s3c2416_setup_sdhci1_cfg_gpio(struct platform_device *dev, int width)
{
	unsigned int gpio;
	unsigned int end;

	end = S3C2410_GPE(7 + width);

	/* Set all the necessary GPE pins to special-function 0 */
	for (gpio = S3C2410_GPE(5); gpio < end; gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(2));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	}
}

void s3c2416_setup_sdhci0_cfg_gpio(struct platform_device *dev, int width)
{
	unsigned int gpio;
	unsigned int end;

	end = S3C2410_GPL(0 + width);

	/* Set all the necessary GPG pins to special-function 0 */
	for (gpio = S3C2410_GPL(0); gpio < end; gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(2));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	}

	s3c_gpio_cfgpin(S3C2410_GPL(8), S3C_GPIO_SFN(2));
	s3c_gpio_setpull(S3C2410_GPL(8), S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(S3C2410_GPL(9), S3C_GPIO_SFN(2));
	s3c_gpio_setpull(S3C2410_GPL(9), S3C_GPIO_PULL_NONE);
}


