/* linux/arch/arm/mach-s5pv310/cpufreq.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S5PV310 - CPU frequency scaling support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/suspend.h>
#include <linux/slab.h>
#include <linux/regulator/consumer.h>
#include <linux/cpufreq.h>
#include <linux/reboot.h>
#include <linux/interrupt.h>
#include <linux/irq.h>

#define CPUMON 0

#ifdef CONFIG_S5PV310_BUSFREQ
#include <linux/sysfs.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/cpu.h>
#include <linux/ktime.h>
#include <linux/tick.h>
#include <linux/kernel_stat.h>
#endif

#include <plat/pm.h>

#include <plat/s5pv310.h>

#include <mach/cpufreq.h>
#include <mach/dmc.h>
#include <mach/map.h>
#include <mach/regs-clock.h>
#include <mach/pm-core.h>

#include <mach/regs-tmu.h>

static struct clk *arm_clk;
static struct clk *moutcore;
static struct clk *mout_mpll;
static struct clk *mout_apll;
static struct clk *sclk_dmc;

#ifdef CONFIG_REGULATOR
static struct regulator *arm_regulator;
static struct regulator *int_regulator;
#endif

static struct cpufreq_freqs freqs;
static int s5pv310_dvs_locking;
static bool s5pv310_cpufreq_init_done;
static DEFINE_MUTEX(set_cpu_freq_change);
static DEFINE_MUTEX(set_cpu_freq_lock);

#undef HAVE_DAC

#ifdef HAVE_DAC
void __iomem *dac_base;
#endif

/* temperary define for additional symantics for relation */
#define DISABLE_FURTHER_CPUFREQ         0x10
#define ENABLE_FURTHER_CPUFREQ          0x20
#define MASK_FURTHER_CPUFREQ            0x30
#define MASK_ONLY_SET_CPUFREQ		0x40
#define SET_CPU_FREQ_SAMPLING_RATE      100000

#define ARMCLOCK_1200MHZ		1200000
#define ARMCLOCK_1000MHZ		1000000
#define ARMCLOCK_500MHZ			 500000

static int s5pv310_max_armclk;

enum s5pv310_memory_type{
	DDR2 = 0x4,
	LPDDR2 = 0x5,
	DDR3 = 0x6,
};

#ifdef CONFIG_CPU_S5PV310_EVT1
enum cpufreq_level_index{
	L0, L1, L2, L3, L4, CPUFREQ_LEVEL_END,
};
#else
enum cpufreq_level_index{
	L0, L1, L2, L3, CPUFREQ_LEVEL_END,
};
#endif

/* Using lookup table to support 1200MHz/1000MHz by reading chip id */
static struct cpufreq_frequency_table s5pv310_lookup_freq_table[]= {
	{L0, 1200*1000},
	{L1, 1000*1000},
	{L2, 800*1000},
	{L3, 500*1000},
	{L4, 200*1000},
	{0, CPUFREQ_TABLE_END},
};

static unsigned int clkdiv_cpu0_lookup[][7] = {
	/*
	 * Clock divider value for following
	 * { DIVCORE, DIVCOREM0, DIVCOREM1, DIVPERIPH,
	 *		DIVATB, DIVPCLK_DBG, DIVAPLL }
	 */
	/* ARM L0: 1200MHz */
	{ 0, 3, 7, 3, 4, 1, 7 },

	/* ARM L1: 1000MHz */
	{ 0, 3, 7, 3, 4, 1, 7 },

	/* ARM L2: 800MHz */
	{ 0, 3, 7, 3, 3, 1, 7 },

	/* ARM L3: 500MHz */
	{ 0, 3, 7, 3, 3, 1, 7 },

	/* ARM L4: 200MHz */
	{ 0, 1, 3, 1, 3, 1, 7 },
};

static unsigned int clkdiv_cpu1_lookup[][2] = {
	/* Clock divider value for following
	 * { DIVCOPY, DIVHPM }
	 */
	/* ARM L0: 1200MHz */
	{ 5, 0 },

	/* ARM L1: 1000MHz */
	{ 4, 0 },

	/* ARM L1: 800MHz */
	{ 3, 0 },

	/* ARM L2: 500MHz */
	{ 3, 0 },

	/* ARM L3: 200MHz */
	{ 3, 0 },
};

#ifdef CONFIG_CPU_S5PV310_EVT1
static struct cpufreq_frequency_table s5pv310_freq_table[] = {
	{L0, 1200*1000},
	{L1, 1000*1000},
	{L2, 800*1000},
	{L3, 500*1000},
#ifndef CONFIG_MACH_P6_REV02
	{L4, 200*1000},
#endif
	{0, CPUFREQ_TABLE_END},
};
#else
static struct cpufreq_frequency_table s5pv310_freq_table[] = {
	{L0, 1000*1000},
	{L1, 800*1000},
	{L2, 400*1000},
	{L3, 100*1000},
	{0, CPUFREQ_TABLE_END},
};
#endif

#ifdef CONFIG_S5PV310_BUSFREQ
#undef SYSFS_DEBUG_BUSFREQ

#define MAX_LOAD	100
#define UP_THRESHOLD_DEFAULT	23

static unsigned int up_threshold;
static struct s5pv310_dmc_ppmu_hw dmc[2];
static struct s5pv310_cpu_ppmu_hw cpu;
static unsigned int bus_utilization[2];

static unsigned int busfreq_fix;
static unsigned int fix_busfreq_level;
static unsigned int pre_fix_busfreq_level;

static unsigned int calc_bus_utilization(struct s5pv310_dmc_ppmu_hw *ppmu);
static void busfreq_target(void);

static DEFINE_MUTEX(set_bus_freq_change);
static DEFINE_MUTEX(set_bus_freq_lock);

enum busfreq_level_idx {
	LV_0,
	LV_1,
#ifndef CONFIG_MACH_P6_REV02
	LV_2,
#endif
	LV_END
};

#ifdef SYSFS_DEBUG_BUSFREQ
static unsigned int time_in_state[LV_END];
unsigned long prejiffies;
unsigned long curjiffies;
#endif

static unsigned int p_idx;

struct busfreq_table {
	unsigned int idx;
	unsigned int mem_clk;
	unsigned int volt;
};
static struct busfreq_table s5pv310_busfreq_table[] = {
	{LV_0, 400000, 1100000},
	{LV_1, 267000, 1000000},
#ifndef CONFIG_MACH_P6_REV02
	{LV_2, 133000, 1000000},
#endif
	{0, 0, 0},
};
#endif

/* This defines are for cpufreq lock */
#define CPUFREQ_MIN_LEVEL	(CPUFREQ_LEVEL_END - 1)

unsigned int g_cpufreq_lock_id;
unsigned int g_cpufreq_lock_val[DVFS_LOCK_ID_END];
unsigned int g_cpufreq_lock_level = CPUFREQ_MIN_LEVEL;

#define CPUFREQ_LIMIT_LEVEL	L0

unsigned int g_cpufreq_limit_id;
unsigned int g_cpufreq_limit_val[DVFS_LOCK_ID_END];
unsigned int g_cpufreq_limit_level = CPUFREQ_LIMIT_LEVEL;

#define BUSFREQ_MIN_LEVEL	(LV_END - 1)

unsigned int g_busfreq_lock_id;
unsigned int g_busfreq_lock_val[DVFS_LOCK_ID_END];
unsigned int g_busfreq_lock_level = BUSFREQ_MIN_LEVEL;

#ifdef CONFIG_CPU_S5PV310_EVT1
static unsigned int clkdiv_cpu0[CPUFREQ_LEVEL_END][7] = {
	/*
	 * Clock divider value for following
	 * { DIVCORE, DIVCOREM0, DIVCOREM1, DIVPERIPH,
	 *		DIVATB, DIVPCLK_DBG, DIVAPLL }
	 */
	/* ARM L0: 1200MHz */
	{ 0, 3, 7, 3, 4, 1, 7 },

	/* ARM L1: 1000MHz */
	{ 0, 3, 7, 3, 4, 1, 7 },

	/* ARM L2: 800MHz */
	{ 0, 3, 7, 3, 3, 1, 7 },

	/* ARM L3: 500MHz */
	{ 0, 3, 7, 3, 3, 1, 7 },

	/* ARM L4: 200MHz */
	{ 0, 1, 3, 1, 3, 1, 7 },
};

static unsigned int clkdiv_cpu1[CPUFREQ_LEVEL_END][2] = {
	/* Clock divider value for following
	 * { DIVCOPY, DIVHPM }
	 */
	/* ARM L0: 1200MHz */
	{ 5, 0 },

	/* ARM L1: 1000MHz */
	{ 4, 0 },

	/* ARM L1: 800MHz */
	{ 3, 0 },

	/* ARM L2: 500MHz */
	{ 3, 0 },

	/* ARM L3: 200MHz */
	{ 3, 0 },
};
#else
static unsigned int clkdiv_cpu0[CPUFREQ_LEVEL_END][7] = {
	/*
	 * Clock divider value for following
	 * { DIVCORE, DIVCOREM0, DIVCOREM1, DIVPERIPH,
	 *		DIVATB, DIVPCLK_DBG, DIVAPLL }
	 */

	/* ARM L0: 1000MHz */
	{ 0, 3, 7, 3, 3, 0, 1 },

	/* ARM L1: 800MHz */
	{ 0, 3, 7, 3, 3, 0, 1 },

	/* ARM L2: 400MHz */
	{ 0, 1, 3, 1, 3, 0, 1 },

	/* ARM L3: 100MHz */
	{ 0, 0, 1, 0, 3, 1, 1 },
};

static unsigned int clkdiv_cpu1[CPUFREQ_LEVEL_END][2] = {
	/* Clock divider value for following
	 * { DIVCOPY, DIVHPM }
	 */
	 /* ARM L0: 1000MHz */
	{ 3, 0 },

	/* ARM L1: 800MHz */
	{ 3, 0 },

	/* ARM L2: 400MHz */
	{ 3, 0 },

	/* ARM L3: 100MHz */
	{ 3, 0 },
};
#endif


#ifdef CONFIG_CPU_S5PV310_EVT1
#ifndef CONFIG_S5PV310_BUSFREQ
static unsigned int clkdiv_dmc0[CPUFREQ_LEVEL_END][8] = {
	/*
	 * Clock divider value for following
	 * { DIVACP, DIVACP_PCLK, DIVDPHY, DIVDMC, DIVDMCD
	 *		DIVDMCP, DIVCOPY2, DIVCORE_TIMERS }
	 */

	/* DMC L0: 400MHz */
	{ 3, 2, 1, 1, 1, 1, 3, 1 },

	/* DMC L1: 400MHz */
	{ 3, 2, 1, 1, 1, 1, 3, 1 },

	/* DMC L2: 266.7MHz */
	{ 4, 1, 1, 2, 1, 1, 3, 1 },

	/* DMC L3: 133MHz */
	{ 5, 1, 1, 5, 1, 1, 3, 1 },
};


static unsigned int clkdiv_top[CPUFREQ_LEVEL_END][5] = {
	/*
	 * Clock divider value for following
	 * { DIVACLK200, DIVACLK100, DIVACLK160, DIVACLK133, DIVONENAND }
	 */

	/* ACLK200 L0: 200MHz */
	{ 3, 7, 4, 5, 1 },

	/* ACLK200 L1: 200MHz */
	{ 3, 7, 4, 5, 1 },

	/* ACLK200 L2: 160MHz */
	{ 4, 7, 5, 6, 1 },

	/* ACLK200 L3: 133MHz */
	{ 5, 7, 7, 7, 1 },
};

static unsigned int clkdiv_lr_bus[CPUFREQ_LEVEL_END][2] = {
	/*
	 * Clock divider value for following
	 * { DIVGDL/R, DIVGPL/R }
	 */

	/* ACLK_GDL/R L0: 200MHz */
	{ 3, 1 },

	/* ACLK_GDL/R L1: 200MHz */
	{ 3, 1 },

	/* ACLK_GDL/R L2: 160MHz */
	{ 4, 1 },

	/* ACLK_GDL/R L3: 133MHz */
	{ 5, 1 },
};

