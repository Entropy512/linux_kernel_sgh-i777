/* linux/arch/arm/mach-s5pv310/cpufreq_high.c
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

/* temperary define for additional symantics for relation */
#define DISABLE_FURTHER_CPUFREQ         0x10
#define ENABLE_FURTHER_CPUFREQ          0x20
#define MASK_FURTHER_CPUFREQ            0x30
#define MASK_ONLY_SET_CPUFREQ		0x40
#define SET_CPU_FREQ_SAMPLING_RATE      100000

static int s5pv310_max_armclk;

enum {
	DEBUG_CPUFREQ = 1U << 0,
	DEBUG_BUSFREQ = 1U << 1,
	DEBUG_ASV     = 1U << 2,
};
static int debug_mask = DEBUG_CPUFREQ;

enum s5pv310_memory_type{
	DDR2 = 0x4,
	LPDDR2 = 0x5,
	DDR3 = 0x6,
};

enum cpufreq_level_index{
	L0, L1, L2, L3, L4, L5, CPUFREQ_LEVEL_END,
};

static unsigned int freq_trans_table[CPUFREQ_LEVEL_END][CPUFREQ_LEVEL_END] = {
	/* This indicates what to do when cpufreq is changed.
	 * i.e. s-value change in apll changing.
	 *      arm voltage up in freq changing btn 500MHz and 200MHz.
	 * The line & column of below array means new & old frequency.
	 * the conents of array means types to do when frequency is changed.
	 *  @type 1 ---> changing only s-value in apll is changed.
	 *  @type 2 ---> increasing frequency
	 *  @type 4 ---> decreasing frequency
	 *  @type 8 ---> changing frequecy btn 500MMhz & 200MHz,
	 *    and temporaily set voltage @ 800MHz
	 *  The value 5 means to be set both type1 and type4.
	 *
	 * (for example)
	 * from\to 1400/1200/1000/800/500/200 (old_idex, new_index)
	 * 1400
	 * 1200
	 * 1000
	 *  800
	 *  500
	 *  200
	*/
	{ 0, 4, 4, 4, 4, 4 },
	{ 2, 0, 4, 4, 4, 4 },
	{ 2, 2, 0, 4, 5, 4 },
	{ 2, 2, 2, 0, 4, 5 },
	{ 2, 2, 3, 2, 0, 12 },
	{ 2, 2, 2, 3, 10, 0 },
};

static struct cpufreq_frequency_table s5pv310_freq_table[] = {
	{L0, 1400*1000},
	{L1, 1200*1000},
	{L2, 1000*1000},
	{L3, 800*1000},
	{L4, 500*1000},
#if !defined(CONFIG_MACH_P6_REV00) && !defined(CONFIG_MACH_P6_REV02)
	{L5, 200*1000},
#endif
	{0, CPUFREQ_TABLE_END},
};

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
#if !defined(CONFIG_MACH_P6_REV00) && !defined(CONFIG_MACH_P6_REV02)
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
#if !defined(CONFIG_MACH_P6_REV00) && !defined(CONFIG_MACH_P6_REV02)
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

