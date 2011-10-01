/* linux/arch/arm/mach-s5p6450/button-smdk6450.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S5PV210 - Button Driver
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
	printk(KERN_INFO "### %d\n", irq);
	if (irq == IRQ_EINT(9))
		printk(KERN_INFO "XEINT 9 Button Interrupt occure\n");
	else if (irq == IRQ_EINT(10))
		printk(KERN_INFO "XEINT 10 Button Interrupt occure\n");
	else if (irq == IRQ_EINT(11))
		printk(KERN_INFO "XEINT 11 Button Interrupt occure\n");
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

	err = gpio_request(S5P6450_GPN(9), "GPN");
	if (err) {
		printk(KERN_INFO "gpio request error : %d\n", err);
	} else {
		s3c_gpio_cfgpin(S5P6450_GPN(9), S3C_GPIO_SFN(3));
		s3c_gpio_setpull(S5P6450_GPN(9), S3C_GPIO_PULL_NONE);
	}

	err = gpio_request(S5P6450_GPN(10), "GPN");
	if (err) {
		printk(KERN_INFO "gpio request error : %d\n", err);
	} else {
		s3c_gpio_cfgpin(S5P6450_GPN(10), S3C_GPIO_SFN(3));
		s3c_gpio_setpull(S5P6450_GPN(10), S3C_GPIO_PULL_NONE);
	}

	err = gpio_request(S5P6450_GPN(11), "GPN");
	if (err) {
		printk(KERN_INFO "gpio request error : %d\n", err);
	} else {
		s3c_gpio_cfgpin(S5P6450_GPN(11), S3C_GPIO_SFN(3));
		s3c_gpio_setpull(S5P6450_GPN(11), S3C_GPIO_PULL_NONE);
	}

	return err;
}

static int __init s3c_button_init(void)
{
	printk(KERN_INFO "SMDK6450 Button init function \n");

	if (s3c_button_gpio_init()) {
		printk(KERN_ERR "%s failed\n", __func__);
		return 0;
	}

	set_irq_type(IRQ_EINT(9), IRQF_TRIGGER_FALLING);
	set_irq_wake(IRQ_EINT(9), 1);
	setup_irq(IRQ_EINT(9), &s3c_button_irq);

	set_irq_type(IRQ_EINT(10), IRQF_TRIGGER_FALLING);
	set_irq_wake(IRQ_EINT(10), 1);
	setup_irq(IRQ_EINT(10), &s3c_button_irq);

	set_irq_type(IRQ_EINT(11), IRQ_TYPE_EDGE_FALLING);
	set_irq_wake(IRQ_EINT(11), 1);
	setup_irq(IRQ_EINT(11), &s3c_button_irq);

	return 0;
}
late_initcall(s3c_button_init);
