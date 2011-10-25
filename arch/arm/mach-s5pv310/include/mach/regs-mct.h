/* arch/arm/mach-s5pv310/include/mach/regs-mct.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * S5PV310 MCT configutation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_REGS_MCT_H
#define __ASM_ARCH_REGS_MCT_H

#include <mach/map.h>

#define S5PV310_MCT_CFG			(S5P_VA_SYSTIMER + 0x00)
#define S5PV310_MCT_G_CNT_L		(S5P_VA_SYSTIMER + 0x100)
#define S5PV310_MCT_G_CNT_U		(S5P_VA_SYSTIMER + 0x104)
#define S5PV310_MCT_G_CNT_WSTAT		(S5P_VA_SYSTIMER + 0x110)
#define		G_CNT_U_STAT		(1 << 1)
#define		G_CNT_L_STAT		(1 << 0)

#define S5PV310_MCT_G_TCON		(S5P_VA_SYSTIMER + 0x240)
#define		G_TCON_START		(1 << 8)

#define S5PV310_MCT_G_INT_CSTAT		(S5P_VA_SYSTIMER + 0x244)
#define S5PV310_MCT_G_INT_ENB		(S5P_VA_SYSTIMER + 0x248)
#define S5PV310_MCT_G_WSTAT		(S5P_VA_SYSTIMER + 0x24C)
#define		G_WSTAT_TCON_STAT	(1 << 16)

#define S5PV310_MCT_L0_TCNTB		(S5P_VA_SYSTIMER + 0x300)
#define S5PV310_MCT_L0_TCNTO		(S5P_VA_SYSTIMER + 0x304)
#define S5PV310_MCT_L0_ICNTB		(S5P_VA_SYSTIMER + 0x308)
#define S5PV310_MCT_L0_ICNTO		(S5P_VA_SYSTIMER + 0x30C)
#define S5PV310_MCT_L0_FRCNTB		(S5P_VA_SYSTIMER + 0x310)
#define S5PV310_MCT_L0_FRCNTO		(S5P_VA_SYSTIMER + 0x314)
#define S5PV310_MCT_L0_TCON		(S5P_VA_SYSTIMER + 0x320)

#define S5PV310_MCT_L0_INT_CSTAT	(S5P_VA_SYSTIMER + 0x330)
#define S5PV310_MCT_L0_INT_ENB		(S5P_VA_SYSTIMER + 0x334)
#define S5PV310_MCT_L0_WSTAT		(S5P_VA_SYSTIMER + 0x340)

#define S5PV310_MCT_L1_TCNTB		(S5P_VA_SYSTIMER + 0x400)
#define S5PV310_MCT_L1_TCNTO		(S5P_VA_SYSTIMER + 0x404)
#define S5PV310_MCT_L1_ICNTB		(S5P_VA_SYSTIMER + 0x408)
#define S5PV310_MCT_L1_ICNTO		(S5P_VA_SYSTIMER + 0x40C)
#define S5PV310_MCT_L1_FRCNTB		(S5P_VA_SYSTIMER + 0x410)
#define S5PV310_MCT_L1_FRCNTO		(S5P_VA_SYSTIMER + 0x414)
#define S5PV310_MCT_L1_TCON		(S5P_VA_SYSTIMER + 0x420)
#define S5PV310_MCT_L1_INT_CSTAT	(S5P_VA_SYSTIMER + 0x430)
#define S5PV310_MCT_L1_INT_ENB		(S5P_VA_SYSTIMER + 0x434)
#define S5PV310_MCT_L1_WSTAT		(S5P_VA_SYSTIMER + 0x440)

#define		L_TCON_INTERVAL_MODE	(1 << 2)
#define		L_TCON_INT_START	(1 << 1)
#define		L_TCON_TIMER_START	(1 << 0)

#define		L_INT_ENB_CNTIE		(1 << 0)

#define		L_TCON_STAT		(1 << 3)
#define		L_ICNTB_STAT		(1 << 1)
#define		L_TCNTB_STAT		(1 << 0)

#define		L_UPDATE_ICNTB		(1 << 31)

#endif /*  __ASM_ARCH_REGS_MCT_H */

