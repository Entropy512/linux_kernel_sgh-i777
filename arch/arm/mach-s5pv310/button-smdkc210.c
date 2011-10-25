/* linux/arch/arm/mach-s5pv310/button-smdkv310.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S5PV310 - Button Driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/serial_core.h>
#include <linux/io.h>
#include <linux/platform_device.h>

#include <plat/map-base.h>
#include <plat/gpio-cfg.h>

#include <mach/regs-gpio.h>
#include <mach/regs-irq.h>
#include <linux/gpio.h>

static irqreturn_t
s3c_button_interrupt(int irq, void *dev_id)
{
	if (irq == IRQ_EINT(0))
		printk(KERN_INFO "XEINT 0 Button Interrupt occure\n");
	else if (irq == IRQ_EINT(31))
		printk(KERN_INFO "XEINT 31 Button Interrupt occure\n");
	else
		printk(KERN_INFO "%d Button Interrupt occure\n", irq);

	return IRQ_HANDLED;
}

static struct irqaction s3c_button_irq = {
	.name		= "s3c button Tick",
	.flags		= IRQF_SHARED ,
	.handler	= s3c_button_interrupt,
};

static unsigned int s3c_button_gpio_init(void)
{
	u32 err;

	err = gpio_request(S5PV310_GPX3(7), "GPH3");
	if (err) {
		printk(KERN_INFO "gpio request error : %d\n", err);
	} else {
		s3c_gpio_cfgpin(S5PV310_GPX3(7), (0xf << 28));
		s3c_gpio_setpull(S5PV310_GPX3(7), S3C_GPIO_PULL_NONE);
	}

	return err;
}

static int __init s3c_button_init(void)
{
	printk(KERN_INFO"SMDKV310 Button init function\n");

	if (s3c_button_gpio_init()) {
		printk(KERN_ERR "%s failed\n", __func__);
		return 0;
	}

	set_irq_type(gpio_to_irq(S5PV310_GPX3(7)), IRQ_TYPE_EDGE_FALLING);

	setup_irq(gpio_to_irq(S5PV310_GPX3(7)), &s3c_button_irq);

#ifdef CONFIG_PM
	set_irq_wake(gpio_to_irq(S5PV310_GPX3(7)), 1);
#endif

	return 0;
}
late_initcall(s3c_button_init);
