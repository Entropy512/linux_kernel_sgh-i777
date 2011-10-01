/* linux/arch/arm/plat-samsung/dev-tmu.c
 *
 * Copyright 2009 by SAMSUNG
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include <asm/irq.h>

#include <mach/irqs.h>
#include <mach/map.h>

#include <plat/devs.h>
#include <plat/s5p-tmu.h>

static struct resource s5p_tmu_resource[] = {
	[0] = {
		.start	= S5P_PA_TMU,
		.end	= S5P_PA_TMU + 0xFFFF - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_TMU_TRIG0,
		.end	= IRQ_TMU_TRIG0,
		.flags	= IORESOURCE_IRQ,
	}
};

struct platform_device s5p_device_tmu= {
	.name	= "s5p-tmu",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s5p_tmu_resource),
	.resource	= s5p_tmu_resource,
};

static struct tmu_data default_tmu_data __initdata = {
	.te1 = 0,
	.te2 = 0,
	.cooling = 82,			/* cooling temperature */
	.tmu_flag = 0,
	.mode = 0,			/* 0: 1-point compensation, 1: 2-point compensation */ 
};

int s5p_tmu_get_irqno(int num)
{
	return platform_get_irq(&s5p_device_tmu,num);
}
	
struct s5p_tmu *s5p_tmu_get_platdata(void)
{   
	return platform_get_drvdata(&s5p_device_tmu);
}

void __init s5p_tmu_set_platdata(struct tmu_data *pd)
{   
	struct s5p_tmu *npd;

	npd = kmalloc(sizeof(struct s5p_tmu), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);
	
	if(!pd) 
		memcpy(&npd->data, &default_tmu_data, sizeof(struct tmu_data));
	else 
		memcpy(&npd->data, pd, sizeof(struct tmu_data));
	
	platform_set_drvdata(&s5p_device_tmu, npd);
}


