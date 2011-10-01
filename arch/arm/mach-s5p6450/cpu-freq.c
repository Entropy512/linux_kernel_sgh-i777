/*  linux/arch/arm/mach-s5p6450/cpu-freq.c
 *
 *  Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *
 *  CPU frequency scaling for S5PC110
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include <asm/system.h>

#include <plat/pll.h>
#include <plat/clock.h>
#include <plat/gpio-cfg.h>

#include <mach/map.h>
#include <mach/cpu-freq-s5p6450.h>
#include <mach/regs-clock.h>
#include <mach/regs-gpio.h>

static struct clk *mpu_clk;
static struct regulator *arm_regulator;
static struct regulator *internal_regulator;

struct s5p6450_cpufreq_freqs s5p6450_freqs;

static enum perf_level bootup_level;

/* Based on MAX8698C 
 * RAMP time : 10mV/us
 **/
#define PMIC_RAMP_UP	10
static unsigned long previous_arm_volt;

/* If you don't need to wait exact ramp up delay
 * you can use fixed delay time
 **/
//#define USE_FIXED_RAMP_UP_DELAY

/* frequency */
static struct cpufreq_frequency_table s5p6450_freq_table[] = {
	{L0, 667*KHZ_T},
	{L1, 333*KHZ_T},
	{L2, 166*KHZ_T},
	{0, CPUFREQ_TABLE_END},
};

static struct s5p6450_dvs_conf s5p6450_dvs_confs[] = {
	{
		.lvl		= L0,
		.arm_volt	= 1200000,
		.int_volt	= 1100000,
	}, {
		.lvl		= L1,
		.arm_volt	= 1100000,
		.int_volt	= 1100000,

	}, {
		.lvl		= L2,
		.arm_volt	= 1000000,
		.int_volt	= 1000000,
	},
};


static u32 clkdiv_val[3][5] = {
/*{ ARM, HCLK_HIGH, PCLK_HIGH,
 *	HCLK_LOW, PCLK_LOW}
 */
	{0, 3, 1, 4, 1, },
	{1, 3, 1, 4, 1, },
	{3, 3, 1, 4, 1, },
};

static struct s5p6450_domain_freq s5p6450_clk_info[] = {
	{
		.apll_out 	= 667000,
		.armclk 	= 667000,
		.hclk_high 	= 166000,
		.pclk_high 	= 83000,
		.hclk_low 	= 133000,
		.pclk_low 	= 66000,
	}, {
		.apll_out 	= 667000,
		.armclk 	= 333000,
		.hclk_high 	= 166000,
		.pclk_high 	= 83000,
		.hclk_low 	= 133000,
		.pclk_low 	= 66000,
	}, {
		.apll_out 	= 667000,
		.armclk 	= 166000,
		.hclk_high 	= 166000,
		.pclk_high 	= 83000,
		.hclk_low 	= 133000,
		.pclk_low 	= 66000,
	}, 
};

int s5p6450_verify_speed(struct cpufreq_policy *policy)
{

	if (policy->cpu)
		return -EINVAL;

	return cpufreq_frequency_table_verify(policy, s5p6450_freq_table);
}

unsigned int s5p6450_getspeed(unsigned int cpu)
{
	unsigned long rate;

	if (cpu)
		return 0;

	rate = clk_get_rate(mpu_clk) / KHZ_T;

	return rate;
}

static void s5p6450_set_dvi_for_dvfs(unsigned int value)
{

	unsigned int reg_div0,reg_div3;

	reg_div0 = __raw_readl(S5P_CLK_DIV0);
	reg_div3 = __raw_readl(S5P_CLK_DIV3);

	reg_div0 &= ~(S5P_CLKDIV0_ARM_MASK|S5P_CLKDIV0_HCLK166_MASK|S5P_CLKDIV0_PCLK83_MASK);
	reg_div3 &= ~(S5P_CLKDIV3_HCLK133_MASK|S5P_CLKDIV3_PCLK66_MASK);

	reg_div0 |= ((clkdiv_val[value][0] << S5P_CLKDIV0_ARM_SHIFT)|
			 (clkdiv_val[value][1] << S5P_CLKDIV0_HCLK166_SHIFT)|
	 		(clkdiv_val[value][2] << S5P_CLKDIV0_PCLK83_SHIFT));
	reg_div3 |= ((clkdiv_val[value][3] << S5P_CLKDIV3_HCLK133_SHIFT) |
			 (clkdiv_val[value][4] << S5P_CLKDIV3_PCLK66_SHIFT));

	__raw_writel(reg_div0,S5P_CLK_DIV0);
	__raw_writel(reg_div3,S5P_CLK_DIV3);

}