static unsigned int clkdiv_ip_bus[CPUFREQ_LEVEL_END][3] = {
	/*
	 * Clock divider value for following
	 * { DIV_MFC, DIV_G2D, DIV_FIMC }
	 */

	/* L0: MFC 200MHz G2D 266MHz FIMC 160MHz */
	{ 3, 2, 4 },

	/* L1: MFC 200MHz G2D 266MHz FIMC 160MHz */
	{ 3, 2, 4 },

	/* L2: MFC/G2D 160MHz FIMC 133MHz */
	/* { 4, 4, 5 },*/
	{ 3, 4, 5 },

	/* L3: MFC/G2D 133MHz FIMC 100MHz */
	/* { 5, 5, 7 },*/
	{ 3, 5, 7 },
};

#else
static unsigned int clkdiv_dmc0[LV_END][8] = {
	/*
	 * Clock divider value for following
	 * { DIVACP, DIVACP_PCLK, DIVDPHY, DIVDMC, DIVDMCD
	 *		DIVDMCP, DIVCOPY2, DIVCORE_TIMERS }
	 */

	/* DMC L0: 400MHz */
	{ 3, 2, 1, 1, 1, 1, 3, 1 },

	/* DMC L1: 266.7MHz */
	{ 4, 1, 1, 2, 1, 1, 3, 1 },

	/* DMC L2: 133MHz */
	{ 5, 1, 1, 5, 1, 1, 3, 1 },
};


static unsigned int clkdiv_top[LV_END][5] = {
	/*
	 * Clock divider value for following
	 * { DIVACLK200, DIVACLK100, DIVACLK160, DIVACLK133, DIVONENAND }
	 */

	/* ACLK200 L1: 200MHz */
	{ 3, 7, 4, 5, 1 },

	/* ACLK200 L2: 160MHz */
	{ 4, 7, 5, 6, 1 },

	/* ACLK200 L3: 133MHz */
	{ 5, 7, 7, 7, 1 },
};

static unsigned int clkdiv_lr_bus[LV_END][2] = {
	/*
	 * Clock divider value for following
	 * { DIVGDL/R, DIVGPL/R }
	 */

	/* ACLK_GDL/R L1: 200MHz */
	{ 3, 1 },

	/* ACLK_GDL/R L2: 160MHz */
	{ 4, 1 },

	/* ACLK_GDL/R L3: 133MHz */
	{ 5, 1 },
};

static unsigned int clkdiv_ip_bus[LV_END][3] = {
	/*
	 * Clock divider value for following
	 * { DIV_MFC, DIV_G2D, DIV_FIMC }
	 */

	/* L0: MFC 200MHz G2D 266MHz FIMC 160MHz */
	{ 3, 2, 4 },

	/* L1: MFC/G2D 160MHz FIMC 133MHz */
	/* { 4, 4, 5 },*/
	{ 3, 4, 5 },

	/* L2: MFC/G2D 133MHz FIMC 100MHz */
	/* { 5, 5, 7 },*/
	{ 3, 5, 7 },
};

#endif
#else
#ifndef CONFIG_S5PV310_BUSFREQ
static unsigned int clkdiv_dmc0[CPUFREQ_LEVEL_END][8] = {
	/*
	 * Clock divider value for following
	 * { DIVACP, DIVACP_PCLK, DIVDPHY, DIVDMC, DIVDMCD
	 *		DIVDMCP, DIVCOPY2, DIVCORE_TIMERS }
	 */

	/* DMC L0: 400MHz */
	{ 3, 2, 1, 1, 1, 1, 3, 1 },

	/* DMC L1: 400MHz */
	{ 3, 2, 1, 1, 1, 1, 3, 1 },

	/* DMC L2: 266.7MHz */
	{ 7, 1, 1, 2, 1, 1, 3, 1 },

	/* DMC L3: 200MHz */
	{ 7, 1, 1, 3, 1, 1, 3, 1 },
};


static unsigned int clkdiv_top[CPUFREQ_LEVEL_END][5] = {
	/*
	 * Clock divider value for following
	 * { DIVACLK200, DIVACLK100, DIVACLK160, DIVACLK133, DIVONENAND }
	 */

	/* ACLK200 L0: 200MHz */
	{ 3, 7, 4, 5, 1 },

	/* ACLK200 L1: 200MHz */
	{ 3, 7, 4, 5, 1 },

	/* ACLK200 L2: 160MHz */
	{ 4, 7, 5, 7, 1 },

	/* ACLK200 L3: 133.3MHz */
	{ 5, 7, 7, 7, 1 },
};

static unsigned int clkdiv_lr_bus[CPUFREQ_LEVEL_END][2] = {
	/*
	 * Clock divider value for following
	 * { DIVGDL/R, DIVGPL/R }
	 */

	/* ACLK_GDL/R L0: 200MHz */
	{ 3, 1 },

	/* ACLK_GDL/R L1: 200MHz */
	{ 3, 1 },

	/* ACLK_GDL/R L2: 160MHz */
	{ 4, 1 },

	/* ACLK_GDL/R L3: 133.3MHz */
	{ 5, 1 },
};

static unsigned int clkdiv_ip_bus[CPUFREQ_LEVEL_END][3] = {
	/*
	 * Clock divider value for following
	 * { DIV_MFC, DIV_G2D, DIV_FIMC }
	 */

	/* L0: MFC 200MHz G2D 266MHz FIMC 160MHz */
	{ 3, 2, 4 },

	/* L1: MFC 200MHz G2D 266MHz FIMC 160MHz */
	{ 3, 2, 4 },

	/* L2: MFC/G2D 160MHz FIMC 133MHz */
	/* { 4, 4, 5 },*/
	{ 3, 4, 5 },

	/* L3: MFC/G2D 133MHz FIMC 100MHz */
	/* { 5, 5, 7 },*/
	{ 3, 5, 7 },
};

#else
static unsigned int clkdiv_dmc0[CPUFREQ_LEVEL_END][8] = {
	/*
	 * Clock divider value for following
	 * { DIVACP, DIVACP_PCLK, DIVDPHY, DIVDMC, DIVDMCD
	 *		DIVDMCP, DIVCOPY2, DIVCORE_TIMERS }
	 */

	/* DMC L0: 400MHz */
	{ 3, 2, 1, 1, 1, 1, 3, 1 },

	/* DMC L1: 266.7MHz */
	{ 7, 1, 1, 2, 1, 1, 3, 1 },

	/* DMC L2: 200MHz */
	{ 7, 1, 1, 3, 1, 1, 3, 1 },
};

static unsigned int clkdiv_top[CPUFREQ_LEVEL_END][5] = {
	/*
	 * Clock divider value for following
	 * { DIVACLK200, DIVACLK100, DIVACLK160, DIVACLK133, DIVONENAND }
	 */

	/* ACLK200 L0: 200MHz */
	{ 3, 7, 4, 5, 1 },

	/* ACLK200 L1: 160MHz */
	{ 4, 7, 5, 7, 1 },

	/* ACLK200 L2: 133.3MHz */
	{ 5, 7, 7, 7, 1 },
};

static unsigned int clkdiv_lr_bus[CPUFREQ_LEVEL_END][2] = {
	/*
	 * Clock divider value for following
	 * { DIVGDL/R, DIVGPL/R }
	 */

	/* ACLK_GDL/R L0: 200MHz */
	{ 3, 1 },

	/* ACLK_GDL/R L1: 160MHz */
	{ 4, 1 },

	/* ACLK_GDL/R L2: 133.3MHz */
	{ 5, 1 },
};

static unsigned int clkdiv_ip_bus[LV_END][3] = {
	/*
	 * Clock divider value for following
	 * { DIV_MFC, DIV_G2D, DIV_FIMC }
	 */

	/* L0: MFC 200MHz G2D 266MHz FIMC 160MHz */
	{ 3, 2, 4 },

	/* L1: MFC/G2D 160MHz FIMC 133MHz */
	/* { 4, 4, 5 },*/
	{ 3, 4, 5 },

	/* L2: MFC/G2D 133MHz FIMC 100MHz */
	/* { 5, 5, 7 }, */
	{ 3, 5, 7 },
};

#endif
#endif

struct cpufreq_voltage_table {
	unsigned int	index;		/* any */
	unsigned int	arm_volt;	/* uV */
	unsigned int	int_volt;
};

/* Using lookup table to support 1200MHz/1000MHz by reading chip id */
static struct cpufreq_voltage_table s5pv310_lookup_volt_table[] = {
	{
		.index		= L0,
		.arm_volt	= 1300000,
		.int_volt	= 1100000,
	}, {
		.index		= L1,
		.arm_volt	= 1200000,
		.int_volt	= 1100000,
	}, {
		.index		= L2,
		.arm_volt	= 1100000,
		.int_volt	= 1100000,
	}, {
		.index		= L3,
		.arm_volt	= 1000000,
		.int_volt	= 1000000,
	}, {
		.index		= L4,
		.arm_volt	= 975000,
		.int_volt	= 1000000,
	},
};

static unsigned int s5pv310_lookup_apll_pms_table[CPUFREQ_LEVEL_END] = {
	/* APLL FOUT L0: 1200MHz */
	((150<<16)|(3<<8)|(0x1)),

	/* APLL FOUT L1: 1000MHz */
	((250<<16)|(6<<8)|(0x1)),

	/* APLL FOUT L2: 800MHz */
	((200<<16)|(6<<8)|(0x1)),

	/* APLL FOUT L3: 500MHz */
	((250<<16)|(6<<8)|(0x2)),

	/* APLL FOUT L4: 200MHz */
	((200<<16)|(6<<8)|(0x3)),
};

#ifdef CONFIG_CPU_S5PV310_EVT1
#ifdef CONFIG_S5PV310_ASV
static struct cpufreq_voltage_table s5pv310_volt_table[CPUFREQ_LEVEL_END] = {
	{
		.index		= L0,
		.arm_volt	= 1300000,
		.int_volt	= 1100000,
	}, {
		.index		= L1,
		.arm_volt	= 1200000,
		.int_volt	= 1100000,
	}, {
		.index		= L2,
		.arm_volt	= 1100000,
		.int_volt	= 1100000,
	}, {
		.index		= L3,
		.arm_volt	= 1000000,
		.int_volt	= 1000000,
	}, {
		.index		= L4,
		.arm_volt	= 975000,
		.int_volt	= 1000000,
	},
};
#else
static struct cpufreq_voltage_table s5pv310_volt_table[CPUFREQ_LEVEL_END] = {
	{
		.index		= L0,
		.arm_volt	= 1300000,
		.int_volt	= 1100000,
	}, {
		.index		= L1,
		.arm_volt	= 1200000,
		.int_volt	= 1100000,
	}, {
		.index		= L2,
		.arm_volt	= 1100000,
		.int_volt	= 1100000,
	}, {
		.index		= L3,
		.arm_volt	= 1000000,
		.int_volt	= 1000000,
	}, {
		.index		= L4,
		.arm_volt	= 950000,
		.int_volt	= 1000000,
	},
};
#endif
static unsigned int s5pv310_apll_pms_table[CPUFREQ_LEVEL_END] = {
	/* APLL FOUT L0: 1200MHz */
	((150<<16)|(3<<8)|(0x1)),

	/* APLL FOUT L1: 1000MHz */
	((250<<16)|(6<<8)|(0x1)),

	/* APLL FOUT L2: 800MHz */
	((200<<16)|(6<<8)|(0x1)),

	/* APLL FOUT L3: 500MHz */
	((250<<16)|(6<<8)|(0x2)),

	/* APLL FOUT L4: 200MHz */
	((200<<16)|(6<<8)|(0x3)),
};
#else
static struct cpufreq_voltage_table s5pv310_volt_table[CPUFREQ_LEVEL_END] = {
	{
		.index		= L0,
		.arm_volt	= 1200000,
		.int_volt	= 1100000,
	}, {
		.index		= L1,
		.arm_volt	= 1100000,
		.int_volt	= 1100000,
	}, {
		.index		= L2,
		.arm_volt	= 1000000,
		.int_volt	= 1000000,
	}, {
		.index		= L3,
		.arm_volt	= 950000,
		.int_volt	= 1000000,
	},
};

