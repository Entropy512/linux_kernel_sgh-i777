/* linux/arch/arm/plat-samsung/dev-i2c5.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S3C series device definition for i2c device 5
 *
 * Based on plat-samsung/dev-i2c0.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/gfp.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/i2c-gpio.h>

#include <mach/irqs.h>
#include <mach/map.h>
#include <mach/gpio.h>

#include <plat/regs-iic.h>
#include <plat/iic.h>
#include <plat/devs.h>
#include <plat/cpu.h>

#if defined CONFIG_MFD_MAX8998
static struct i2c_gpio_platform_data gpio_i2c_data5 __initdata = {
	.sda_pin	= S5PV310_GPB(6),
	.scl_pin	= S5PV310_GPB(7),
};

struct platform_device s3c_device_i2c5 = {
	.name	= "i2c-gpio",
	.id	= 5,
	.dev.platform_data	= &gpio_i2c_data5,
};

void __init s3c_i2c5_set_platdata(struct s3c2410_platform_i2c *pd)
{
	/* Do nothing */
}

#else

static struct resource s3c_i2c_resource[] = {
	[0] = {
		.start = S3C_PA_IIC5,
		.end   = S3C_PA_IIC5 + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_IIC5,
		.end   = IRQ_IIC5,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device s3c_device_i2c5 = {
	.name		  = "s3c2410-i2c",
	.id		  = 5,
	.num_resources	  = ARRAY_SIZE(s3c_i2c_resource),
	.resource	  = s3c_i2c_resource,
};

static struct s3c2410_platform_i2c default_i2c_data5 __initdata = {
	.flags		= 0,
	.bus_num	= 5,
	.slave_addr	= 0x10,
	.frequency	= 400*1000,
	.sda_delay	= 100,
};

void __init s3c_i2c5_set_platdata(struct s3c2410_platform_i2c *pd)
{
	struct s3c2410_platform_i2c *npd;

	if (!pd)
		pd = &default_i2c_data5;

	npd = kmemdup(pd, sizeof(struct s3c2410_platform_i2c), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);
	else if (!npd->cfg_gpio)
		npd->cfg_gpio = s3c_i2c5_cfg_gpio;

	s3c_device_i2c5.dev.platform_data = npd;
}

#endif
