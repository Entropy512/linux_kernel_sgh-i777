/* linux/arch/arm/mach-s5p6450/cpu.c
 *
 * Copyright (c) 2009 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/sysdev.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>

#include <asm/proc-fns.h>

#include <mach/hardware.h>
#include <mach/map.h>
#include <asm/irq.h>

#include <plat/regs-serial.h>
#include <mach/regs-clock.h>
#include <mach/regs-power.h>

#include <plat/cpu.h>
#include <plat/devs.h>
#include <plat/clock.h>
#include <plat/adc-core.h>
#include <plat/s5p6450.h>
#include <plat/sdhci.h>

/* Initial IO mappings */

static struct map_desc s5p6450_iodesc[] __initdata = {
	{
		.virtual        = (unsigned long)S3C_VA_UART5,
		.pfn            = __phys_to_pfn(S5P_PA_UART5),
		.length         = SZ_4K,
		.type           = MT_DEVICE,
	}, {
		.virtual        = (unsigned long)S3C_VA_GPS,
		.pfn            = __phys_to_pfn(S5P6450_PA_GPS),
		.length         = SZ_4K,
		.type           = MT_DEVICE,
	}, {
		.virtual   	= (unsigned long)S3C_VA_MEM,
		.pfn        	= __phys_to_pfn(S5P6450_PA_DMC0),
		.length     	= SZ_4K,
		.type       	= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)S5P_VA_SROMC,
		.pfn		= __phys_to_pfn(S5P6450_PA_SROMC),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	},
};

static void s5p6450_idle(void)
{
	unsigned long val;

	if (!need_resched()) {
		val = __raw_readl(S5P6450_PWR_CFG);
		val &= ~(0x3<<5);
		val |= (0x1<<5);
		__raw_writel(val, S5P6450_PWR_CFG);

		cpu_do_idle();
	}
	local_irq_enable();
}

/* s5p6450_map_io
 *
 * register the standard cpu IO areas
*/

void __init s5p6450_map_io(void)
{
	iotable_init(s5p6450_iodesc, ARRAY_SIZE(s5p6450_iodesc));
	/* initialize any device information early */
#ifdef CONFIG_S3C_DEV_HSMMC
	s5p6450_default_sdhci0();
#endif
#ifdef CONFIG_S3C_DEV_HSMMC1
	s5p6450_default_sdhci1();
#endif
#ifdef CONFIG_S3C_DEV_HSMMC2
	s5p6450_default_sdhci2();
#endif
	s3c_adc_setname("s3c64xx-adc");
}

void __init s5p6450_init_clocks(int xtal)
{
	printk(KERN_DEBUG "%s: initializing clocks\n", __func__);

	s3c24xx_register_baseclocks(xtal);
	s5p_register_clocks(xtal);
	s5p6450_register_clocks();
	s5p6450_setup_clocks();
}

void __init s5p6450_init_irq(void)
{
	/* S5P6450 supports only 2 VIC */
	u32 vic[2];

	/*
	 * VIC0 is missing IRQ_VIC0[(13-15), (21-22)]
	 * VIC1 is missing IRQ VIC1[12, 14, 23]
	 */
	vic[0] = 0xff9f1fff;
	vic[1] = 0xff7fafff;

	s5p_init_irq(vic, ARRAY_SIZE(vic));
}

struct sysdev_class s5p6450_sysclass = {
	.name	= "s5p6450-core",
};

static struct sys_device s5p6450_sysdev = {
	.cls	= &s5p6450_sysclass,
};

static int __init s5p6450_core_init(void)
{
	return sysdev_class_register(&s5p6450_sysclass);
}

core_initcall(s5p6450_core_init);

int __init s5p6450_init(void)
{
	printk(KERN_INFO "S5P6450: Initializing architecture\n");

	/* set idle function */
	pm_idle = s5p6450_idle;

	return sysdev_register(&s5p6450_sysdev);
}
