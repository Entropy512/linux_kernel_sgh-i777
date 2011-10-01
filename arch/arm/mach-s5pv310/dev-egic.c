/* linux/arch/arm/mach-s5pv310/dev-egic.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S5PV310 - External GIC support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/delay.h>

struct platform_device s5pv310_device_egic = {
	.name	= "s5pv310-external-GIC",
	.id	= -1,
};

static int __init s5pv310_egic_device_init(void)
{
	int ret;

	ret = platform_device_register(&s5pv310_device_egic);

	if (ret) {
		printk(KERN_ERR "failed at(%d)\n", __LINE__);
		return ret;
	}

	return ret;
}

arch_initcall(s5pv310_egic_device_init);