static unsigned int s5pv310_apll_pms_table[CPUFREQ_LEVEL_END] = {
	/* APLL FOUT L0: 1000MHz */
	((250<<16)|(6<<8)|(0x1)),

	/* APLL FOUT L1: 800MHz */
	((200<<16)|(6<<8)|(0x1)),

	/* APLL FOUT L2 : 400MHz */
	((200<<16)|(6<<8)|(0x2)),

	/* APLL FOUT L3: 100MHz */
	((200<<16)|(6<<8)|(0x4)),
};
#endif

int s5pv310_verify_policy(struct cpufreq_policy *policy)
{
	return cpufreq_frequency_table_verify(policy, s5pv310_freq_table);
}

unsigned int s5pv310_getspeed(unsigned int cpu)
{
	unsigned long rate;

	rate = clk_get_rate(arm_clk) / 1000;

	return rate;
}

static unsigned int s5pv310_getspeed_dmc(unsigned int dmc)
{
	unsigned long rate;

	rate = clk_get_rate(sclk_dmc) / 1000;

	return rate;
}

void s5pv310_set_busfreq(unsigned int div_index)
{
	unsigned int tmp, val;

	/* Change Divider - DMC0 */
	tmp = __raw_readl(S5P_CLKDIV_DMC0);

	tmp &= ~(S5P_CLKDIV_DMC0_ACP_MASK | S5P_CLKDIV_DMC0_ACPPCLK_MASK |
		S5P_CLKDIV_DMC0_DPHY_MASK | S5P_CLKDIV_DMC0_DMC_MASK |
		S5P_CLKDIV_DMC0_DMCD_MASK | S5P_CLKDIV_DMC0_DMCP_MASK |
		S5P_CLKDIV_DMC0_COPY2_MASK | S5P_CLKDIV_DMC0_CORETI_MASK);

	tmp |= ((clkdiv_dmc0[div_index][0] << S5P_CLKDIV_DMC0_ACP_SHIFT) |
		(clkdiv_dmc0[div_index][1] << S5P_CLKDIV_DMC0_ACPPCLK_SHIFT) |
		(clkdiv_dmc0[div_index][2] << S5P_CLKDIV_DMC0_DPHY_SHIFT) |
		(clkdiv_dmc0[div_index][3] << S5P_CLKDIV_DMC0_DMC_SHIFT) |
		(clkdiv_dmc0[div_index][4] << S5P_CLKDIV_DMC0_DMCD_SHIFT) |
		(clkdiv_dmc0[div_index][5] << S5P_CLKDIV_DMC0_DMCP_SHIFT) |
		(clkdiv_dmc0[div_index][6] << S5P_CLKDIV_DMC0_COPY2_SHIFT) |
		(clkdiv_dmc0[div_index][7] << S5P_CLKDIV_DMC0_CORETI_SHIFT));

	__raw_writel(tmp, S5P_CLKDIV_DMC0);

	do {
		tmp = __raw_readl(S5P_CLKDIV_STAT_DMC0);
	} while (tmp & 0x11111111);


	/* Change Divider - TOP */
	tmp = __raw_readl(S5P_CLKDIV_TOP);

	tmp &= ~(S5P_CLKDIV_TOP_ACLK200_MASK | S5P_CLKDIV_TOP_ACLK100_MASK |
		S5P_CLKDIV_TOP_ACLK160_MASK | S5P_CLKDIV_TOP_ACLK133_MASK |
		S5P_CLKDIV_TOP_ONENAND_MASK);

	tmp |= ((clkdiv_top[div_index][0] << S5P_CLKDIV_TOP_ACLK200_SHIFT) |
		(clkdiv_top[div_index][1] << S5P_CLKDIV_TOP_ACLK100_SHIFT) |
		(clkdiv_top[div_index][2] << S5P_CLKDIV_TOP_ACLK160_SHIFT) |
		(clkdiv_top[div_index][3] << S5P_CLKDIV_TOP_ACLK133_SHIFT) |
		(clkdiv_top[div_index][4] << S5P_CLKDIV_TOP_ONENAND_SHIFT));

	__raw_writel(tmp, S5P_CLKDIV_TOP);

	do {
		tmp = __raw_readl(S5P_CLKDIV_STAT_TOP);
	} while (tmp & 0x11111);

	/* Change Divider - LEFTBUS */
	tmp = __raw_readl(S5P_CLKDIV_LEFTBUS);

	tmp &= ~(S5P_CLKDIV_BUS_GDLR_MASK | S5P_CLKDIV_BUS_GPLR_MASK);

	tmp |= ((clkdiv_lr_bus[div_index][0] << S5P_CLKDIV_BUS_GDLR_SHIFT) |
		(clkdiv_lr_bus[div_index][1] << S5P_CLKDIV_BUS_GPLR_SHIFT));

	__raw_writel(tmp, S5P_CLKDIV_LEFTBUS);

	do {
		tmp = __raw_readl(S5P_CLKDIV_STAT_LEFTBUS);
	} while (tmp & 0x11);

	/* Change Divider - RIGHTBUS */
	tmp = __raw_readl(S5P_CLKDIV_RIGHTBUS);

	tmp &= ~(S5P_CLKDIV_BUS_GDLR_MASK | S5P_CLKDIV_BUS_GPLR_MASK);

	tmp |= ((clkdiv_lr_bus[div_index][0] << S5P_CLKDIV_BUS_GDLR_SHIFT) |
		(clkdiv_lr_bus[div_index][1] << S5P_CLKDIV_BUS_GPLR_SHIFT));

	__raw_writel(tmp, S5P_CLKDIV_RIGHTBUS);

	do {
		tmp = __raw_readl(S5P_CLKDIV_STAT_RIGHTBUS);
	} while (tmp & 0x11);

#if 1
	/* Change Divider - SCLK_MFC */
	tmp = __raw_readl(S5P_CLKDIV_MFC);

	tmp &= ~S5P_CLKDIV_MFC_MASK;

	tmp |= (clkdiv_ip_bus[div_index][0] << S5P_CLKDIV_MFC_SHIFT);

	__raw_writel(tmp, S5P_CLKDIV_MFC);

	do {
		tmp = __raw_readl(S5P_CLKDIV_STAT_MFC);
	} while (tmp & 0x1);

	/* Change Divider - SCLK_G2D */
	tmp = __raw_readl(S5P_CLKDIV_IMAGE);

	tmp &= ~S5P_CLKDIV_IMAGE_MASK;

	tmp |= (clkdiv_ip_bus[div_index][1] << S5P_CLKDIV_IMAGE_SHIFT);

	__raw_writel(tmp, S5P_CLKDIV_IMAGE);

	do {
		tmp = __raw_readl(S5P_CLKDIV_STAT_IMAGE);
	} while (tmp & 0x1);

	/* Change Divider - SCLK_FIMC */
	tmp = __raw_readl(S5P_CLKDIV_CAM);

	tmp &= ~S5P_CLKDIV_CAM_MASK;

	val = clkdiv_ip_bus[div_index][2];
	tmp |= ((val << 0) | (val << 4) | (val << 8) | (val << 12));

	__raw_writel(tmp, S5P_CLKDIV_CAM);

	do {
		tmp = __raw_readl(S5P_CLKDIV_STAT_CAM);
	} while (tmp & 0x1111);
#endif
}

void s5pv310_set_clkdiv(unsigned int div_index)
{
	unsigned int tmp;

	/* Change Divider - CPU0 */
	tmp = __raw_readl(S5P_CLKDIV_CPU);

	tmp &= ~(S5P_CLKDIV_CPU0_CORE_MASK | S5P_CLKDIV_CPU0_COREM0_MASK |
		S5P_CLKDIV_CPU0_COREM1_MASK | S5P_CLKDIV_CPU0_PERIPH_MASK |
		S5P_CLKDIV_CPU0_ATB_MASK | S5P_CLKDIV_CPU0_PCLKDBG_MASK |
		S5P_CLKDIV_CPU0_APLL_MASK);

	tmp |= ((clkdiv_cpu0[div_index][0] << S5P_CLKDIV_CPU0_CORE_SHIFT) |
		(clkdiv_cpu0[div_index][1] << S5P_CLKDIV_CPU0_COREM0_SHIFT) |
		(clkdiv_cpu0[div_index][2] << S5P_CLKDIV_CPU0_COREM1_SHIFT) |
		(clkdiv_cpu0[div_index][3] << S5P_CLKDIV_CPU0_PERIPH_SHIFT) |
		(clkdiv_cpu0[div_index][4] << S5P_CLKDIV_CPU0_ATB_SHIFT) |
		(clkdiv_cpu0[div_index][5] << S5P_CLKDIV_CPU0_PCLKDBG_SHIFT) |
		(clkdiv_cpu0[div_index][6] << S5P_CLKDIV_CPU0_APLL_SHIFT));

	__raw_writel(tmp, S5P_CLKDIV_CPU);

	do {
		tmp = __raw_readl(S5P_CLKDIV_STATCPU);
	} while (tmp & 0x1111111);

	/* Change Divider - CPU1 */
	tmp = __raw_readl(S5P_CLKDIV_CPU1);

	tmp &= ~((0x7 << 4) | (0x7));

	tmp |= ((clkdiv_cpu1[div_index][0] << 4) |
		(clkdiv_cpu1[div_index][1] << 0));

	__raw_writel(tmp, S5P_CLKDIV_CPU1);

	do {
		tmp = __raw_readl(S5P_CLKDIV_STATCPU1);
	} while (tmp & 0x11);

#ifndef CONFIG_S5PV310_BUSFREQ
	s5pv310_set_busfreq(div_index);
#endif
}

void s5pv310_set_apll(unsigned int index)
{
	unsigned int tmp;
	unsigned int save_val;

	/* 1. MUX_CORE_SEL = MPLL,
	 * Reduce the CLKDIVCPU value for using MPLL */
	save_val = __raw_readl(S5P_CLKDIV_CPU);
	tmp = save_val;
	tmp &= ~S5P_CLKDIV_CPU0_CORE_MASK;
	tmp |= (((save_val & 0xf) + 1) << S5P_CLKDIV_CPU0_CORE_SHIFT);
	__raw_writel(tmp, S5P_CLKDIV_CPU);
	do {
		tmp = __raw_readl(S5P_CLKDIV_STATCPU);
	} while (tmp & 0x1);

	/* ARMCLK uses MPLL for lock time */
	clk_set_parent(moutcore, mout_mpll);

	do {
		tmp = (__raw_readl(S5P_CLKMUX_STATCPU)
			>> S5P_CLKSRC_CPU_MUXCORE_SHIFT);
		tmp &= 0x7;
	} while (tmp != 0x2);

	/* 2. Set APLL Lock time */
	__raw_writel(S5P_APLL_LOCKTIME, S5P_APLL_LOCK);

	/* 3. Change PLL PMS values */
	tmp = __raw_readl(S5P_APLL_CON0);
	tmp &= ~((0x3ff << 16) | (0x3f << 8) | (0x7 << 0));
	tmp |= s5pv310_apll_pms_table[index];
	__raw_writel(tmp, S5P_APLL_CON0);

	/* 4. wait_lock_time */
	do {
		tmp = __raw_readl(S5P_APLL_CON0);
	} while (!(tmp & (0x1 << S5P_APLLCON0_LOCKED_SHIFT)));

	/* 5. MUX_CORE_SEL = APLL */
	clk_set_parent(moutcore, mout_apll);

	do {
		tmp = __raw_readl(S5P_CLKMUX_STATCPU);
		tmp &= S5P_CLKMUX_STATCPU_MUXCORE_MASK;
	} while (tmp != (0x1 << S5P_CLKSRC_CPU_MUXCORE_SHIFT));

	/* Restore the CLKDIVCPU value for APLL */
	__raw_writel(save_val, S5P_CLKDIV_CPU);
	do {
		tmp = __raw_readl(S5P_CLKDIV_STATCPU);
	} while (tmp & 0x1);

}

