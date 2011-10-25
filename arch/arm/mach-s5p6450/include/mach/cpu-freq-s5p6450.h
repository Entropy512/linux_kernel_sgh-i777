/* arch/arm/mach-s5p6450/include/mach/cpu-freq-6450.h
 *
 *  Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/cpufreq.h>

#define USE_FREQ_TABLE
#define KHZ_T		1000
#define MPU_CLK		"armclk"

enum perf_level {
	L0,
	L1,
	L2,
};

struct s5p6450_domain_freq {
	unsigned long	apll_out;
	unsigned long	armclk;
	unsigned long	hclk_high;
	unsigned long	pclk_high;
	unsigned long	hclk_low;
	unsigned long	pclk_low;
};

struct s5p6450_cpufreq_freqs {
	struct cpufreq_freqs	freqs;
	struct s5p6450_domain_freq	old;
	struct s5p6450_domain_freq	new;
};

struct s5p6450_dvs_conf {
	const unsigned long	lvl;		/* DVFS level : L0,L1,L2.*/
	unsigned long		arm_volt;	/* uV */
	unsigned long		int_volt;	/* uV */
};

