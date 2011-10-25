/* linux/arch/arm/plat-s5p6450/pm.c
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * S5P6450 CPU PM support.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/suspend.h>
#include <linux/serial_core.h>
#include <linux/io.h>

#include <mach/map.h>
#include <mach/irqs.h>

#include <plat/pm.h>
#include <plat/wakeup-mask.h>
#include <plat/regs-timer.h>

#include <mach/regs-sys.h>
#include <mach/regs-gpio.h>
#include <mach/regs-clock.h>
#include <mach/regs-power.h>
#include <mach/regs-irq.h>

#ifdef CONFIG_S3C_PM_DEBUG_LED_SMDK
#include <mach/gpio-bank-n.h>

void s3c_pm_debug_smdkled(u32 set, u32 clear)
{
	unsigned long flags;
	u32 reg;

	local_irq_save(flags);
	reg = __raw_readl(S5P6450_GPNCON);
	reg &= ~(S5P6450_GPN_CONMASK(12) | S5P6450_GPN_CONMASK(13) |
		 S5P6450_GPN_CONMASK(14) | S5P6450_GPN_CONMASK(15));
	reg |= S5P6450_GPN_OUTPUT(12) | S5P6450_GPN_OUTPUT(13) |
	       S5P6450_GPN_OUTPUT(14) | S5P6450_GPN_OUTPUT(15);
	__raw_writel(reg, S5P6450_GPNCON);

	reg = __raw_readl(S5P6450_GPNDAT);
	reg &= ~(clear << 12);
	reg |= set << 12;
	__raw_writel(reg, S5P6450_GPNDAT);

	local_irq_restore(flags);
}
#endif

static struct sleep_save core_save[] = {
	SAVE_ITEM(S5P_APLL_LOCK),
	SAVE_ITEM(S5P_MPLL_LOCK),
	SAVE_ITEM(S5P_EPLL_LOCK),
	SAVE_ITEM(S5P_CLK_SRC0),
	SAVE_ITEM(S5P_CLK_SRC1),
	SAVE_ITEM(S5P_CLK_DIV0),
	SAVE_ITEM(S5P_CLK_DIV1),
	SAVE_ITEM(S5P_CLK_DIV2),
	SAVE_ITEM(S5P_CLK_OUT),
	SAVE_ITEM(S5P_CLK_GATE_HCLK0),
	SAVE_ITEM(S5P_CLK_GATE_HCLK1),
	SAVE_ITEM(S5P_CLK_GATE_PCLK),
	SAVE_ITEM(S5P_CLK_GATE_SCLK0),
	SAVE_ITEM(S5P_CLK_GATE_SCLK1),
	SAVE_ITEM(S5P_CLK_GATE_MEM0),

	SAVE_ITEM(S5P_EPLL_CON),
	SAVE_ITEM(S5P_EPLL_CON_K),

	SAVE_ITEM(S5P6450_MEM0DRVCON),
	SAVE_ITEM(S5P6450_MEM1DRVCON),

#ifndef CONFIG_CPU_FREQ
	SAVE_ITEM(S5P_APLL_CON),
	SAVE_ITEM(S5P_MPLL_CON),
#endif
	SAVE_ITEM(S3C64XX_TINT_CSTAT),
};

static struct sleep_save misc_save[] = {
	SAVE_ITEM(S5P6450_AHB_CON0),

	SAVE_ITEM(S5P6450_SPCON0),
	SAVE_ITEM(S5P6450_SPCON1),

	SAVE_ITEM(S5P6450_MEM0CONSLP0),
	SAVE_ITEM(S5P6450_MEM0CONSLP1),
};

void s3c_pm_configure_extint(void)
{
	__raw_writel(s3c_irqwake_eintmask, S5P6450_EINT_MASK);
}

void s3c_pm_restore_core(void)
{
	__raw_writel(0, S5P6450_EINT_MASK);

	s3c_pm_debug_smdkled(1 << 2, 0);

	s3c_pm_do_restore_core(core_save, ARRAY_SIZE(core_save));
	s3c_pm_do_restore(misc_save, ARRAY_SIZE(misc_save));
}

void s3c_pm_save_core(void)
{
	s3c_pm_do_save(misc_save, ARRAY_SIZE(misc_save));
	s3c_pm_do_save(core_save, ARRAY_SIZE(core_save));
}

/* since both s3c6400 and s3c6410 share the same sleep pm calls, we
 * put the per-cpu code in here until any new cpu comes along and changes
 * this.
 */

