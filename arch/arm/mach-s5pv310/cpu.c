/* linux/arch/arm/mach-s5pv310/cpu.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/sched.h>
#include <linux/sysdev.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <asm/hardware/cache-l2x0.h>

#include <asm/mach/map.h>
#include <asm/mach/irq.h>

#include <asm/proc-fns.h>

#include <plat/cpu.h>
#include <plat/devs.h>
#include <plat/clock.h>
#include <plat/adc-core.h>
#include <plat/s5pv310.h>
#include <plat/sdhci.h>
#include <plat/mshci.h>
#include <plat/reset.h>
#include <plat/iic-core.h>

#include <mach/regs-irq.h>
#include <mach/regs-pmu.h>
#include <mach/irqs.h>
#include <mach/ext-gic.h>

void (*s5p_reset_hook)(void);

void __iomem *gic_cpu_base_addr;

extern int combiner_init(unsigned int combiner_nr, void __iomem *base,
			 unsigned int irq_start);
extern void combiner_cascade_irq(unsigned int combiner_nr, unsigned int irq);

#ifdef CONFIG_PM
extern int s3c_irq_wake(unsigned int irqno, unsigned int state);
#else
#define s3c_irq_wake NULL
#endif

/* Initial IO mappings */
static struct map_desc s5pv310_iodesc[] __initdata = {
	{
		.virtual	= (unsigned long)S5P_VA_COREPERI_BASE,
		.pfn		= __phys_to_pfn(S5PV310_PA_COREPERI),
		.length		= SZ_8K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)S5P_VA_EXTGIC_CPU,
		.pfn		= __phys_to_pfn(S5PV310_PA_EXTGIC_CPU),
		.length		= SZ_64K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)S5P_VA_EXTGIC_DIST,
		.pfn		= __phys_to_pfn(S5PV310_PA_EXTGIC_DIST),
		.length		= SZ_64K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)S5P_VA_EXTCOMBINER_BASE,
		.pfn		= __phys_to_pfn(S5PV310_PA_EXTCOMBINER),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)S5P_VA_COMBINER_BASE,
		.pfn		= __phys_to_pfn(S5PV310_PA_COMBINER),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)S5P_VA_L2CC,
		.pfn		= __phys_to_pfn(S5PV310_PA_L2CC),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)S5P_VA_SROMC,
		.pfn		= __phys_to_pfn(S5PV310_PA_SROMC),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)S5PV310_VA_GPIO2,
		.pfn		= __phys_to_pfn(S5PV310_PA_GPIO2),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)S5PV310_VA_GPIO3,
		.pfn		= __phys_to_pfn(S5PV310_PA_GPIO3),
		.length		= SZ_256,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)S5PV310_VA_CMU,
		.pfn		= __phys_to_pfn(S5PV310_PA_CMU),
		.length		= SZ_128K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)S5PV310_VA_PMU,
		.pfn		= __phys_to_pfn(S5PV310_PA_PMU),
		.length		= SZ_64K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)S5PV310_VA_PPMU_CPU,
		.pfn		= __phys_to_pfn(S5PV310_PA_PPMU_CPU),
		.length		= SZ_16K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)S5P_VA_AUDSS,
		.pfn		= __phys_to_pfn(S5PV310_PA_AUDSS),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)S5P_VA_SYSTIMER,
		.pfn		= __phys_to_pfn(S5PV310_PA_SYSTIMER),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual        = (unsigned long)S3C_VA_GPS,
		.pfn            = __phys_to_pfn(S5PV310_PA_GPS),
		.length         = SZ_4K,
		.type           = MT_DEVICE,
	}, {
		.virtual        = (unsigned long)S5P_VA_DMC0,
		.pfn            = __phys_to_pfn(S5PV310_PA_DMC0),
		.length         = SZ_64K,
		.type           = MT_DEVICE,
	}, {
		.virtual        = (unsigned long)S5P_VA_DMC1,
		.pfn            = __phys_to_pfn(S5PV310_PA_DMC1),
		.length         = SZ_64K,
		.type           = MT_DEVICE,
	}, {
		.virtual	= (unsigned long)S5P_VA_SYSRAM,
		.pfn		= __phys_to_pfn(S5PV310_PA_SYSRAM),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)S3C_VA_WATCHDOG,
		.pfn		= __phys_to_pfn(S3C_PA_WDT),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	},
};

