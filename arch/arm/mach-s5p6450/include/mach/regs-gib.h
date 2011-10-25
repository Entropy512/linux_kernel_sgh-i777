/* linux/arch/arm/mach-s5p6450/include/mach/regs-gpio.h
 *
 * Copyright (c) 2009 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S5P6450 - GPIO register definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_REGS_GIB_H
#define __ASM_ARCH_REGS_GIB_H


#define S5P6450_GPSREG(x) (S3C_VA_GPS + x)

#define S5P6450_GPS_GPSIO    S5P6450_GPSREG(0x00)
#define S5P6450_GPS_RAWINTSOURCE   S5P6450_GPSREG(0x04)
#define S5P6450_GPS_INTMASK    S5P6450_GPSREG(0x08)

#define GPS_IN8 (0x1 << 0)
#define GPS_IN9 (0x1 << 1)
#define GPS_IN10 (0x1 << 2)
#define GPS_IN11 (0x1 << 3)

#define GPS_OUT8 (0x1 << 16)
#define GPS_OUT9 (0x1 << 17)
#define GPS_OUT10 (0x1 << 18)
#define GPS_OUT11 (0x1 << 19)


#endif /* __ASM_ARCH_REGS_GPIO_H */