static void s5p6450_cpu_suspend(void)
{
	unsigned long tmp;

	/* set our standby method to sleep */

	/* 1.Set PWR CFG reg to sleep at WFI bit */
	tmp = __raw_readl(S5P6450_PWR_CFG);
	tmp &= ~S5P6450_PWRCFG_CFG_WFI_MASK;
	tmp |= S5P6450_PWRCFG_CFG_WFI_SLEEP;
	__raw_writel(tmp, S5P6450_PWR_CFG);

	/* 2.Set VICINTENCLEAR to disable all VIC interrupt */
	tmp = 0xFFFFFFFF;
	__raw_writel(tmp, (VA_VIC0 + VIC_INT_ENABLE_CLEAR));
	__raw_writel(tmp, (VA_VIC1 + VIC_INT_ENABLE_CLEAR));

	/* 3.Set OTHERS'31 bit to 1 to disable interrupt before SLEEP */
	tmp = __raw_readl(S5P_OTHERS);
	tmp |= S5P_OTHERS_DISABLE_INT;
	__raw_writel(tmp, S5P_OTHERS);


	/* 4.Set VICINTEN to enable all VIC interrupt */
	tmp = 0xFFFFFFFF;
	__raw_writel(tmp, (VA_VIC0 + VIC_INT_ENABLE));
	__raw_writel(tmp, (VA_VIC1 + VIC_INT_ENABLE));

	/* clear any old wakeup */

	__raw_writel(__raw_readl(S5P6450_WAKEUP_STAT),
		     S5P6450_WAKEUP_STAT);

	tmp = 0x4;
	__raw_writel(tmp, S5P6450_PWR_STABLE);

	/* set the LED state to 0110 over sleep */
	s3c_pm_debug_smdkled(3 << 1, 0xf);

	/* issue the standby signal into the pm unit. Note, we
	 * issue a write-buffer drain just in case */

	tmp = 0;

	asm("b 1f\n\t"
	    ".align 5\n\t"
	    "1:\n\t"
	    "mcr p15, 0, %0, c7, c10, 5\n\t"
	    "mcr p15, 0, %0, c7, c10, 4\n\t"
	    "mcr p15, 0, %0, c7, c0, 4" :: "r" (tmp));

	/* we should never get past here */

	panic("sleep resumed to originator?");
}

/* mapping of interrupts to parts of the wakeup mask */
static struct samsung_wakeup_mask wake_irqs[] = {
	{ .irq = IRQ_RTC_TIC,	.bit = S5P6450_PWRCFG_RTC_TICK_DISABLE, },
	{ .irq = IRQ_PENDN,		.bit = S5P6450_PWRCFG_TS_DISABLE, },
	{ .irq = IRQ_HSMMC0,	.bit = S5P6450_PWRCFG_MMC0_DISABLE, },
	{ .irq = IRQ_HSMMC1,	.bit = S5P6450_PWRCFG_MMC1_DISABLE, },
	{ .irq = IRQ_MSHC,	.bit = S5P6450_PWRCFG_MMC3_DISABLE, },
};

static void s5p6450_pm_prepare(void)
{
	samsung_sync_wakemask(S5P6450_PWR_CFG,
			      wake_irqs, ARRAY_SIZE(wake_irqs));

	/* store address of resume. */
	__raw_writel(virt_to_phys(s3c_cpu_resume), S5P6450_INFORM4);

	/* ensure previous wakeup state is cleared before sleeping */
	__raw_writel(__raw_readl(S5P6450_WAKEUP_STAT), S5P6450_WAKEUP_STAT);
}

static int s5p6450_pm_init(void)
{
	pm_cpu_prep = s5p6450_pm_prepare;
	pm_cpu_sleep = s5p6450_cpu_suspend;
	pm_uart_udivslot = 1;
	return 0;
}

arch_initcall(s5p6450_pm_init);