static int s5p6450_target(struct cpufreq_policy *policy,
		       unsigned int target_freq,
		       unsigned int relation)
{
	int ret = 0;
	unsigned long arm_clk;
	unsigned int index, arm_volt, int_volt;
	unsigned int bus_speed_changing = 0;

	s5p6450_freqs.freqs.old = s5p6450_getspeed(0);

	if (cpufreq_frequency_table_target(policy, s5p6450_freq_table,
		target_freq, relation, &index)) {
		ret = -EINVAL;
		goto out;
	}

	arm_clk = s5p6450_freq_table[index].frequency;

	s5p6450_freqs.freqs.new = arm_clk;
	s5p6450_freqs.freqs.cpu = 0;

	if (s5p6450_freqs.freqs.new == s5p6450_freqs.freqs.old)
		return 0;

	arm_volt = s5p6450_dvs_confs[index].arm_volt;
	int_volt = s5p6450_dvs_confs[index].int_volt;

    /* Check if there need to change System bus clock */
	if (s5p6450_freqs.new.hclk_high != s5p6450_freqs.old.hclk_high)
	        bus_speed_changing = 1;

	/* iNew clock inforamtion update */
	memcpy(&s5p6450_freqs.new, &s5p6450_clk_info[index],
					sizeof(struct s5p6450_domain_freq));

	cpufreq_notify_transition(&s5p6450_freqs.freqs, CPUFREQ_PRECHANGE);

	if (s5p6450_freqs.freqs.new > s5p6450_freqs.freqs.old) {
		/* Voltage up */
		regulator_set_voltage(arm_regulator, arm_volt, arm_volt);
		regulator_set_voltage(internal_regulator, int_volt,	int_volt);
		s5p6450_set_dvi_for_dvfs(index);
		if(bus_speed_changing)
			__raw_writel(0X50E, S3C_VA_MEM + 0x30);
	} else {
		/* Voltage down */
		if(bus_speed_changing)
			__raw_writel(0X287, S3C_VA_MEM + 0x30);
		s5p6450_set_dvi_for_dvfs(index);
		regulator_set_voltage(arm_regulator, arm_volt,arm_volt);
		regulator_set_voltage(internal_regulator, int_volt,	int_volt);
	}

	cpufreq_notify_transition(&s5p6450_freqs.freqs, CPUFREQ_POSTCHANGE);

	memcpy(&s5p6450_freqs.old, &s5p6450_freqs.new, sizeof(struct s5p6450_domain_freq));
	printk(KERN_INFO "Perf changed[L%d]\n", index);

	previous_arm_volt = s5p6450_dvs_confs[index].arm_volt;
out:
	return ret;
}

#ifdef CONFIG_PM
static int s5p6450_cpufreq_suspend(struct cpufreq_policy *policy,
			pm_message_t pmsg)
{
	int ret = 0;

	return ret;
}

static int s5p6450_cpufreq_resume(struct cpufreq_policy *policy)
{
	int ret = 0;
	/* Clock inforamtion update with wakeup value */
	memcpy(&s5p6450_freqs.old, &s5p6450_clk_info[bootup_level],
			sizeof(struct s5p6450_domain_freq));
	previous_arm_volt = s5p6450_dvs_confs[bootup_level].arm_volt;
	return ret;
}
#endif

static int __init s5p6450_cpu_init(struct cpufreq_policy *policy)
{
	u32 reg, rate;

#ifdef CLK_OUT_PROBING
	reg = __raw_readl(S5P_CLK_OUT);
	reg &= ~(0x1f << 12 | 0xf << 20);
	reg |= (0xf << 12 | 0x1 << 20);
	__raw_writel(reg, S5P_CLK_OUT);
#endif
	mpu_clk = clk_get(NULL, MPU_CLK);
	if (IS_ERR(mpu_clk))
		return PTR_ERR(mpu_clk);
#if defined(CONFIG_REGULATOR)
	arm_regulator = regulator_get(NULL, "vddarm");
	if (IS_ERR(arm_regulator)) {
		printk(KERN_ERR "failed to get resource %s\n", "vddarm");
		return PTR_ERR(arm_regulator);
	}
	internal_regulator = regulator_get(NULL, "vddint");
	if (IS_ERR(internal_regulator)) {
		printk(KERN_ERR "failed to get resource %s\n", "vddint");
		return PTR_ERR(internal_regulator);
	}

#endif

	if (policy->cpu != 0)
		return -EINVAL;
	policy->cur = policy->min = policy->max = s5p6450_getspeed(0);

	cpufreq_frequency_table_get_attr(s5p6450_freq_table, policy->cpu);

	policy->cpuinfo.transition_latency = 40000;

	rate = clk_get_rate(mpu_clk);

	switch (rate) {
	case 66720000:	
		bootup_level = L0;
		break;
	default:
		printk(KERN_ERR "[%s] cannot find matching clock"
				"[%s] rate [%d]\n"
				, __FUNCTION__, MPU_CLK, rate);
		bootup_level = L0;
		break;
	}
	memcpy(&s5p6450_freqs.old, &s5p6450_clk_info[bootup_level],
			sizeof(struct s5p6450_domain_freq));
	
	previous_arm_volt = s5p6450_dvs_confs[bootup_level].arm_volt;
	
	return cpufreq_frequency_table_cpuinfo(policy, s5p6450_freq_table);
}

static struct cpufreq_driver s5p6450_driver = {
	.flags		= CPUFREQ_STICKY,
	.verify		= s5p6450_verify_speed,
	.target		= s5p6450_target,
	.get		= s5p6450_getspeed,
	.init		= s5p6450_cpu_init,
	.name		= "s5p6450",
#ifdef CONFIG_PM
	.suspend	= s5p6450_cpufreq_suspend,
	.resume		= s5p6450_cpufreq_resume,
#endif
};

static int __init s5p6450_cpufreq_init(void)
{
	return cpufreq_register_driver(&s5p6450_driver);
}

late_initcall(s5p6450_cpufreq_init);