void s5pv310_set_frequency(unsigned int old_index, unsigned int new_index)
{
	unsigned int tmp;
	unsigned int is_curfreq_table = 0;
	unsigned int change_s_value = 0;

	if (freqs.old == s5pv310_freq_table[old_index].frequency)
		is_curfreq_table = 1;

	if (freqs.old < freqs.new) {
#ifdef CONFIG_CPU_S5PV310_EVT1
		if (s5pv310_max_armclk == ARMCLOCK_1200MHZ) {
			/* L1/L3, L2/L4 Level change require to only change s value */
			if (is_curfreq_table &&
				(((old_index == L3) && (new_index == L1)) ||
				((old_index == L4) && (new_index == L2))))
					change_s_value = 1;

		} else {
			/* L0/L2, L1/L3 Level change require to only change s value */
			if (is_curfreq_table &&
				(((old_index == L2) && (new_index == L0)) ||
				((old_index == L3) && (new_index == L1))))
					change_s_value = 1;
		}

		if (change_s_value) {
			/* 1. Change the system clock divider values */
			s5pv310_set_clkdiv(new_index);

			/* 2. Change just s value in apll m,p,s value */
			tmp = __raw_readl(S5P_APLL_CON0);
			tmp &= ~(0x7 << 0);
			tmp |= (s5pv310_apll_pms_table[new_index] & 0x7);
			__raw_writel(tmp, S5P_APLL_CON0);

		} else {
			/* Clock Configuration Procedure */

			/* 1. Change the system clock divider values */
			s5pv310_set_clkdiv(new_index);

			/* 2. Change the apll m,p,s value */
			if (freqs.new == ARMCLOCK_500MHZ) {
				regulator_set_voltage(arm_regulator,
					s5pv310_volt_table[new_index - 1].arm_volt,
					s5pv310_volt_table[new_index - 1].arm_volt);
			}
			s5pv310_set_apll(new_index);
		}
#else
		/* The frequency change to L0 needs to change apll */
		if (is_curfreq_table &&
			(freqs.new == s5pv310_freq_table[L0].frequency)) {
			/* Clock Configuration Procedure */

			/* 1. Change the system clock divider values */
			s5pv310_set_clkdiv(new_index);

			/* 2. Change the apll m,p,s value */
			s5pv310_set_apll(new_index);
		} else {
			/* 1. Change the system clock divider values */
			s5pv310_set_clkdiv(new_index);

			/* 2. Change just s value in apll m,p,s value */
			tmp = __raw_readl(S5P_APLL_CON0);
			tmp &= ~(0x7 << 0);
			tmp |= (s5pv310_apll_pms_table[new_index] & 0x7);
			__raw_writel(tmp, S5P_APLL_CON0);
		}
#endif
	} else if (freqs.old > freqs.new) {
#ifdef CONFIG_CPU_S5PV310_EVT1
		if (s5pv310_max_armclk == ARMCLOCK_1200MHZ) {
			/* L1/L3, L2/L4 Level change require to only change s value */
			if (is_curfreq_table &&
				(((old_index == L1) && (new_index == L3)) ||
				((old_index == L2) && (new_index == L4))))
					change_s_value = 1;
		} else {
			/* L0/L2, L1/L3 Level change require to only change s value */
			if (is_curfreq_table &&
				(((old_index == L0) && (new_index == L2))
				|| ((old_index == L1) && (new_index == L3))))
					change_s_value = 1;
		}

		if (change_s_value) {
			/* 1. Change just s value in apll m,p,s value */
			tmp = __raw_readl(S5P_APLL_CON0);
			tmp &= ~(0x7 << 0);
			tmp |= (s5pv310_apll_pms_table[new_index] & 0x7);
			__raw_writel(tmp, S5P_APLL_CON0);

			/* 2. Change the system clock divider values */
			s5pv310_set_clkdiv(new_index);
		} else {
			/* Clock Configuration Procedure */

			/* 1. Change the apll m,p,s value */
			s5pv310_set_apll(new_index);

			/* 2. Change the system clock divider values */
			s5pv310_set_clkdiv(new_index);
		}
#else
		/* The frequency change from L0 needs to change apll */
		if (is_cpufreq_table &&
			(freqs.old == s5pv310_freq_table[L0].frequency)) {
			/* Clock Configuration Procedure */

			/* 1. Change the apll m,p,s value */
			if (freqs.old == ARMCLOCK_500MHZ) {
				regulator_set_voltage(arm_regulator,
					s5pv310_volt_table[new_index - 2].arm_volt,
					s5pv310_volt_table[new_index - 2].arm_volt);
			}
			s5pv310_set_apll(new_index);

			/* 2. Change the system clock divider values */
			s5pv310_set_clkdiv(new_index);
		} else {
			/* 1. Change just s value in apll m,p,s value */
			tmp = __raw_readl(S5P_APLL_CON0);
			tmp &= ~(0x7 << 0);
			tmp |= (s5pv310_apll_pms_table[new_index] & 0x7);
			__raw_writel(tmp, S5P_APLL_CON0);

			/* 2. Change the system clock divider values */
			s5pv310_set_clkdiv(new_index);
		}
#endif
	}
}

static int s5pv310_target(struct cpufreq_policy *policy,
		unsigned int target_freq,
		unsigned int relation)
{
	int ret = 0;
	unsigned int index, old_index;
	unsigned int arm_volt;
#ifndef CONFIG_S5PV310_BUSFREQ
	unsigned int int_volt;
#endif

	unsigned int check_gov = 0;

	mutex_lock(&set_cpu_freq_change);

	if ((relation & ENABLE_FURTHER_CPUFREQ) &&
		(relation & DISABLE_FURTHER_CPUFREQ)) {
		/* Invalidate both if both marked */
		relation &= ~ENABLE_FURTHER_CPUFREQ;
		relation &= ~DISABLE_FURTHER_CPUFREQ;
		printk(KERN_ERR "%s:%d denied marking \"FURTHER_CPUFREQ\""
				" as both marked.\n",
				__FILE__, __LINE__);
	}

	if (!strncmp(policy->governor->name, "ondemand", CPUFREQ_NAME_LEN)
	|| !strncmp(policy->governor->name, "conservative", CPUFREQ_NAME_LEN)) {
		check_gov = 1;
		if (relation & ENABLE_FURTHER_CPUFREQ)
			s5pv310_dvs_locking = 0;

		if (s5pv310_dvs_locking == 1)
			goto cpufreq_out;

		if (relation & DISABLE_FURTHER_CPUFREQ)
			s5pv310_dvs_locking = 1;

		relation &= ~(MASK_FURTHER_CPUFREQ | MASK_ONLY_SET_CPUFREQ);
	} else {
		if ((relation & ENABLE_FURTHER_CPUFREQ) ||
			(relation & DISABLE_FURTHER_CPUFREQ) ||
			(relation & MASK_ONLY_SET_CPUFREQ))
			goto cpufreq_out;
	}

	freqs.old = s5pv310_getspeed(policy->cpu);

	if (cpufreq_frequency_table_target(policy, s5pv310_freq_table,
				freqs.old, relation, &old_index)) {
		ret = -EINVAL;
		goto cpufreq_out;
	}

	if (cpufreq_frequency_table_target(policy, s5pv310_freq_table,
				target_freq, relation, &index)) {
		ret = -EINVAL;
		goto cpufreq_out;
	}

	if ((index > g_cpufreq_lock_level) && check_gov)
		index = g_cpufreq_lock_level;

	if ((index < g_cpufreq_limit_level) && check_gov)
		index = g_cpufreq_limit_level;

	if (s5pv310_max_armclk == ARMCLOCK_1200MHZ) {
#ifdef CONFIG_FREQ_STEP_UP_L2_L0
		/* change L2 -> L0 */
		if ((index == L0) && (old_index > L2)) {
			index = L2;
		}
#else
		/* change L2 -> L1 and change L1 -> L0 */
		if (index == L0) {
			if (old_index > L1)
				index = L1;

			if (old_index > L2)
				index = L2;
		}
#endif
	} else {
		/* Prevent from jumping to 1GHz directly */
		if ((index == L0) && (old_index > L1))
			index = L1;

		if (index > L3)
			index = L3;

		if (old_index > L3)
			old_index = L3;
	}

	freqs.new = s5pv310_freq_table[index].frequency;
	freqs.cpu = policy->cpu;

	/* If the new frequency is same with previous frequency, skip */
	if (freqs.new == freqs.old)
		goto bus_freq;

#if CPUMON
	printk(KERN_ERR "CPUMON F %d\n", freqs.new);
#endif

	/* get the voltage value */
	arm_volt = s5pv310_volt_table[index].arm_volt;
#ifndef CONFIG_S5PV310_BUSFREQ
	int_volt = s5pv310_volt_table[index].int_volt;
#endif
	cpufreq_notify_transition(&freqs, CPUFREQ_PRECHANGE);

	/* When the new frequency is higher than current frequency */
	if (freqs.new > freqs.old) {
		/* Firstly, voltage up to increase frequency */
#if defined(CONFIG_REGULATOR)
		regulator_set_voltage(arm_regulator, arm_volt, arm_volt);
#ifndef CONFIG_S5PV310_BUSFREQ
		regulator_set_voltage(int_regulator, int_volt, int_volt);
#endif
#endif
	}

	s5pv310_set_frequency(old_index, index);

	/* When the new frequency is lower than current frequency */
	if (freqs.new < freqs.old) {
		/* down the voltage after frequency change */
#if defined(CONFIG_REGULATOR)
		regulator_set_voltage(arm_regulator, arm_volt, arm_volt);
#ifndef CONFIG_S5PV310_BUSFREQ
		regulator_set_voltage(int_regulator, int_volt, int_volt);
#endif
#endif
	}

	cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);

#ifdef HAVE_DAC
	switch (index) {
	case L0:
		__raw_writeb(0xff, dac_base);
		break;
	case L1:
		__raw_writeb(0xaa, dac_base);
		break;
	case L2:
		__raw_writeb(0x55, dac_base);
		break;
	case L3:
		__raw_writeb(0x00, dac_base);
		break;
	}
#endif

bus_freq:
	mutex_unlock(&set_cpu_freq_change);
#ifdef CONFIG_S5PV310_BUSFREQ
	busfreq_target();
#endif
	return ret;

cpufreq_out:
	mutex_unlock(&set_cpu_freq_change);
	return ret;
}

static int busfreq_ppmu_init(void)
{
	unsigned int i;

	for (i = 0; i < 2; i++) {
		s5pv310_dmc_ppmu_reset(&dmc[i]);
		s5pv310_dmc_ppmu_setevent(&dmc[i], 0x6);
		s5pv310_dmc_ppmu_start(&dmc[i]);
	}

	return 0;
}

static int cpu_ppmu_init(void)
{
	s5pv310_cpu_ppmu_reset(&cpu);
	s5pv310_cpu_ppmu_setevent(&cpu, 0x5, 0);
	s5pv310_cpu_ppmu_setevent(&cpu, 0x6, 1);
	s5pv310_cpu_ppmu_start(&cpu);

	return 0;
}

#ifdef CONFIG_S5PV310_BUSFREQ
static unsigned int calc_bus_utilization(struct s5pv310_dmc_ppmu_hw *ppmu)
{
	unsigned int bus_usage;

	if (ppmu->ccnt == 0) {
		printk(KERN_DEBUG "%s: 0 value is not permitted\n", __func__);
		return MAX_LOAD;
	}

	if (!(ppmu->ccnt >> 7))
		bus_usage = (ppmu->count[0] * 100) / ppmu->ccnt;
	else
		bus_usage = ((ppmu->count[0] >> 7) * 100) / (ppmu->ccnt >> 7);

	return bus_usage;
}