static void s5pv310_idle(void)
{
	if (!need_resched())
		cpu_do_idle();

	local_irq_enable();
}

/* s5pv310_map_io
 *
 * register the standard cpu IO areas
*/
void __init s5pv310_map_io(void)
{
	iotable_init(s5pv310_iodesc, ARRAY_SIZE(s5pv310_iodesc));

	/* initialize device information early */
#ifdef CONFIG_S3C_DEV_HSMMC
	s5pv310_default_sdhci0();
#endif
#ifdef CONFIG_S3C_DEV_HSMMC1
	s5pv310_default_sdhci1();
#endif
#ifdef CONFIG_S3C_DEV_HSMMC2
	s5pv310_default_sdhci2();
#endif
#ifdef CONFIG_S3C_DEV_HSMMC3
	s5pv310_default_sdhci3();
#endif
#ifdef CONFIG_S5P_DEV_MSHC
	s5pv310_default_mshci();
#endif

	s3c_adc_setname("s3c64xx-adc");
}

void __init s5pv310_init_clocks(int xtal)
{
	printk(KERN_DEBUG "%s: initializing clocks\n", __func__);

	s3c24xx_register_baseclocks(xtal);

#ifndef CONFIG_S5PV310_FPGA
	s5p_register_clocks(xtal);
	s5pv310_register_clocks();
	s5pv310_setup_clocks();
#endif
}

void __init s5pv310_init_irq(void)
{
	int irq;

#ifdef CONFIG_USE_EXT_GIC
	gic_cpu_base_addr = S5P_VA_EXTGIC_CPU;
	gic_dist_init(0, S5P_VA_EXTGIC_DIST, IRQ_SPI(0));
	gic_cpu_init(0, S5P_VA_EXTGIC_CPU);
#else
	gic_cpu_base_addr = S5P_VA_GIC_CPU;
	gic_dist_init(0, S5P_VA_GIC_DIST, IRQ_SPI(0));
	gic_cpu_init(0, S5P_VA_GIC_CPU);
#endif

	for (irq = 0; irq < MAX_COMBINER_NR; irq++) {

#ifdef CONFIG_CPU_S5PV310_EVT1
		/* From SPI(0) to SPI(39) and SPI(51), SPI(53)
		* are connected to the interrupt combiner. These irqs
		* should be initialized to support cascade interrupt.
		*/
		if ((irq >= 40) && !(irq == 51) && !(irq == 53))
			continue;
#endif

#ifdef CONFIG_USE_EXT_GIC
		combiner_init(irq, (void __iomem *)S5P_VA_EXTCOMBINER(irq),
				COMBINER_IRQ(irq, 0));
#else
		combiner_init(irq, (void __iomem *)S5P_VA_COMBINER(irq),
				COMBINER_IRQ(irq, 0));
#endif
		combiner_cascade_irq(irq, IRQ_SPI(irq));
	}

	/* The parameters of s5p_init_irq() are for VIC init.
	 * Theses parameters should be NULL and 0 because S5PV310
	 * uses GIC instead of VIC.
	 */
	s5p_init_irq(NULL, 0);

	/* Set s3c_irq_wake as set_wake() of GIC irq_chip */
	get_irq_chip(IRQ_RTC_ALARM)->set_wake = s3c_irq_wake;
}

#ifdef CONFIG_CACHE_L2X0
int s5p_l2x0_cache_init(void)
{
	/* DATA, TAG Latency is 2cycle */
	__raw_writel(0x110, S5P_VA_L2CC + L2X0_TAG_LATENCY_CTRL);
	__raw_writel(0x110, S5P_VA_L2CC + L2X0_DATA_LATENCY_CTRL);

	/*  L2 cache Prefetch Control Register setting */
	__raw_writel(0x30000007, S5P_VA_L2CC + L2X0_PREFETCH_CTRL);

	/* Power control register setting */
	__raw_writel(L2X0_DYNAMIC_CLK_GATING_EN | L2X0_STNDBY_MODE_EN,
		     S5P_VA_L2CC + L2X0_POWER_CTRL);

	l2x0_init(S5P_VA_L2CC, 0x7C470001, 0xC200ffff);

	return 0;
}
early_initcall(s5p_l2x0_cache_init);
#endif