static unsigned int clkdiv_cpu0[CPUFREQ_LEVEL_END][7] = {
	/*
	 * Clock divider value for following
	 * { DIVCORE, DIVCOREM0, DIVCOREM1, DIVPERIPH,
	 *		DIVATB, DIVPCLK_DBG, DIVAPLL }
	 */
	/* ARM L0: 1400MHz */
	{ 0, 3, 7, 3, 4, 1, 7 },

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
	/* ARM L0: 1400MHz */
	{ 5, 0 },

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

struct cpufreq_voltage_table {
	unsigned int	index;		/* any */
	unsigned int	arm_volt;	/* uV */
	unsigned int	int_volt;
};

#ifdef CONFIG_S5PV310_ASV

/* ASV table to work 1.4GHz in DVFS has 5 asv level. */
enum asv_group_index {
	GR_S , GR_A, GR_B, GR_C, GR_D, ASV_GROUP_END,
};

static unsigned int s5pv310_asv_cpu_volt_table[ASV_GROUP_END][CPUFREQ_LEVEL_END] = {
	{ 1350000, 1300000, 1200000, 1125000, 1050000, 1025000 },	/* SS */
	{ 1350000, 1250000, 1150000, 1075000, 1000000, 975000 },	/* A */
	{ 1300000, 1200000, 1100000, 1025000, 950000, 950000 },	/* B */
	{ 1250000, 1150000, 1050000, 975000, 950000, 950000 },	/* C */
	{ 1225000, 1125000, 1025000, 950000, 950000, 950000 },	/* D */
};

/* level 1 and 2 of vdd_int uses the same voltage value in U1 project */
static unsigned int asv_int_volt_table[ASV_GROUP_END][LV_END] = {
	{ 1150000, 1050000, 1050000 },	/* SS */
	{ 1125000, 1025000, 1025000 },	/* A */
	{ 1100000, 1000000, 1000000 },	/* B */
	{ 1075000, 975000, 975000 },	/* C */
	{ 1050000, 950000, 950000 },	/* D */
};

static struct cpufreq_voltage_table s5pv310_volt_table[CPUFREQ_LEVEL_END] = {
	{
		.index		= L0,
		.arm_volt	= 1350000, /*1400000,*/
		.int_volt	= 1100000,
	}, {
		.index		= L1,
		.arm_volt	= 1300000,
		.int_volt	= 1100000,
	}, {
		.index		= L2,
		.arm_volt	= 1200000,
		.int_volt	= 1100000,
	}, {
		.index		= L3,
		.arm_volt	= 1100000,
		.int_volt	= 1100000,
	}, {
		.index		= L4,
		.arm_volt	= 1000000,
		.int_volt	= 1000000,
	}, {
		.index		= L5,
		.arm_volt	= 975000,
		.int_volt	= 1000000,
	},
};
#else
static struct cpufreq_voltage_table s5pv310_volt_table[CPUFREQ_LEVEL_END] = {
	{
		.index		= L0,
		.arm_volt	= 1350000, /*1400000,*/
		.int_volt	= 1100000,
	}, {
		.index		= L1,
		.arm_volt	= 1300000,
		.int_volt	= 1100000,
	}, {
		.index		= L2,
		.arm_volt	= 1200000,
		.int_volt	= 1100000,
	}, {
		.index		= L3,
		.arm_volt	= 1100000,
		.int_volt	= 1100000,
	}, {
		.index		= L4,
		.arm_volt	= 1000000,
		.int_volt	= 1000000,
	}, {
		.index		= L5,
		.arm_volt	= 950000,
		.int_volt	= 1000000,
	},
};
#endif

static unsigned int s5pv310_apll_pms_table[CPUFREQ_LEVEL_END] = {
	/* APLL FOUT L0: 1400MHz */
	((350<<16)|(6<<8)|(0x1)),

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

	if (freqs.old == s5pv310_freq_table[old_index].frequency)
		is_curfreq_table = 1;

	if (freqs.old < freqs.new) {
		/* 500->1000 & 200->800 change require to only change s value */
		if (is_curfreq_table &&
			(freq_trans_table[old_index][new_index] & CHANGE_ONLY_S_VALUE)) {
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
			s5pv310_set_apll(new_index);
		}
	} else if (freqs.old > freqs.new) {
		/* 1000->500 & 800->200 change require to only change s value */
		if (is_curfreq_table &&
			(freq_trans_table[old_index][new_index] & CHANGE_ONLY_S_VALUE)) {
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
	}
}

static int s5pv310_target(struct cpufreq_policy *policy,
		unsigned int target_freq,
		unsigned int relation)
{
	int ret = 0;
	unsigned int index, old_index;
	unsigned int pos_varm, pre_varm, v_change;
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

#ifdef CONFIG_FREQ_STEP_UP_L2_L0
	/* change L2 -> L0 */
	if ((index == L0) && (old_index > L2))
		index = L2;
#else
	/* change L3 -> L0 */
	if ((index == L0) && (old_index > L3))
		index = L3;
#endif

	freqs.new = s5pv310_freq_table[index].frequency;
	freqs.cpu = policy->cpu;

	/* If the new frequency is same with previous frequency, skip */
	if (freqs.new == freqs.old)
		goto bus_freq;

#if CPUMON
	printk(KERN_ERR "CPUMON F %d\n", freqs.new);
#endif

	/* get the voltage value */
	switch (freq_trans_table[old_index][index]) {
	case FREQ_UP_AND_CHANGE_ONLY_S_VALUE:
	case FREQ_UP:
		/* When the new frequency is higher than current frequency,
		 * voltage is up.
		 */
		v_change = DVS_BEFORE_DFS;
		pre_varm = s5pv310_volt_table[index].arm_volt;
		break;

	case FREQ_DOWN_AND_CHANGE_ONLY_S_VALUE:
	case FREQ_DOWN:
		/* When the new frequency is lower than current frequency,
		 * voltage is downable.
		 */
		v_change = DVS_AFTER_DFS;
		pos_varm = s5pv310_volt_table[index].arm_volt;
		break;

	case SET_VDDARM_TO_800M_AND_FREQ_UP:
		/* voltage up in increasing frequency from 200MHz to 500MHz */
		v_change =  DVS_BEFORE_DFS | DVS_AFTER_DFS;
		pre_varm = s5pv310_volt_table[index-1].arm_volt;
		pos_varm = s5pv310_volt_table[index].arm_volt;
		break;

	case SET_VDDARM_TO_800M_AND_FREQ_DOWN:
		/*voltage up in decreaing frequency from 500 to 200 */
		v_change = DVS_BEFORE_DFS | DVS_AFTER_DFS;
		pre_varm = s5pv310_volt_table[index-2].arm_volt;
		pos_varm = s5pv310_volt_table[index].arm_volt;
		break;

	default:
		printk(KERN_WARNING "pls, check freq_trans_table!!\n");
		goto bus_freq;
	}

#ifndef CONFIG_S5PV310_BUSFREQ
	int_volt = s5pv310_volt_table[index].int_volt;
#endif
	cpufreq_notify_transition(&freqs, CPUFREQ_PRECHANGE);

	/* When the new frequency is higher than current frequency
	 * and freqency is changed btn 500MHz to 200MHz
	 */
	if (v_change & DVS_BEFORE_DFS) {
#if defined(CONFIG_REGULATOR)
		regulator_set_voltage(arm_regulator, pre_varm, pre_varm);
#ifndef CONFIG_S5PV310_BUSFREQ
		regulator_set_voltage(int_regulator, int_volt, int_volt);
#endif
#endif
	}

	s5pv310_set_frequency(old_index, index);

	/* When the new frequency is lower than current frequency
	 * and freqency is increased from 200MHz to 500MHz
	*/
	if (v_change & DVS_AFTER_DFS) {
#if defined(CONFIG_REGULATOR)
		regulator_set_voltage(arm_regulator, pos_varm, pos_varm);
#ifndef CONFIG_S5PV310_BUSFREQ
		regulator_set_voltage(int_regulator, int_volt, int_volt);
#endif
#endif
	}

	cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);

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

#ifdef CONFIG_S5PV310_BUSFREQ
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

#ifdef CONFIG_S5PV310_ASV

#include <mach/regs-iem.h>
#include <mach/asv.h>

#define IDS_OFFSET	24
#define IDS_MASK	0xFF

/* Support 1.4GHz */
#define IDS_S		8
#define IDS_A		12
#define IDS_B		32
#define IDS_C		52
#define IDS_D		53

#define HPM_S		13
#define HPM_A		17
#define HPM_B		22
#define HPM_C		26
#define HPM_D		27

#define LOOP_CNT	50

struct s5pv310_asv_info asv_info = {
	.asv_num = 0,
	.asv_init_done = 0
};
EXPORT_SYMBOL(asv_info);

static int init_iem_clock(void)
{
	struct clk *clk_hpm = NULL;
	struct clk *clk_pwi = NULL;
	struct clk *clk_pwi_parent = NULL;
	struct clk *clk_copy = NULL;
	struct clk *clk_copy_parent = NULL;

	/* PWI clock setting */
	clk_pwi = clk_get(NULL, "sclk_pwi");
	if (IS_ERR(clk_pwi)) {
		printk(KERN_ERR"ASV : SCLK_PWI clock get error\n");
		goto out;
	}
	clk_pwi_parent = clk_get(NULL, "xusbxti");
	if (IS_ERR(clk_pwi_parent)) {
		printk(KERN_ERR"ASV : MOUT_APLL clock get error\n");
		goto out;
	}

	clk_set_parent(clk_pwi, clk_pwi_parent);
	clk_put(clk_pwi_parent);

	clk_set_rate(clk_pwi, 4800000);
	clk_put(clk_pwi);

	/* HPM clock setting */
	clk_copy = clk_get(NULL, "dout_copy");
	if (IS_ERR(clk_copy)) {
		printk(KERN_ERR"ASV : DOUT_COPY clock get error\n");
		goto out;
	}
	clk_copy_parent = clk_get(NULL, "mout_apll");
	if (IS_ERR(clk_copy_parent)) {
		printk(KERN_ERR"ASV : MOUT_APLL clock get error\n");
		goto out;
	}

	clk_set_parent(clk_copy, clk_copy_parent);
	clk_put(clk_copy_parent);

	clk_set_rate(clk_copy, 1000000000);
	clk_put(clk_copy);

	clk_hpm = clk_get(NULL, "sclk_hpm");
	if (IS_ERR(clk_hpm))
		return -EINVAL;

	clk_set_rate(clk_hpm, (210 * 1000 * 1000));
	clk_put(clk_hpm);

	return 0;
out:
	if (IS_ERR(clk_pwi))
		clk_put(clk_pwi);

	if (IS_ERR(clk_pwi_parent))
		clk_put(clk_pwi_parent);

	if (IS_ERR(clk_copy))
		clk_put(clk_copy);

	if (IS_ERR(clk_copy_parent))
		clk_put(clk_copy_parent);

	if (IS_ERR(clk_hpm))
		clk_put(clk_hpm);

	return -EINVAL;
}

void set_iem_clock(void)
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
static int s5pv310_get_hpm_code(void)
{
	unsigned int i;
	unsigned long hpm_code = 0;
	unsigned int tmp;
	unsigned int hpm[LOOP_CNT];
	static void __iomem *iem_base;
	struct clk *clk_iec = NULL;
	struct clk *clk_apc = NULL;
	struct clk *clk_hpm = NULL;

	iem_base = ioremap(S5PV310_PA_IEM, (128 * 1024));
	if (iem_base == NULL) {
		printk(KERN_ERR "faile to ioremap\n");
		goto out;
	}
	/* IEC clock gate enable */
	clk_iec = clk_get(NULL, "iem-iec");
	if (IS_ERR(clk_iec)) {
		printk(KERN_ERR"ASV : IEM IEC clock get error\n");
		goto out;
	}
	clk_enable(clk_iec);

	/* APC clock gate enable */
	clk_apc = clk_get(NULL, "iem-apc");
	if (IS_ERR(clk_apc)) {
		printk(KERN_ERR"ASV : IEM APC clock get error\n");
		goto out;
	}
	clk_enable(clk_apc);

	/* hpm clock gate enalbe */
	clk_hpm = clk_get(NULL, "hpm");
	if (IS_ERR(clk_hpm)) {
		printk(KERN_ERR"ASV : HPM clock get error\n");
		goto out;
	}
	clk_enable(clk_hpm);

	if (init_iem_clock()) {
		printk(KERN_ERR "ASV driver clock_init fail\n");
		goto out;
	} else {
		/* HPM enable  */
		tmp = __raw_readl(iem_base + S5PV310_APC_CONTROL);
		tmp |= APC_HPM_EN;
		__raw_writel(tmp, (iem_base + S5PV310_APC_CONTROL));

		set_iem_clock();

		/* IEM enable */
		tmp = __raw_readl(iem_base + S5PV310_IECDPCCR);
		tmp |= IEC_EN;
		__raw_writel(tmp, (iem_base + S5PV310_IECDPCCR));
	}

	for (i = 0; i < LOOP_CNT; i++) {
		tmp = __raw_readb(iem_base + S5PV310_APC_DBG_DLYCODE);
		hpm_code += tmp;
		hpm[i] = tmp;
	}

	for (i = 0; i < LOOP_CNT; i++)
		printk(KERN_INFO "ASV : hpm[%d] = %d value\n", i, hpm[i]);

	hpm_code /= LOOP_CNT;
	printk(KERN_INFO "ASV : sum average value : %ld\n", hpm_code);
	hpm_code -= 1;
	printk(KERN_INFO "ASV : hpm value %ld\n", hpm_code);

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

	return hpm_code;

out:
	if (IS_ERR(clk_hpm)) {
		clk_disable(clk_hpm);
		clk_put(clk_hpm);
	}
	if (IS_ERR(clk_iec)) {
		clk_disable(clk_iec);
		clk_put(clk_iec);
	}
	if (IS_ERR(clk_apc)) {
		clk_disable(clk_apc);
		clk_put(clk_apc);
	}

	iounmap(iem_base);

	return -EINVAL;
}

static int s5pv310_get_hpm_group(void)
{
	unsigned int hpm_code, tmp;
	unsigned int hpm_group = 0xff;

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

	hpm_code = s5pv310_get_hpm_code();

	/* HPM SCLKAPLL */
	tmp = __raw_readl(S5P_CLKSRC_CPU);
	tmp &= ~(0x1 << S5P_CLKSRC_CPU_MUXHPM_SHIFT);
	tmp |= 0x0 << S5P_CLKSRC_CPU_MUXHPM_SHIFT;
	__raw_writel(tmp, S5P_CLKSRC_CPU);

	/* hpm grouping */
	if ((hpm_code > 0) && (hpm_code <= HPM_S))
		hpm_group = GR_S;
	else if ((hpm_code > HPM_S) && (hpm_code <= HPM_A))
		hpm_group = GR_A;
	else if ((hpm_code > HPM_A) && (hpm_code <= HPM_B))
		hpm_group = GR_B;
	else if ((hpm_code > HPM_B) && (hpm_code <= HPM_C))
		hpm_group = GR_C;
	else if (hpm_code >= HPM_D)
		hpm_group = GR_D;

	return hpm_group;
}

static int s5pv310_get_ids_arm_group(void)
{
	unsigned int ids_arm, tmp;
	unsigned int ids_arm_group = 0xff;
	struct clk *clk_chipid = NULL;

	/* chip id clock gate enable*/
	clk_chipid = clk_get(NULL, "chipid");
	if (IS_ERR(clk_chipid)) {
		printk(KERN_ERR "ASV : chipid clock get error\n");
		goto out;
	}
	clk_enable(clk_chipid);

	tmp = __raw_readl(S5P_VA_CHIPID + 0x4);

	/* get the ids_arm */
	ids_arm = ((tmp >> IDS_OFFSET) & IDS_MASK);
	if (!ids_arm) {
		printk(KERN_ERR "S5PV310 : Cannot read IDS\n");
		return -EINVAL;
	}

	clk_disable(clk_chipid);
	clk_put(clk_chipid);

	/* ids grouping */
	if ((ids_arm > 0) && (ids_arm <= IDS_S))
		ids_arm_group = GR_S;
	else if ((ids_arm > IDS_S) && (ids_arm <= IDS_A))
		ids_arm_group = GR_A;
	else if ((ids_arm > IDS_A) && (ids_arm <= IDS_B))
		ids_arm_group = GR_B;
	else if ((ids_arm > IDS_B) && (ids_arm <= IDS_C))
		ids_arm_group = GR_C;
	else if (ids_arm >= IDS_D)
		ids_arm_group = GR_D;

	printk(KERN_INFO "******************************ASV *********************\n");
	printk(KERN_INFO "ASV ids_arm = %d ids_arm_group = %d\n",
			ids_arm, ids_arm_group);

	return ids_arm_group;

out:
	if (IS_ERR(clk_chipid)) {
		clk_disable(clk_chipid);
		clk_put(clk_chipid);
	}

	return -EINVAL;
}

static int s5pv310_update_asv_table(void)
{
	unsigned int i;
	unsigned int hpm_group = 0xff, ids_arm_group = 0xff;
	unsigned int asv_group;

	/* get the ids_arm and hpm group */
	ids_arm_group = s5pv310_get_ids_arm_group();
	hpm_group = s5pv310_get_hpm_group();

	/* decide asv group */
	if (ids_arm_group > hpm_group) {
		if (ids_arm_group - hpm_group >= 3)
			asv_group = ids_arm_group - 3;
		else
			asv_group = hpm_group;
	} else {
		if (hpm_group - ids_arm_group >= 3)
			asv_group = hpm_group - 3;
		else
			asv_group = ids_arm_group;
	}

	/* set asv infomation for 3D */
	asv_info.asv_num = asv_group;
	asv_info.asv_init_done = 1;

	printk(KERN_INFO "******************************ASV *********************\n");
	printk(KERN_INFO "ASV asv_info.asv_num = %d, asv_info.asv_init_done = %d\n",
		asv_info.asv_num, asv_info.asv_init_done);
	printk(KERN_INFO "ASV ids_arm_group = %d hpm_group = %d asv_group = %d\n",
		ids_arm_group, hpm_group, asv_group);

	/* Update VDD_ARM table*/
	for (i = 0; i < CPUFREQ_LEVEL_END; i++) {
		s5pv310_volt_table[i].arm_volt =
			s5pv310_asv_cpu_volt_table[asv_group][i];
		printk(KERN_INFO "index = %d, arm_volt = %d\n",
			s5pv310_volt_table[i].index,
			s5pv310_volt_table[i].arm_volt);
	}

	/* Update VDD_INT table */
	for (i = 0; i < LV_END; i++) {
		s5pv310_busfreq_table[i].volt = asv_int_volt_table[asv_group][i];

		printk(KERN_INFO "NEW ASV busfreq_table[%d].volt = %d\n",
			i, s5pv310_busfreq_table[i].volt);
	}

	return 0;
}

static void s5pv310_set_asv_voltage(void)
{
	unsigned int asv_arm_index = 0, asv_int_index = 0;
	unsigned int asv_arm_volt, asv_int_volt;
	unsigned int rate, i;

	/* get current ARM level */
	freqs.old = s5pv310_getspeed(0);

	for (i = 0; s5pv310_freq_table[i].frequency != CPUFREQ_TABLE_END; i++) {
		if (freqs.old == s5pv310_freq_table[i].frequency) {
			asv_arm_index = s5pv310_freq_table[i].index;
		break;
		} else if (i == (CPUFREQ_LEVEL_END - 1)) {
			printk(KERN_ERR "%s: Level not found\n",
				__func__);
			goto set_asv_int;
		}
	}

	asv_arm_volt = s5pv310_volt_table[asv_arm_index].arm_volt;
#if defined(CONFIG_REGULATOR)
	regulator_set_voltage(arm_regulator, asv_arm_volt, asv_arm_volt);
#endif

set_asv_int:
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

static void print_dvfs_table(void)
{
	unsigned int i, j;

	printk(KERN_INFO "@@@@@ DVFS table values @@@@@@\n");
	for (i = 0; s5pv310_freq_table[i].frequency != CPUFREQ_TABLE_END; i++)
		printk(KERN_INFO "index = %d, freq = %d\n",
			s5pv310_freq_table[i].index,
			s5pv310_freq_table[i].frequency);

	/* output values of s5pv310_freq_table at CPUFREQ_TABLE_END */
	printk(KERN_INFO "index = %d, freq = %d\n",
		s5pv310_freq_table[i].index, s5pv310_freq_table[i].frequency);

	for (i = 0; s5pv310_freq_table[i].frequency != CPUFREQ_TABLE_END; i++)
		printk(KERN_INFO "index = %d, arm_volt = %d\n",
			s5pv310_volt_table[i].index,
			s5pv310_volt_table[i].arm_volt);

	for (i = 0; s5pv310_freq_table[i].frequency != CPUFREQ_TABLE_END; i++)
		printk(KERN_INFO "apll pms_table = 0x%08x\n",
			s5pv310_apll_pms_table[i]);

	printk(KERN_INFO "clkdiv_cpu0\n");
	for (i = 0; s5pv310_freq_table[i].frequency != CPUFREQ_TABLE_END; i++) {
		for (j = 0; j < 7; j++)
			printk("%d, ", clkdiv_cpu0[i][j]);
		printk("\n");
	}

	printk(KERN_INFO "clkdiv_cpu1\n");
	for (i = 0; s5pv310_freq_table[i].frequency != CPUFREQ_TABLE_END; i++) {
		for (j = 0; j < 2; j++)
			printk("%d, ", clkdiv_cpu1[i][j]);
		printk("\n");
	}

	printk(KERN_INFO "freq_trans_table\n");
	for (i = 0; s5pv310_freq_table[i].frequency != CPUFREQ_TABLE_END; i++) {
		for (j = 0; s5pv310_freq_table[j].frequency != CPUFREQ_TABLE_END; j++)
			printk("%d, ", freq_trans_table[i][j]);
		printk("\n");
	}
}

static int s5pv310_update_dvfs_table(void)
{
	int ret = 0;

	/* Get the maximum arm clock */
	s5pv310_max_armclk = s5pv310_get_max_speed();
	printk(KERN_INFO "armclk set max %d \n", s5pv310_max_armclk);

	if (s5pv310_max_armclk < 0) {
		printk(KERN_ERR "Fail to get max armclk infomatioin.\n");
		s5pv310_max_armclk  = 1000000; /* 1000MHz as default value */
		ret = -EINVAL;
	}

	/* logout the selected dvfs table value for debugging */
	if (debug_mask & DEBUG_CPUFREQ)
		print_dvfs_table();

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
#endif

	/*
	 * By getting chip information from pkg_id & pro_id register,
	 * adujst dvfs level, update clk divider, voltage table,
	 * apll pms value of dvfs table.
	*/
	if (s5pv310_update_dvfs_table() < 0)
		printk(KERN_INFO "arm clock limited to max 1000MHz.\n");

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
	if (s5pv310_update_asv_table())
		return -EINVAL;

	s5pv310_set_asv_voltage();
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