static unsigned int calc_cpu_bus_utilization(struct s5pv310_cpu_ppmu_hw *cpu_ppmu)
{
	unsigned int cpu_bus_usage;

	if (cpu.ccnt == 0)
		cpu.ccnt = MAX_LOAD;

	cpu_bus_usage = (cpu.count[0] + cpu.count[1])*100 / cpu.ccnt;

	return cpu_bus_usage;
}

static unsigned int get_cpu_ppmu_load(void)
{
	unsigned int cpu_bus_load;

	s5pv310_cpu_ppmu_stop(&cpu);
	s5pv310_cpu_ppmu_update(&cpu);

	cpu_bus_load = calc_cpu_bus_utilization(&cpu);

	return cpu_bus_load;
}

static unsigned int get_ppc_load(void)
{
	int i;
	unsigned int bus_load;

	for (i = 0; i < 2; i++) {
		s5pv310_dmc_ppmu_stop(&dmc[i]);
		s5pv310_dmc_ppmu_update(&dmc[i]);
		bus_utilization[i] = calc_bus_utilization(&dmc[i]);
	}

	bus_load = max(bus_utilization[0], bus_utilization[1]);

	return bus_load;
}

static int busload_observor(struct busfreq_table *freq_table,
			unsigned int bus_load,
			unsigned int cpu_bus_load,
			unsigned int pre_idx,
			unsigned int *index)
{
	unsigned int i, target_freq, idx = 0;

	if ((freqs.new == s5pv310_freq_table[L0].frequency)) {
		*index = LV_0;
		return 0;
	}

	if (bus_load > MAX_LOAD)
		return -EINVAL;

	/*
	* Maximum bus_load of S5PV310 is about 50%.
	* Default Up Threshold is 27%.
	*/
	if (bus_load > 50) {
		printk(KERN_DEBUG "BUSLOAD is larger than 50(%d)\n", bus_load);
		bus_load = 50;
	}

	if (bus_load >= up_threshold || cpu_bus_load > 10) {
		target_freq = freq_table[0].mem_clk;
		idx = 0;
	} else if (bus_load < (up_threshold - 2)) {
		target_freq = (bus_load * freq_table[pre_idx].mem_clk) /
							(up_threshold - 2);

		if (target_freq >= freq_table[pre_idx].mem_clk) {
			for (i = 0; (freq_table[i].mem_clk != 0); i++) {
				unsigned int freq = freq_table[i].mem_clk;
				if (freq <= target_freq) {
					idx = i;
					break;
				}
			}
		} else {
			for (i = 0; (freq_table[i].mem_clk != 0); i++) {
				unsigned int freq = freq_table[i].mem_clk;
				if (freq >= target_freq) {
					idx = i;
					continue;
				}
				if (freq < target_freq)
					break;
			}
		}
	} else {
		idx = pre_idx;
	}

	if ((freqs.new == s5pv310_freq_table[L0].frequency) && (bus_load == 0))
		idx = pre_idx;

	if ((idx > LV_1) && (cpu_bus_load > 5))
		idx = LV_1;

	if (idx > g_busfreq_lock_level)
		idx = g_busfreq_lock_level;

	*index = idx;

	return 0;
}

static void busfreq_target(void)
{
	unsigned int i, index = 0, ret, voltage;
	unsigned int bus_load, cpu_bus_load;
#ifdef SYSFS_DEBUG_BUSFREQ
	unsigned long level_state_jiffies;
#endif

	mutex_lock(&set_bus_freq_change);

	if (busfreq_fix) {
		for (i = 0; i < 2; i++)
			s5pv310_dmc_ppmu_stop(&dmc[i]);

		s5pv310_cpu_ppmu_stop(&cpu);
		goto fix_out;
	}

	cpu_bus_load = get_cpu_ppmu_load();

	if (cpu_bus_load > 10) {
		if (p_idx != LV_0) {
			voltage = s5pv310_busfreq_table[LV_0].volt;
#if defined(CONFIG_REGULATOR)
			regulator_set_voltage(int_regulator, voltage, voltage);
#endif
			s5pv310_set_busfreq(LV_0);
		}
	  p_idx = LV_0;

	  goto out;
	}

	bus_load = get_ppc_load();

	/* Change bus frequency */
	ret = busload_observor(s5pv310_busfreq_table,
				bus_load, cpu_bus_load, p_idx, &index);
	if (ret < 0)
		printk(KERN_ERR "%s:fail to check load (%d)\n", __func__, ret);

#ifdef SYSFS_DEBUG_BUSFREQ
	curjiffies = jiffies;
	if (prejiffies != 0)
		level_state_jiffies = curjiffies - prejiffies;
	else
		level_state_jiffies = 0;

	prejiffies = jiffies;

	switch (p_idx) {
	case LV_0:
		time_in_state[LV_0] += level_state_jiffies;
		break;
	case LV_1:
		time_in_state[LV_1] += level_state_jiffies;
		break;
	case LV_2:
		time_in_state[LV_2] += level_state_jiffies;
		break;
	default:
		break;
	}
#endif

	if (p_idx != index) {
		voltage = s5pv310_busfreq_table[index].volt;
		if (p_idx > index) {
#if defined(CONFIG_REGULATOR)
			regulator_set_voltage(int_regulator, voltage, voltage);
#endif
		}

		s5pv310_set_busfreq(index);

		if (p_idx < index) {
#if defined(CONFIG_REGULATOR)
			regulator_set_voltage(int_regulator, voltage, voltage);
#endif
		}
		smp_mb();
		p_idx = index;
	}
out:
	busfreq_ppmu_init();
	cpu_ppmu_init();
fix_out:
	mutex_unlock(&set_bus_freq_change);

}
#endif

int s5pv310_cpufreq_lock(unsigned int nId,
			enum cpufreq_level_request cpufreq_level)
{
	int ret = 0, cpu = 0;
	unsigned int cur_freq;

	if (!s5pv310_cpufreq_init_done)
		return 0;

	if (s5pv310_max_armclk != ARMCLOCK_1200MHZ) {
		if (cpufreq_level != CPU_L0) {
			cpufreq_level -= 1;
		} else {
			printk(KERN_WARNING
				"[CPUFREQ]cpufreq lock to 1GHz in place of 1.2GHz\n");
		}
	}

	if (g_cpufreq_lock_id & (1 << nId)) {
		printk(KERN_ERR
		"[CPUFREQ]This device [%d] already locked cpufreq\n", nId);
		return 0;
	}
	mutex_lock(&set_cpu_freq_lock);
	g_cpufreq_lock_id |= (1 << nId);
	g_cpufreq_lock_val[nId] = cpufreq_level;

	/* If the requested cpufreq is higher than current min frequency */
	if (cpufreq_level < g_cpufreq_lock_level)
		g_cpufreq_lock_level = cpufreq_level;
	mutex_unlock(&set_cpu_freq_lock);

	/* If current frequency is lower than requested freq, need to update */
	cur_freq = s5pv310_getspeed(cpu);
	if (cur_freq < s5pv310_freq_table[cpufreq_level].frequency) {
		ret = cpufreq_driver_target(cpufreq_cpu_get(cpu),
				s5pv310_freq_table[cpufreq_level].frequency,
				MASK_ONLY_SET_CPUFREQ);
	}

	return ret;
}

void s5pv310_cpufreq_lock_free(unsigned int nId)
{
	unsigned int i;

	if (!s5pv310_cpufreq_init_done)
		return;

	mutex_lock(&set_cpu_freq_lock);

	g_cpufreq_lock_id &= ~(1 << nId);
	g_cpufreq_lock_val[nId] = CPUFREQ_MIN_LEVEL;
	g_cpufreq_lock_level = CPUFREQ_MIN_LEVEL;
	if (g_cpufreq_lock_id) {
		for (i = 0; i < DVFS_LOCK_ID_END; i++) {
			if (g_cpufreq_lock_val[i] < g_cpufreq_lock_level)
				g_cpufreq_lock_level = g_cpufreq_lock_val[i];
		}
	}

	mutex_unlock(&set_cpu_freq_lock);
}

int s5pv310_cpufreq_upper_limit(unsigned int nId, enum cpufreq_level_request cpufreq_level)
{
	int ret = 0, cpu = 0;
	unsigned int cur_freq;

	if (!s5pv310_cpufreq_init_done)
		return 0;

	if (s5pv310_max_armclk != ARMCLOCK_1200MHZ) {
		if (cpufreq_level != CPU_L0) {
			cpufreq_level -= 1;
		} else {
			printk(KERN_DEBUG
				"[CPUFREQ]cpufreq lock to 1GHz in place of 1.2GHz\n");
		}
	}

	if (g_cpufreq_limit_id & (1 << nId)) {
		printk(KERN_ERR "[CPUFREQ]This device [%d] already limited cpufreq\n", nId);
		return 0;
	}

	mutex_lock(&set_cpu_freq_lock);
	g_cpufreq_limit_id |= (1 << nId);
	g_cpufreq_limit_val[nId] = cpufreq_level;

	/* If the requested limit level is lower than current value */
	if (cpufreq_level > g_cpufreq_limit_level)
		g_cpufreq_limit_level = cpufreq_level;

	mutex_unlock(&set_cpu_freq_lock);

	/* If cur frequency is higher than limit freq, it needs to update */
	cur_freq = s5pv310_getspeed(cpu);
	if (cur_freq > s5pv310_freq_table[cpufreq_level].frequency) {
		ret = cpufreq_driver_target(cpufreq_cpu_get(cpu),
				s5pv310_freq_table[cpufreq_level].frequency,
				MASK_ONLY_SET_CPUFREQ);
	}

	return ret;
}

void s5pv310_cpufreq_upper_limit_free(unsigned int nId)
{
	unsigned int i;

	if (!s5pv310_cpufreq_init_done)
		return;

	mutex_lock(&set_cpu_freq_lock);
	g_cpufreq_limit_id &= ~(1 << nId);
	g_cpufreq_limit_val[nId] = CPUFREQ_LIMIT_LEVEL;
	g_cpufreq_limit_level = CPUFREQ_LIMIT_LEVEL;
	if (g_cpufreq_limit_id) {
		for (i = 0; i < DVFS_LOCK_ID_END; i++) {
			if (g_cpufreq_limit_val[i] > g_cpufreq_limit_level)
				g_cpufreq_limit_level = g_cpufreq_limit_val[i];
		}
	}
	mutex_unlock(&set_cpu_freq_lock);
}

#ifdef CONFIG_S5PV310_BUSFREQ
int s5pv310_busfreq_lock(unsigned int nId,
			enum busfreq_level_request busfreq_level)
{
	if (g_busfreq_lock_id & (1 << nId)) {
		printk(KERN_ERR
		"[BUSFREQ] This device [%d] already locked busfreq\n", nId);
		return 0;
	}
	mutex_lock(&set_bus_freq_lock);
	g_busfreq_lock_id |= (1 << nId);
	g_busfreq_lock_val[nId] = busfreq_level;

	/* If the requested cpufreq is higher than current min frequency */
	if (busfreq_level < g_busfreq_lock_level) {
		g_busfreq_lock_level = busfreq_level;
		mutex_unlock(&set_bus_freq_lock);
		busfreq_target();
	} else
		mutex_unlock(&set_bus_freq_lock);

	return 0;
}

void s5pv310_busfreq_lock_free(unsigned int nId)
{
	unsigned int i;

	mutex_lock(&set_bus_freq_lock);

	g_busfreq_lock_id &= ~(1 << nId);
	g_busfreq_lock_val[nId] = BUSFREQ_MIN_LEVEL;
	g_busfreq_lock_level = BUSFREQ_MIN_LEVEL;

	if (g_busfreq_lock_id) {
		for (i = 0; i < DVFS_LOCK_ID_END; i++) {
			if (g_busfreq_lock_val[i] < g_busfreq_lock_level)
				g_busfreq_lock_level = g_busfreq_lock_val[i];
		}
	}

	mutex_unlock(&set_bus_freq_lock);
}
#endif

