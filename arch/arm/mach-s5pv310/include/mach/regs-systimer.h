/* arch/arm/mach-s5pv310/include/mach/regs-systimer.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * S5PV310 System Time configutation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_REGS_SYSTIMER_H
#define __ASM_ARCH_REGS_SYSTIMER_H

#include <mach/map.h>

#define S5PV310_TCFG			(S5P_VA_SYSTIMER + (0x00))
#define S5PV310_TCON			(S5P_VA_SYSTIMER + (0x04))
#define S5PV310_TICNTB			(S5P_VA_SYSTIMER + (0x08))
#define S5PV310_TICNTO			(S5P_VA_SYSTIMER + (0x0C))
#define S5PV310_INT_CSTAT		(S5P_VA_SYSTIMER + (0x20))
#define S5PV310_FRCNTB			(S5P_VA_SYSTIMER + (0x24))
#define S5PV310_FRCNTO			(S5P_VA_SYSTIMER + (0x28))

#define S5PV310_TCFG_TICK_SWRST		(1<<16)

#define S5PV310_TCFG_CLKBASE_PCLK	(0<<12)
#define S5PV310_TCFG_CLKBASE_SYS_MAIN	(1<<12)
#define S5PV310_TCFG_CLKBASE_XRTCXTI	(2<<12)
#define S5PV310_TCFG_CLKBASE_MASK	(3<<12)
#define S5PV310_TCFG_CLKBASE_SHIFT	(12)

#define S5PV310_TCFG_PRESCALER_MASK	(255<<0)

#define S5PV310_TCFG_MUX_DIV1		(0<<8)
#define S5PV310_TCFG_MUX_MASK		(7<<8)

#define S5PV310_TCON_FRC_START		(1<<6)
#define S5PV310_TCON_TICK_INT_START	(1<<3)
#define S5PV310_TCON_TICK_START		(1<<0)

#define S5PV310_INT_FRC_EN		(1<<9)
#define S5PV310_INT_TICK_EN		(1<<8)
#define S5PV310_INT_FRC_STATUS		(1<<2)
#define S5PV310_INT_TICK_STATUS		(1<<1)
#define S5PV310_INT_EN			(1<<0)

#endif /*  __ASM_ARCH_REGS_SYSTIMER_H */

