/* linux/arch/arm/mach-s5pv310/include/mach/cpufreq.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __MACH_CPUFREQ_H
#define __MACH_CPUFREQ_H

/* CPUFreq for S5PV310/S5PC210 */

/* CPU frequency level index for using cpufreq lock API
 * This should be same with cpufreq_frequency_table
*/
#ifdef CONFIG_S5PV310_HI_ARMCLK_THAN_1_2GHZ
enum cpufreq_level_request{
	CPU_L0,	/* 1.4GHz */
	CPU_L1,	/* 1.2GHz */
	CPU_L2,	/* 1GHz */
	CPU_L3,	/* 800MHz */
	CPU_L4,	/* 500MHz */
	CPU_L5,	/* 200MHz */
	CPU_LEVEL_END,
};

#else
enum cpufreq_level_request{
	CPU_L0,	/* 1.2GHz */
	CPU_L1,	/* 1GHz */
	CPU_L2,	/* 800MHz */
	CPU_L3,	/* 500MHz */
	CPU_L4,	/* 200MHz */
	CPU_LEVEL_END,
};
#endif

enum busfreq_level_request{
	BUS_L0,	/* MEM 400MHz BUS 200MHz */
	BUS_L1,	/* MEM 267MHz BUS 160MHz */
	BUS_L2,	/* MEM 133MHz BUS 133MHz */
	BUS_LEVEL_END,
};

enum cpufreq_lock_ID{
	DVFS_LOCK_ID_G2D,	/* G2D */
	DVFS_LOCK_ID_MFC,	/* MFC */
	DVFS_LOCK_ID_USB,	/* USB */
	DVFS_LOCK_ID_CAM,	/* CAM */
	DVFS_LOCK_ID_APP,	/* APP */
	DVFS_LOCK_ID_PM,	/* PM */
	DVFS_LOCK_ID_TSP,	/*TSP*/
	DVFS_LOCK_ID_TMU,	/*TMU*/
	DVFS_LOCK_ID_END,
};

#ifdef CONFIG_S5PV310_HI_ARMCLK_THAN_1_2GHZ
#define CHANGE_ONLY_S_VALUE (0x1 << 0)
#define FREQ_UP (0x1 << 1)
#define FREQ_DOWN (0x1 << 2)
#define SET_VDDARM_TO_800M (0x1 << 3)
#define FREQ_DOWN_AND_CHANGE_ONLY_S_VALUE	(FREQ_DOWN | CHANGE_ONLY_S_VALUE)
#define FREQ_UP_AND_CHANGE_ONLY_S_VALUE		(FREQ_UP | CHANGE_ONLY_S_VALUE)
#define SET_VDDARM_TO_800M_AND_FREQ_DOWN	(SET_VDDARM_TO_800M | FREQ_DOWN)
#define SET_VDDARM_TO_800M_AND_FREQ_UP		(SET_VDDARM_TO_800M | FREQ_UP)
#define DVS_BEFORE_DFS (0x1 << 1)
#define DVS_AFTER_DFS (0x1 << 2)
#endif   /* CONFIG_S5PV310_HI_ARMCLK_1_2GHZ */

int s5pv310_cpufreq_lock(unsigned int nId, enum cpufreq_level_request cpufreq_level);
void s5pv310_cpufreq_lock_free(unsigned int nId);

int s5pv310_busfreq_lock(unsigned int nId, enum busfreq_level_request busfreq_level);
void s5pv310_busfreq_lock_free(unsigned int nId);

int s5pv310_cpufreq_upper_limit(unsigned int nId, enum cpufreq_level_request cpufreq_level);
void s5pv310_cpufreq_upper_limit_free(unsigned int nId);

#endif /* __MACH_CPUFREQ_H */