#ifdef CONFIG_PM
static int s5pv310_cpufreq_suspend(struct cpufreq_policy *policy,
			pm_message_t pmsg)
{
	int ret = 0;

	return ret;
}

static int s5pv310_cpufreq_resume(struct cpufreq_policy *policy)
{
	int ret = 0;

	return ret;
}
#endif

static int s5pv310_cpufreq_notifier_event(struct notifier_block *this,
		unsigned long event, void *ptr)
{
	unsigned int cpu = 0;
	int ret = 0;

	switch (event) {
	case PM_SUSPEND_PREPARE:
		ret = cpufreq_driver_target(cpufreq_cpu_get(cpu),
		s5pv310_freq_table[L1].frequency, DISABLE_FURTHER_CPUFREQ);
		if (WARN_ON(ret < 0))
			return NOTIFY_BAD;
#ifdef CONFIG_S5PV310_BUSFREQ
		s5pv310_busfreq_lock(DVFS_LOCK_ID_PM, BUS_L0);
#endif
		printk(KERN_DEBUG "PM_SUSPEND_PREPARE for CPUFREQ\n");
		return NOTIFY_OK;
	case PM_POST_RESTORE:
	case PM_POST_SUSPEND:
		printk(KERN_DEBUG "PM_POST_SUSPEND for CPUFREQ: %d\n", ret);
		ret = cpufreq_driver_target(cpufreq_cpu_get(cpu),
		s5pv310_freq_table[L1].frequency, ENABLE_FURTHER_CPUFREQ);
#ifdef CONFIG_S5PV310_BUSFREQ
		s5pv310_busfreq_lock_free(DVFS_LOCK_ID_PM);
#endif
		return NOTIFY_OK;
	}
	return NOTIFY_DONE;
}

static struct notifier_block s5pv310_cpufreq_notifier = {
	.notifier_call = s5pv310_cpufreq_notifier_event,
};

static int s5pv310_cpufreq_reboot_notifier_call(struct notifier_block *this,
				   unsigned long code, void *_cmd)
{
	unsigned int cpu = 0;
	int ret = 0;
	struct cpufreq_policy *policy = cpufreq_cpu_get(cpu);

	if (strncmp(policy->governor->name, "powersave", CPUFREQ_NAME_LEN)) {
		ret = cpufreq_driver_target(policy,
			s5pv310_freq_table[L0].frequency, DISABLE_FURTHER_CPUFREQ);
		if (ret < 0)
			return NOTIFY_BAD;
#ifdef CONFIG_S5PV310_BUSFREQ
		s5pv310_busfreq_lock(DVFS_LOCK_ID_PM, BUS_L0);
#endif
	} else {
#if defined(CONFIG_REGULATOR)
		regulator_set_voltage(arm_regulator, s5pv310_volt_table[L0].arm_volt,
			s5pv310_volt_table[L0].arm_volt);

		regulator_set_voltage(int_regulator, s5pv310_busfreq_table[LV_0].volt,
			s5pv310_busfreq_table[LV_0].volt);
#endif
	}

	printk(KERN_INFO "C1 REBOOT Notifier for CPUFREQ\n");

	return NOTIFY_DONE;
}

static struct notifier_block s5pv310_cpufreq_reboot_notifier = {
	.notifier_call = s5pv310_cpufreq_reboot_notifier_call,
};

static int s5pv310_cpufreq_cpu_init(struct cpufreq_policy *policy)
{
	printk(KERN_DEBUG "++ %s\n", __func__);

	policy->cur = policy->min = policy->max = s5pv310_getspeed(policy->cpu);

	cpufreq_frequency_table_get_attr(s5pv310_freq_table, policy->cpu);

	/* set the transition latency value
	 */
	policy->cpuinfo.transition_latency = SET_CPU_FREQ_SAMPLING_RATE;

	/* S5PV310 multi-core processors has 2 cores
	 * that the frequency cannot be set independently.
	 * Each cpu is bound to the same speed.
	 * So the affected cpu is all of the cpus.
	 */
	if (!cpu_online(1)) {
		cpumask_copy(policy->related_cpus, cpu_possible_mask);
		cpumask_copy(policy->cpus, cpu_online_mask);
	} else {
		cpumask_setall(policy->cpus);
	}

	return cpufreq_frequency_table_cpuinfo(policy, s5pv310_freq_table);
}

static struct cpufreq_driver s5pv310_driver = {
	.flags = CPUFREQ_STICKY,
	.verify = s5pv310_verify_policy,
	.target = s5pv310_target,
	.get = s5pv310_getspeed,
	.init = s5pv310_cpufreq_cpu_init,
	.name = "s5pv310_cpufreq",
#ifdef CONFIG_PM
	.suspend = s5pv310_cpufreq_suspend,
	.resume = s5pv310_cpufreq_resume,
#endif
};

#ifdef HAVE_DAC
static int s5pv310_dac_init(void)
{
	unsigned int ret;

	printk(KERN_INFO "S5PV310 DAC Function init\n");
	s3c_gpio_cfgpin(S5PV310_GPY0(0), (0x2 << 0));

	__raw_writel(0xFFFFFFFF, S5P_SROM_BC0);

	dac_base = ioremap(S5PV310_PA_SROM0, SZ_16);

	__raw_writeb(0xff, dac_base);
}
#endif

#ifdef CONFIG_S5PV310_ASV

#include <mach/regs-iem.h>
#include <mach/asv.h>

#define IDS_OFFSET	24
#define IDS_MASK	0xFF

#define IDS_SS		4
#define IDS_A1		8
#define IDS_A2		12
#define IDS_B1		17
#define IDS_B2		27
#define IDS_C1		45
#define IDS_C2		55
#define IDS_D1		56

#define HPM_SS		8
#define HPM_A1		11
#define HPM_A2		14
#define HPM_B1		18
#define HPM_B2		21
#define HPM_C1		23
#define HPM_C2		25
#define HPM_D1		26

#define INT_LEVEL_END	3

#define LOOP_CNT	50

struct s5pv310_asv_info asv_info = {
	.asv_num = 0,
	.asv_init_done = 0
};
EXPORT_SYMBOL(asv_info);

static int iem_clock_init(void)
{
	struct clk *clk_hpm;
	struct clk *clk_copy;
	struct clk *clk_parent;
	struct clk *clk_iec;
	struct clk *clk_apc;

	/* PWI clock setting */
	clk_copy = clk_get(NULL, "sclk_pwi");
	if (IS_ERR(clk_copy)) {
		printk(KERN_ERR"ASV : SCLK_PWI clock get error\n");
		return -EINVAL;
	} else {
		clk_parent = clk_get(NULL, "xusbxti");
		if (IS_ERR(clk_parent)) {
			printk(KERN_ERR"ASV : MOUT_APLL clock get error\n");
			return -EINVAL;
		}
		clk_set_parent(clk_copy, clk_parent);

		clk_put(clk_parent);
	}

	clk_set_rate(clk_copy, 4800000);

	clk_put(clk_copy);

	/* HPM clock setting */
	clk_copy = clk_get(NULL, "dout_copy");

	if (IS_ERR(clk_copy)) {
		printk(KERN_ERR"ASV : DOUT_COPY clock get error\n");
		return -EINVAL;
	} else {
		clk_parent = clk_get(NULL, "mout_apll");
		if (IS_ERR(clk_parent)) {
			printk(KERN_ERR"ASV : MOUT_APLL clock get error\n");
			return -EINVAL;
		}
		clk_set_parent(clk_copy, clk_parent);

		clk_put(clk_parent);
	}

	clk_set_rate(clk_copy, 1000000000);

	clk_put(clk_copy);

	clk_hpm = clk_get(NULL, "sclk_hpm");
	if (IS_ERR(clk_hpm))
		return -EINVAL;

	clk_set_rate(clk_hpm, (210 * 1000 * 1000));

	clk_put(clk_hpm);

	return 0;
}

