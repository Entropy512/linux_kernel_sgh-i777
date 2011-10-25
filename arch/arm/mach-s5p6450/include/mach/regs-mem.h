/* linux/arch/arm/mach-s5p6450/include/mach/regs-mem.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * S5P6450 - SROMC register definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_REGS_MEM_H
#define __ASM_ARCH_REGS_MEM_H __FILE__

#include <mach/map.h>

#define S5P_MEMREG(x)		(S5P_VA_SROMC + (x))

#define S5P_SROM_BW		S5P_MEMREG(0x00)
#define S5P_SROM_BC0		S5P_MEMREG(0x04)

#define S5P_SROM_BYTE_EN(x)	(1 << (((x) * 4) + 3))
#define S5P_SROM_BYTE_DIS(x)	(0 << (((x) * 4) + 3))

#define S5P_SROM_WAIT_EN(x)	(1 << (((x) * 4) + 2))
#define S5P_SROM_WAIT_DIS(x)	(0 << (((x) * 4) + 2))

#endif /* __ASM_ARCH_REGS_MEM_H */
