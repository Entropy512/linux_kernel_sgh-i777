/* linux/arch/arm/plat-s5pc1xx/setup-sdhci-gpio.c
 *
 * Copyright (c) 2009-2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S5PV210 - Helper functions for setting up SDHCI device(s) GPIO (HSMMC)
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
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>

#include <mach/gpio.h>
#include <plat/gpio-cfg.h>

void s5p6450_setup_mshci_cfg_gpio(struct platform_device *dev, int width)
{
	unsigned int gpio;

	/* Set all the necessary GPG[7:8] pins to special-function 2 */
	for (gpio = S5P6450_GPG(7); gpio < S5P6450_GPG(9); gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(2));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	}

	switch(width) {
	case 8:
		/* Data pin GPQ[6:9] to special-function 3 */
		for (gpio = S5P6450_GPQ(6); gpio <= S5P6450_GPQ(9); gpio++) {
			s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
			s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
		}
	case 4:
		/* Data pin GPG[9:12] to special-function 2 */
		for (gpio = S5P6450_GPG(9); gpio <= S5P6450_GPG(12); gpio++) {
			s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(2));
			s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
		}
	}

	/* GPG[13] special-funtion 2 : MMC3 CDn */
	s3c_gpio_cfgpin(S5P6450_GPG(13), S3C_GPIO_SFN(2));
	s3c_gpio_setpull(S5P6450_GPG(13), S3C_GPIO_PULL_UP);
}
