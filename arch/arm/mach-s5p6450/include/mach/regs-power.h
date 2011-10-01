/* arch/arm/plat-s3c64xx/include/plat/regs-syscon-power.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *      http://armlinux.simtec.co.uk/
 *      Ben Dooks <ben@simtec.co.uk>
 *
 * S5P6450 - syscon power and sleep control registers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __PLAT_S5P6450_REGS_SYSCON_POWER_H
#define __PLAT_S5P6450_REGS_SYSCON_POWER_H __FILE__

#define S5P_PWRREG(x)       (S3C_VA_SYS + (x))

#define S5P6450_PWR_CFG				S5P_PWRREG(0x804)
#define S5P6450_EINT_MASK			S5P_PWRREG(0x808)
#define S5P6450_STOP_CFG			S5P_PWRREG(0x814)
#define S5P6450_SLEEP_CFG			S5P_PWRREG(0x818)
#define S5P6450_OSC_FREQ			S5P_PWRREG(0x820)
#define S5P6450_OSC_STABLE			S5P_PWRREG(0x824)
#define S5P6450_PWR_STABLE			S5P_PWRREG(0x828)
#define S5P6450_RST_STAT			S5P_PWRREG(0x904)
#define S5P6450_WAKEUP_STAT			S5P_PWRREG(0x908)

#define S5P6450_PWRCFG_MMC3_DISABLE		(1 << 18)
#define S5P6450_PWRCFG_OSC_OTG_DISABLE	(1 << 17)
#define S5P6450_PWRCFG_MMC1_DISABLE		(1 << 15)
#define S5P6450_PWRCFG_MMC0_DISABLE		(1 << 14)
#define S5P6450_PWRCFG_TS_DISABLE		(1 << 12)
#define S5P6450_PWRCFG_RTC_TICK_DISABLE		(1 << 11)
#define S5P6450_PWRCFG_RTC_ALARM_DISABLE	(1 << 10)

#define S5P6450_PWRCFG_CFG_WFI_MASK		(0x3 << 5)
#define S5P6450_PWRCFG_CFG_WFI_IGNORE	(0x0 << 5)
#define S5P6450_PWRCFG_CFG_WFI_IDLE		(0x1 << 5)
#define S5P6450_PWRCFG_CFG_WFI_STOP		(0x2 << 5)
#define S5P6450_PWRCFG_CFG_WFI_SLEEP	(0x3 << 5)

#define S5P6450_STOPCFG_ARM_LOGIC_ON	(1 << 17)
#define S5P6450_STOPCFG_OSC_EN			(1 << 0)

#define S5P6450_SLEEPCFG_OSC_EN			(1 << 0)

#define S5P6450_WAKEUPSTAT_MMC3			(1 << 13)
#define S5P6450_WAKEUPSTAT_MMC1			(1 << 10)
#define S5P6450_WAKEUPSTAT_MMC0			(1 << 9)
#define S5P6450_WAKEUPSTAT_TS			(1 << 3)
#define S5P6450_WAKEUPSTAT_RTC_TICK		(1 << 2)
#define S5P6450_WAKEUPSTAT_RTC_ALARM	(1 << 1)
#define S5P6450_WAKEUPSTAT_EINT			(1 << 0)

#define S5P6450_INFORM0				S5P_PWRREG(0xA00)
#define S5P6450_INFORM1				S5P_PWRREG(0xA04)
#define S5P6450_INFORM2				S5P_PWRREG(0xA08)
#define S5P6450_INFORM3				S5P_PWRREG(0xA0C)
#define S5P6450_INFORM4				S5P_PWRREG(0xA10)
#define S5P6450_INFORM7				S5P_PWRREG(0xA1C)

#endif /* __PLAT_S5P6450_REGS_SYSCON_POWER_H */
