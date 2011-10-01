/* linux/arch/arm/mach-s5pv310/setup-i2c4.c
 *
 * Copyright (c) 2009-2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * I2C4 GPIO configuration.
 *
 * Based on plat-s3c64xx/setup-i2c0.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>

struct platform_device; /* don't need the contents */

#include <mach/gpio.h>
#include <plat/iic.h>
#include <plat/gpio-cfg.h>

void s3c_i2c4_cfg_gpio(struct platform_device *dev)
{
#if defined(CONFIG_EPEN_WACOM_G5SP) || defined(CONFIG_WIMAX_CMC)
	s3c_gpio_cfgpin(S5PV310_GPB(2), S3C_GPIO_SFN(3));
	s3c_gpio_setpull(S5PV310_GPB(2), S3C_GPIO_PULL_UP);
	s3c_gpio_cfgpin(S5PV310_GPB(3), S3C_GPIO_SFN(3));
	s3c_gpio_setpull(S5PV310_GPB(3), S3C_GPIO_PULL_UP);
#else
	s3c_gpio_cfgpin(S5PV310_GPB(0), S3C_GPIO_SFN(3));
	s3c_gpio_setpull(S5PV310_GPB(0), S3C_GPIO_PULL_UP);
	s3c_gpio_cfgpin(S5PV310_GPB(1), S3C_GPIO_SFN(3));
	s3c_gpio_setpull(S5PV310_GPB(1), S3C_GPIO_PULL_UP);
#endif /* CONFIG_EPEN_WACOM_G5SP */
}
