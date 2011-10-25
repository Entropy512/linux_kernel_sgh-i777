/* arch/arm/mach-s5p6450/include/mach/regs-sys.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * S5P6450 system register definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __MACH_REGS_SYS_H
#define __MACH_REGS_SYS_H __FILE__

#define S5P_SYSREG(x)		(S3C_VA_SYS + (x))

#define S5P6450_AHB_CON0	S5P_SYSREG(0x100)
#define S5P_OTHERS		S5P_SYSREG(0x900)

/* OHTERS Register */
#define S5P_OTHERS_DISABLE_INT			(1<<31)
#define S5P_OTHERS_USB_SIG_MASK			(1<<16)
#define S5P_OTHERS_HCLK_LOW_SEL_MPLL    (1<<6)

#endif
