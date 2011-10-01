/* linux/arch/arm/mach-s5pv310/include/mach/pm-core.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Based on arch/arm/mach-s3c2410/include/mach/pm-core.h,
 * Copyright 2008 Simtec Electronics
 *      Ben Dooks <ben@simtec.co.uk>
 *      http://armlinux.simtec.co.uk/
 *
 * S5PV310 - PM core support for arch/arm/plat-s5p/pm.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include <mach/regs-gpio.h>

static inline void s3c_pm_debug_init_uart(void)
{
    /* nothing here yet */
}

static inline void s3c_pm_arch_prepare_irqs(void)
{
#ifdef CONFIG_CPU_S5PV310_EVT1
	/* Set the reseverd bits to 0 */
	s3c_irqwake_intmask &= ~(0xFFF << 20);
#else
	/* Add to reset the wakeup event monitoring circuit */
	__raw_writel(0x7fffffff, S5P_WAKEUP_MASK);
#endif

	__raw_writel(s3c_irqwake_eintmask, S5P_EINT_WAKEUP_MASK);
	__raw_writel(s3c_irqwake_intmask, S5P_WAKEUP_MASK);
}

static inline void s3c_pm_arch_stop_clocks(void)
{
	/* nothing here yet */
}

static inline void s3c_pm_arch_show_resume_irqs(void)
{
	pr_info("WAKEUP_STAT: 0x%x\n", __raw_readl(S5P_WAKEUP_STAT));
	pr_info("WAKUP_INT0_PEND: 0x%x\n", __raw_readl(S5P_EINT_PEND(0)));
	pr_info("WAKUP_INT1_PEND: 0x%x\n", __raw_readl(S5P_EINT_PEND(1)));
	pr_info("WAKUP_INT2_PEND: 0x%x\n", __raw_readl(S5P_EINT_PEND(2)));
	pr_info("WAKUP_INT3_PEND: 0x%x\n", __raw_readl(S5P_EINT_PEND(3)));
}

static inline void s3c_pm_arch_update_uart(void __iomem *regs,
					   struct pm_uart_save *save)
{
	/* nothing here yet */
}

extern void (*s3c_config_sleep_gpio_table)(void);