void iem_clock_set(void)
{
	/* APLL_CON0 level register */
	__raw_writel(0x80FA0601, S5P_APLL_CON0L8);
	__raw_writel(0x80C80601, S5P_APLL_CON0L7);
	__raw_writel(0x80C80602, S5P_APLL_CON0L6);
	__raw_writel(0x80C80604, S5P_APLL_CON0L5);
	__raw_writel(0x80C80601, S5P_APLL_CON0L4);
	__raw_writel(0x80C80601, S5P_APLL_CON0L3);
	__raw_writel(0x80C80601, S5P_APLL_CON0L2);
	__raw_writel(0x80C80601, S5P_APLL_CON0L1);

	/* IEM Divider register */
	__raw_writel(0x00500000, S5P_CLKDIV_IEM_L8);
	__raw_writel(0x00500000, S5P_CLKDIV_IEM_L7);
	__raw_writel(0x00500000, S5P_CLKDIV_IEM_L6);
	__raw_writel(0x00500000, S5P_CLKDIV_IEM_L5);
	__raw_writel(0x00500000, S5P_CLKDIV_IEM_L4);
	__raw_writel(0x00500000, S5P_CLKDIV_IEM_L3);
	__raw_writel(0x00500000, S5P_CLKDIV_IEM_L2);
	__raw_writel(0x00500000, S5P_CLKDIV_IEM_L1);
}
static int s5pv310_asv_init(void)
{
	unsigned int i;
	unsigned long sum_result = 0;
	unsigned int tmp;
	unsigned int hpm[LOOP_CNT];
	static void __iomem * iem_base;
	struct clk *clk_iec;
	struct clk *clk_apc;
	struct clk *clk_hpm;

	iem_base = ioremap(S5PV310_PA_IEM, (128 * 1024));
	if (iem_base == NULL) {
		printk(KERN_ERR "faile to ioremap\n");
		goto out;
	}
	/* IEC clock gate enable */
	clk_iec = clk_get(NULL, "iem-iec");
	if (IS_ERR(clk_iec)) {
		printk(KERN_ERR"ASV : IEM IEC clock get error\n");
		return -EINVAL;
	}
	clk_enable(clk_iec);

	/* APC clock gate enable */
	clk_apc = clk_get(NULL, "iem-apc");
	if (IS_ERR(clk_apc)) {
		printk(KERN_ERR"ASV : IEM APC clock get error\n");
		return -EINVAL;
	}
	clk_enable(clk_apc);

	/* hpm clock gate enalbe */
	clk_hpm = clk_get(NULL, "hpm");
	if (IS_ERR(clk_hpm)) {
		printk(KERN_ERR"ASV : HPM clock get error\n");
		return -EINVAL;
	}
	clk_enable(clk_hpm);


	if (iem_clock_init()) {
		printk(KERN_ERR "ASV driver clock_init fail\n");
		goto out;
	} else {
		/* HPM enable  */
		tmp = __raw_readl(iem_base + S5PV310_APC_CONTROL);
		tmp |= APC_HPM_EN;
		__raw_writel(tmp, (iem_base + S5PV310_APC_CONTROL));

		iem_clock_set();

		/* IEM enable */
		tmp = __raw_readl(iem_base + S5PV310_IECDPCCR);
		tmp |= IEC_EN;
		__raw_writel(tmp, (iem_base + S5PV310_IECDPCCR));
	}

	for (i = 0; i < LOOP_CNT; i++) {
		tmp = __raw_readb(iem_base + S5PV310_APC_DBG_DLYCODE);
		sum_result += tmp;
		hpm[i] = tmp;
	}

	for (i = 0; i < LOOP_CNT; i++)
		printk(KERN_INFO "ASV : hpm[%d] = %d value\n", i, hpm[i]);

	sum_result /= LOOP_CNT;
	printk(KERN_INFO "ASV : sum average value : %d\n", sum_result);
	sum_result -= 1;
	printk(KERN_INFO "ASV : hpm value %d\n", sum_result);

	/* hpm clock gate disable */
	clk_disable(clk_hpm);
	clk_put(clk_hpm);

	/* IEC clock gate disable */
	clk_disable(clk_iec);
	clk_put(clk_iec);

	/* APC clock gate disable */
	clk_disable(clk_apc);
	clk_put(clk_apc);

	iounmap(iem_base);

	return sum_result;

out:
	return -EINVAL;
}
static int s5pv310_asv_table_update(void)
{
	unsigned int i;
	unsigned int tmp;
	unsigned int ids_arm, hpm_code;
	unsigned int hpm_group = 0xff, ids_group = 0xff;
	unsigned int asv_group;
	struct clk *clk_chipid;
	unsigned int last_level = 0;

	/* chip id clock gate enable*/
	clk_chipid = clk_get(NULL, "chipid");
	if (IS_ERR(clk_chipid)) {
		printk(KERN_ERR "ASV : chipid clock get error\n");
		return -EINVAL;
	}
	clk_enable(clk_chipid);

	tmp = __raw_readl(S5P_VA_CHIPID + 0x4);

	/* get the ids_arm */
	ids_arm = ((tmp >> IDS_OFFSET) & IDS_MASK);
	if (!ids_arm) {
		printk(KERN_ERR "S5PV310 : Cannot read IDS\n");
		return -EINVAL;
	}

	/* ids grouping */
	if ((ids_arm > 0) && (ids_arm <= IDS_SS))
		ids_group = 0;
	else if ((ids_arm > IDS_SS) && (ids_arm <= IDS_A1))
		ids_group = 1;
	else if ((ids_arm > IDS_A1) && (ids_arm <= IDS_A2))
		ids_group = 2;
	else if ((ids_arm > IDS_A2) && (ids_arm <= IDS_B1))
		ids_group = 3;
	else if ((ids_arm > IDS_B1) && (ids_arm <= IDS_B2))
		ids_group = 4;
	else if ((ids_arm > IDS_B2) && (ids_arm <= IDS_C1))
		ids_group = 5;
	else if ((ids_arm > IDS_C1) && (ids_arm <= IDS_C2))
		ids_group = 6;
	else if (ids_arm >= IDS_D1)
		ids_group = 7;

	/* Change Divider - CPU1 */
	tmp = __raw_readl(S5P_CLKDIV_CPU1);
	tmp &= ~((0x7 << S5P_CLKDIV_CPU1_HPM_SHIFT) |
		(0x7 << S5P_CLKDIV_CPU1_COPY_SHIFT));
	tmp |= ((0x0 << S5P_CLKDIV_CPU1_HPM_SHIFT) |
		(0x3 << S5P_CLKDIV_CPU1_COPY_SHIFT));
	__raw_writel(tmp, S5P_CLKDIV_CPU1);

	/* HPM SCLKMPLL */
	tmp = __raw_readl(S5P_CLKSRC_CPU);
	tmp &= ~(0x1 << S5P_CLKSRC_CPU_MUXHPM_SHIFT);
	tmp |= 0x1 << S5P_CLKSRC_CPU_MUXHPM_SHIFT;
	__raw_writel(tmp, S5P_CLKSRC_CPU);

	hpm_code = s5pv310_asv_init();

	/* HPM SCLKAPLL */
	tmp = __raw_readl(S5P_CLKSRC_CPU);
	tmp &= ~(0x1 << S5P_CLKSRC_CPU_MUXHPM_SHIFT);
	tmp |= 0x0 << S5P_CLKSRC_CPU_MUXHPM_SHIFT;
	__raw_writel(tmp, S5P_CLKSRC_CPU);

	/* hpm grouping */
	if ((hpm_code > 0) && (hpm_code <= HPM_SS))
		hpm_group = 0;
	else if ((hpm_code > HPM_SS) && (hpm_code <= HPM_A1))
		hpm_group = 1;
	else if ((hpm_code > HPM_A1) && (hpm_code <= HPM_A2))
		hpm_group = 2;
	else if ((hpm_code > HPM_A2) && (hpm_code <= HPM_B1))
		hpm_group = 3;
	else if ((hpm_code > HPM_B1) && (hpm_code <= HPM_B2))
		hpm_group = 4;
	else if ((hpm_code > HPM_B2) && (hpm_code <= HPM_C1))
		hpm_group = 5;
	else if ((hpm_code > HPM_C1) && (hpm_code <= HPM_C2))
		hpm_group = 6;
	else if (hpm_code >= HPM_D1)
		hpm_group = 7;

	printk(KERN_INFO "******************************ASV *********************\n");
	printk(KERN_INFO "ASV ids_arm = %d hpm_code = %d\n", ids_arm, hpm_code);

	/* decide asv group */
	if (ids_group > hpm_group) {
		if (ids_group - hpm_group >= 3)
			asv_group = ids_group - 3;
		else
			asv_group = hpm_group;
	} else {
		if (hpm_group - ids_group >= 3)
			asv_group = hpm_group - 3;
		else
			asv_group = ids_group;
	}

	/* set asv infomation */
	asv_info.asv_num = asv_group;
	asv_info.asv_init_done = 1;

	printk(KERN_INFO "******************************ASV *********************\n");
	printk(KERN_INFO "ASV asv_info.asv_num = %d, asv_info.asv_init_done = %d\n",
		asv_info.asv_num, asv_info.asv_init_done);
	printk(KERN_INFO "ASV ids_group = %d hpm_group = %d asv_group = %d\n",
		ids_group, hpm_group, asv_group);

	if (s5pv310_max_armclk == ARMCLOCK_1200MHZ) {
		last_level = CPUFREQ_LEVEL_END - 1;
	} else {
		last_level = CPUFREQ_LEVEL_END - 2;
	}

	/* VDD_ARM level except the last level  */
	for (i = 0; i < last_level; i++) {
		switch (asv_group) {
		case 0:
			s5pv310_volt_table[i].arm_volt += (100*1000);
			break;
		case 1:
			s5pv310_volt_table[i].arm_volt += (50*1000);
			break;
		case 2:
			s5pv310_volt_table[i].arm_volt += (0*1000);
			break;
		case 3:
			s5pv310_volt_table[i].arm_volt -= (25*1000);
			break;
		case 4:
			if (s5pv310_max_armclk == ARMCLOCK_1200MHZ) {
				if (i == 3)
					s5pv310_volt_table[i].arm_volt -= (25*1000);
				else
					s5pv310_volt_table[i].arm_volt -= (50*1000);
			} else {
				if (i == 2)
					s5pv310_volt_table[i].arm_volt -= (25*1000);
				else
					s5pv310_volt_table[i].arm_volt -= (50*1000);
			}
			break;
		case 5:
			if (s5pv310_max_armclk == ARMCLOCK_1200MHZ) {
				if (i == 3)
					s5pv310_volt_table[i].arm_volt -= (25*1000);
				else
					s5pv310_volt_table[i].arm_volt -= (50*1000);
			} else {
				if (i == 2)
					s5pv310_volt_table[i].arm_volt -= (25*1000);
				else
					s5pv310_volt_table[i].arm_volt -= (50*1000);
			}
			break;
		case 6:
			s5pv310_volt_table[i].arm_volt -= (100*1000);
			break;
		case 7:
			s5pv310_volt_table[i].arm_volt -= (125*1000);
			break;
		}
		/* Maximum Voltage */
		if (s5pv310_volt_table[i].arm_volt > 1350000)
			s5pv310_volt_table[i].arm_volt = 1350000;

		/* Minimum Voltage */
		if (s5pv310_volt_table[i].arm_volt < 925000)
			s5pv310_volt_table[i].arm_volt = 925000;

		printk(KERN_INFO "ASV voltage_table[%d].arm_volt = %d\n",
				i, s5pv310_volt_table[i].arm_volt);
	}

	/* The last level of VDD_ARM */
	switch (asv_group) {
	case 0:
		s5pv310_volt_table[last_level].arm_volt += (75*1000);
		break;
	case 1:
		s5pv310_volt_table[last_level].arm_volt += (25*1000);
		break;
	case 2:
		s5pv310_volt_table[last_level].arm_volt += (0*1000);
		break;
	case 3:
	case 4:
		s5pv310_volt_table[last_level].arm_volt -= (25*1000);
		break;
	case 5:
	case 6:
	case 7:
		s5pv310_volt_table[last_level].arm_volt -= (50*1000);
		break;
	}
	printk(KERN_INFO "ASV voltage_table[%d].arm_volt = %d\n",
		last_level, s5pv310_volt_table[last_level].arm_volt);

	/* VDD_INT ASV */
	for (i = 0; i < INT_LEVEL_END; i++) {
		switch (asv_group) {
		case 0:
			s5pv310_busfreq_table[i].volt += (50*1000);
			break;
		case 1:
		case 2:
			s5pv310_busfreq_table[i].volt += (25*1000);
			break;
		case 3:
		case 4:
			s5pv310_busfreq_table[i].volt -= (0*1000);
			break;
		case 5:
		case 6:
			s5pv310_busfreq_table[i].volt -= (25*1000);
			break;
		case 7:
			s5pv310_busfreq_table[i].volt -= (50*1000);
			break;
		}

		if (s5pv310_busfreq_table[i].volt < 950000)
			s5pv310_busfreq_table[i].volt = 950000;

		printk(KERN_INFO "ASV busfreq_table[%d].volt = %d\n",
			i, s5pv310_busfreq_table[i].volt);
	}

	/* Disable chipid clock */
	clk_disable(clk_chipid);

	return 0;
}

static void s5pv310_asv_set_voltage()
{
	unsigned int asv_arm_index, asv_int_index;
	unsigned int asv_arm_volt, asv_int_volt;
	unsigned int rate;

	/* get current ARM level */
	mutex_lock(&set_cpu_freq_change);

	freqs.old = s5pv310_getspeed(0);

	switch (freqs.old) {
	case 1200000:
		asv_arm_index = 0;
	case 1000000:
		asv_arm_index = 1;
		break;
	case 800000:
		asv_arm_index = 2;
		break;
	case 500000:
		asv_arm_index = 3;
		break;
	case 200000:
		asv_arm_index = 4;
		break;
	}

	if (s5pv310_max_armclk != ARMCLOCK_1200MHZ)
		asv_arm_index -= 1;

	asv_arm_volt = s5pv310_volt_table[asv_arm_index].arm_volt;
#if defined(CONFIG_REGULATOR)
	regulator_set_voltage(arm_regulator, asv_arm_volt, asv_arm_volt);
#endif
	mutex_unlock(&set_cpu_freq_change);

	/* get current INT level */
	mutex_lock(&set_bus_freq_change);

	rate = s5pv310_getspeed_dmc(0);

	switch (rate) {
	case 400000:
		asv_int_index = 0;
		break;
	case 266666:
	case 267000:
		asv_int_index = 1;
		break;
	case 133000:
	case 133333:
		asv_int_index = 2;
		break;
	}
	asv_int_volt = s5pv310_busfreq_table[asv_int_index].volt;
#if defined(CONFIG_REGULATOR)
	regulator_set_voltage(int_regulator, asv_int_volt, asv_int_volt);
#endif
	mutex_unlock(&set_bus_freq_change);

	printk(KERN_INFO "******************************ASV *********************\n");
	printk(KERN_INFO "ASV**** arm_index %d, arm_volt %d\n",
			asv_arm_index, asv_arm_volt);
	printk(KERN_INFO "ASV**** int_index %d, int_volt %d\n",
			asv_int_index, asv_int_volt);
}

#endif