struct sysdev_class s5pv310_sysclass = {
	.name	= "s5pv310-core",
};

static struct sys_device s5pv310_sysdev = {
	.cls	= &s5pv310_sysclass,
};

static int __init s5pv310_core_init(void)
{
	return sysdev_class_register(&s5pv310_sysclass);
}

core_initcall(s5pv310_core_init);

static void s5pv310_sw_reset(void)
{
	int count = 3;
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

	while (count--) {
		__raw_writel(0x0, S5P_INFORM1);
		__raw_writel(0x1, S5P_SWRESET);
		mdelay(500);
	}
}

static void s5pv310_sc_feedback(void)
{
	unsigned int tmp;

	tmp = __raw_readl(S5P_ARM_CORE0_OPTION);
	tmp &= ~(1 << 0);
	tmp |= (1 << 1);
	__raw_writel(tmp, S5P_ARM_CORE0_OPTION);

	tmp = __raw_readl(S5P_ARM_CORE1_OPTION);
	tmp &= ~(1 << 0);
	tmp |= (1 << 1);
	__raw_writel(tmp, S5P_ARM_CORE1_OPTION);

	__raw_writel(0x2, S5P_ARM_COMMON_OPTION);
	__raw_writel(0x2, S5P_CAM_OPTION);
	__raw_writel(0x2, S5P_TV_OPTION);
	__raw_writel(0x2, S5P_MFC_OPTION);
	__raw_writel(0x2, S5P_G3D_OPTION);
	__raw_writel(0x2, S5P_LCD0_OPTION);
	__raw_writel(0x2, S5P_LCD1_OPTION);
	__raw_writel(0x2, S5P_GPS_OPTION);
	__raw_writel(0x2, S5P_GPS_ALIVE_OPTION);
}

int s5pv310_get_max_speed(void)
{
	static int max_speed = -1;

	if (unlikely(max_speed < 0)) {
		struct clk *chipid_clk;
		unsigned int pro_id, pkg_id;

		chipid_clk = clk_get(NULL, "chipid");
		if (chipid_clk == NULL)
		{
			printk(KERN_ERR "failed to find chipid clock source\n");
			return -EINVAL;
		}
		clk_enable(chipid_clk);

		pro_id = __raw_readl(S5P_VA_CHIPID);
		printk(KERN_INFO "pro_id: 0x%08x\n", pro_id);

		if ((pro_id & 0xf) == 0x1) {
			pkg_id = __raw_readl(S5P_VA_CHIPID + 0x4);
			printk(KERN_INFO "pkg_id: 0x%08x\n", pkg_id);

			switch (pkg_id & 0x7) {
			case 1:
				max_speed = 1600000;
				break;
			case 0:
			case 3:
				max_speed = 1000000;
				break;
			default:
				max_speed = 1000000;
				break;
			}

		} else {
			max_speed = 1000000;
		}

		clk_disable(chipid_clk);
		clk_put(chipid_clk);
	}

	return max_speed;
}

int s5pv310_subrev(void)
{
	static int subrev = -1;

	if (unlikely(subrev < 0)) {
		struct clk *chipid_clk;

		chipid_clk = clk_get(NULL, "chipid");
		BUG_ON(!chipid_clk);

		clk_enable(chipid_clk);

		subrev = readl(S5P_VA_CHIPID) & 0xf;
		pr_err("%s: %x\n", __func__, subrev);

		clk_disable(chipid_clk);
		clk_put(chipid_clk);
	}

	return subrev;
}

int __init s5pv310_init(void)
{
	printk(KERN_INFO "S5PV310: Initializing architecture\n");

	/* set idle function */
	pm_idle = s5pv310_idle;

	/* set sw_reset function */
	s5p_reset_hook = s5pv310_sw_reset;

	s5pv310_sc_feedback();

	return sysdev_register(&s5pv310_sysdev);
}