static int s5pv310_update_dvfs_table()
{
	unsigned int i, j;
	int ret = 0;

	// Get the maximum arm clock */
	s5pv310_max_armclk = s5pv310_get_max_speed();
	printk(KERN_INFO "armclk set max %d \n", s5pv310_max_armclk);

	if (s5pv310_max_armclk < 0) {
		printk(KERN_ERR "Fail to get max armclk infomatioin.\n");
		s5pv310_max_armclk  = 0; /* 1000MHz as default value */
		ret = -EINVAL;
	}

	switch (s5pv310_max_armclk) {
	case 1200000:
		printk(KERN_INFO "armclk set max 1200MHz as default@@@@@\n");
		break;

	case 0:
	case 1000000:
	default:
		s5pv310_max_armclk = ARMCLOCK_1000MHZ;
		printk(KERN_INFO "@@@@@ armclk set max 1000MHz @@@@@\n");
		/*
		 *  Prepare to dvfs table to work maximum 1000MHz
		 *
		 *  Copy freq_table, volt_table, apll_pms_table, clk_div0_table,
		 * and clk_div1_table from lists of lookup table.
		*/
		for (i = 1; i < CPUFREQ_LEVEL_END; i++) {
			s5pv310_freq_table[i-1].index = s5pv310_lookup_freq_table[i].index - 1;
			s5pv310_freq_table[i-1].frequency = s5pv310_lookup_freq_table[i].frequency;
			printk(KERN_INFO "index = %d, frequency = %d\n",
				s5pv310_freq_table[i-1].index, s5pv310_freq_table[i-1].frequency);
		}

		for (i = 1; i < CPUFREQ_LEVEL_END; i++) {
			s5pv310_volt_table[i-1].index = s5pv310_lookup_volt_table[i].index - 1;
			s5pv310_volt_table[i-1].arm_volt = s5pv310_lookup_volt_table[i].arm_volt;
			printk(KERN_INFO "index = %d, arm_volt = %d\n",
				 s5pv310_volt_table[i-1].index, s5pv310_volt_table[i-1].arm_volt);
		}

		for (i = 1; i < CPUFREQ_LEVEL_END; i++) {
			s5pv310_apll_pms_table[i-1] = s5pv310_lookup_apll_pms_table[i];
			printk(KERN_INFO "apll pms_table = 0x%08x\n", s5pv310_apll_pms_table[i-1]);
		}

		for (i = 1; i < CPUFREQ_LEVEL_END; i++) {
			for (j = 0; j < 7; j++) {
				clkdiv_cpu0[i-1][j] = clkdiv_cpu0_lookup[i][j];
				printk("%d, ", clkdiv_cpu0[i-1][j]);
			}
			printk("\n");
		}

		for (i = 1; i < CPUFREQ_LEVEL_END; i++) {
			for (j = 0; j < 2; j++) {
				clkdiv_cpu1[i-1][j] = clkdiv_cpu1_lookup[i][j];
				printk("%d, ", clkdiv_cpu1[i-1][j]);
			}
			printk("\n");
		}
		printk(KERN_INFO "@@@@@ updated dvfs table @@@@@@\n");
		break;
	}
	return ret;
}

static int __init s5pv310_cpufreq_init(void)
{
	int i;

	printk(KERN_INFO "++ %s\n", __func__);

	arm_clk = clk_get(NULL, "armclk");
	if (IS_ERR(arm_clk))
		return PTR_ERR(arm_clk);

	moutcore = clk_get(NULL, "moutcore");
	if (IS_ERR(moutcore))
		goto out;

	mout_mpll = clk_get(NULL, "mout_mpll");
	if (IS_ERR(mout_mpll))
		goto out;

	mout_apll = clk_get(NULL, "mout_apll");
	if (IS_ERR(mout_apll))
		goto out;

	sclk_dmc = clk_get(NULL, "sclk_dmc");
	if (IS_ERR(sclk_dmc))
		goto out;

#if defined(CONFIG_REGULATOR)
	arm_regulator = regulator_get(NULL, "vdd_arm");
	if (IS_ERR(arm_regulator)) {
		printk(KERN_ERR "failed to get resource %s\n", "vdd_arm");
		goto out;
	}
	int_regulator = regulator_get(NULL, "vdd_int");
	if (IS_ERR(int_regulator)) {
		printk(KERN_ERR "failed to get resource %s\n", "vdd_int");
		goto out;
	}
	s5pv310_dvs_locking = 0;
#ifdef HAVE_DAC
	s5pv310_dac_init();
#endif
#endif

#ifdef CONFIG_CPU_S5PV310_EVT1
	/*
	 * By getting chip information from pkg_id & pro_id register,
	 * adujst dvfs level, update clk divider, voltage table,
	 * apll pms value of dvfs table.
	*/
	if (s5pv310_update_dvfs_table() < 0)
		printk(KERN_INFO "arm clock limited to maximum 1000MHz.\n");
#endif

#ifdef CONFIG_S5PV310_BUSFREQ
	up_threshold = UP_THRESHOLD_DEFAULT;
	cpu.cpu_hw_base = S5PV310_VA_PPMU_CPU;
	dmc[DMC0].dmc_hw_base = S5P_VA_DMC0;
	dmc[DMC1].dmc_hw_base = S5P_VA_DMC1;

	busfreq_ppmu_init();
	cpu_ppmu_init();

	for (i = 0; i < DVFS_LOCK_ID_END; i++)
		g_busfreq_lock_val[i] = BUSFREQ_MIN_LEVEL;

#endif
	for (i = 0; i < DVFS_LOCK_ID_END; i++)
		g_cpufreq_lock_val[i] = CPUFREQ_MIN_LEVEL;

	register_pm_notifier(&s5pv310_cpufreq_notifier);
	register_reboot_notifier(&s5pv310_cpufreq_reboot_notifier);

	s5pv310_cpufreq_init_done = true;

#ifdef CONFIG_S5PV310_ASV
	asv_info.asv_init_done = 0;
	if (s5pv310_asv_table_update())
		return -EINVAL;

	s5pv310_asv_set_voltage();
#endif
	printk(KERN_INFO "-- %s\n", __func__);
	return cpufreq_register_driver(&s5pv310_driver);

out:
	if (!IS_ERR(arm_clk))
		clk_put(arm_clk);

	if (!IS_ERR(moutcore))
		clk_put(moutcore);

	if (!IS_ERR(mout_mpll))
		clk_put(mout_mpll);

	if (!IS_ERR(mout_apll))
		clk_put(mout_apll);

#ifdef CONFIG_REGULATOR
	if (!IS_ERR(arm_regulator))
		regulator_put(arm_regulator);

	if (!IS_ERR(int_regulator))
		regulator_put(int_regulator);
#endif
	printk(KERN_ERR "%s: failed initialization\n", __func__);
	return -EINVAL;
}

late_initcall(s5pv310_cpufreq_init);

static ssize_t show_busfreq_fix(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	if (!busfreq_fix)
		return sprintf(buf, "Busfreq is NOT fixed\n");
	else
		return sprintf(buf, "Busfreq is Fixed\n");
}

static ssize_t store_busfreq_fix(struct device *dev,
				struct device_attribute *attr,
				const char *buf,
				size_t count)
{
	int ret;
	ret = sscanf(buf, "%u", &busfreq_fix);

	if (ret != 1)
		return -EINVAL;

	return count;
}

static DEVICE_ATTR(busfreq_fix, 0644, show_busfreq_fix, store_busfreq_fix);

static ssize_t show_fix_busfreq_level(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	if (!busfreq_fix)
		return sprintf(buf,
		"busfreq level fix is only available in busfreq_fix state\n");
	else
		return sprintf(buf, "BusFreq Level L%u\n", fix_busfreq_level);
}

static ssize_t store_fix_busfreq_level(struct device *dev,
					struct device_attribute *attr,
					const char *buf,
					size_t count)
{
	int ret;
	if (!busfreq_fix) {
		printk(KERN_ERR
		"busfreq level fix is only avaliable in Busfreq Fix state\n");
		return count;
	} else {
		ret = sscanf(buf, "%u", &fix_busfreq_level);
		if (ret != 1)
			return -EINVAL;
		if ((fix_busfreq_level < 0) ||
				(fix_busfreq_level >= BUS_LEVEL_END)) {
			printk(KERN_INFO "Fixing Busfreq level is invalid\n");
			return count;
		}
		if (pre_fix_busfreq_level >= fix_busfreq_level)
#if defined(CONFIG_REGULATOR)
			regulator_set_voltage(int_regulator,
				s5pv310_busfreq_table[fix_busfreq_level].volt,
				s5pv310_busfreq_table[fix_busfreq_level].volt);
#endif
		s5pv310_set_busfreq(fix_busfreq_level);

		if (pre_fix_busfreq_level < fix_busfreq_level)
#if defined(CONFIG_REGULATOR)
			regulator_set_voltage(int_regulator,
				s5pv310_busfreq_table[fix_busfreq_level].volt,
				s5pv310_busfreq_table[fix_busfreq_level].volt);
#endif
		pre_fix_busfreq_level = fix_busfreq_level;

		return count;
	}
}

static DEVICE_ATTR(fix_busfreq_level, 0644, show_fix_busfreq_level,
				store_fix_busfreq_level);

#ifdef SYSFS_DEBUG_BUSFREQ
static ssize_t show_time_in_state(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	ssize_t len = 0;
	int i;

	for (i = 0; i < LV_END; i++)
		len += sprintf(buf + len, "%u: %u\n",
			s5pv310_busfreq_table[i].mem_clk, time_in_state[i]);

	return len;
}

static ssize_t store_time_in_state(struct device *dev,
				struct device_attribute *attr,
				const char *buf,
				size_t count)
{

	return count;
}

static DEVICE_ATTR(time_in_state, 0644, show_time_in_state,
				store_time_in_state);

static ssize_t show_up_threshold(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	return sprintf(buf, "%u\n", up_threshold);
}

static ssize_t store_up_threshold(struct device *dev,
				struct device_attribute *attr,
				const char *buf,
				size_t count)
{
	int ret;

	ret = sscanf(buf, "%u", &up_threshold);
	if (ret != 1)
		return -EINVAL;
	printk(KERN_ERR "** Up_Threshold is changed to %u **\n", up_threshold);
	return count;
}

static DEVICE_ATTR(up_threshold, 0644, show_up_threshold, store_up_threshold);
#endif

static int sysfs_busfreq_create(struct device *dev)
{
	int ret;

	ret = device_create_file(dev, &dev_attr_busfreq_fix);

	if (ret)
		return ret;

	ret = device_create_file(dev, &dev_attr_fix_busfreq_level);

	if (ret)
		goto failed;

#ifdef SYSFS_DEBUG_BUSFREQ
	ret = device_create_file(dev, &dev_attr_up_threshold);

	if (ret)
		goto failed_fix_busfreq_level;

	ret = device_create_file(dev, &dev_attr_time_in_state);

	if (ret)
		goto failed_up_threshold;

    return ret;

failed_up_threshold:
	device_remove_file(dev, &dev_attr_up_threshold);
failed_fix_busfreq_level:
	device_remove_file(dev, &dev_attr_fix_busfreq_level);
#else
    return ret;
#endif
failed:
	device_remove_file(dev, &dev_attr_busfreq_fix);

	return ret;
}

static struct platform_device s5pv310_busfreq_device = {
	.name	= "s5pv310-busfreq",
	.id	= -1,
};

static int __init s5pv310_busfreq_device_init(void)
{
	int ret;

	ret = platform_device_register(&s5pv310_busfreq_device);

	if (ret) {
		printk(KERN_ERR "failed at(%d)\n", __LINE__);
		return ret;
	}

	ret = sysfs_busfreq_create(&s5pv310_busfreq_device.dev);

	if (ret) {
		printk(KERN_ERR "failed at(%d)\n", __LINE__);
		goto sysfs_err;
	}

	printk(KERN_INFO "s5pv310_busfreq_device_init: %d\n", ret);

	return ret;
sysfs_err:

	platform_device_unregister(&s5pv310_busfreq_device);
	return ret;
}

late_initcall(s5pv310_busfreq_device_init);
