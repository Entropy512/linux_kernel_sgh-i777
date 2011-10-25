/* linux/arch/arm/mach-s5pv310/busfreq.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S5PV310 - BUS clock frequency scaling support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/regulator/consumer.h>
#include <linux/sysfs.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/cpu.h>
#include <linux/ktime.h>
#include <linux/tick.h>
#include <linux/kernel_stat.h>

#include <mach/dmc.h>
#include <mach/map.h>
#include <mach/regs-clock.h>
#include <plat/map-s5p.h>
#include <mach/gpio.h>
#include <mach/regs-mem.h>
#include <plat/gpio-cfg.h>

#define CHECK_DELAY	HZ/25   //8 tick 40ms
#define MAX_LOAD	100
#define DIVIDING_FACTOR	10000
#define UP_THRESHOLD_DEFAULT	20
#define DEFAULT_CPU_LOAD_FOR_TRANSITION 30

static unsigned int up_threshold;
static unsigned int sampling_rate;

static unsigned int hybrid;
static unsigned int trans_load;

#undef HAVE_DAC

#ifdef HAVE_DAC
void __iomem *dac_base;
#endif

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

/* Work for cpu load monitoring */
static struct workqueue_struct *cpuload_wq;
static struct delayed_work cpuload_work;
static struct delayed_work dummy_work;

struct cpu_time_info {
	cputime64_t prev_cpu_idle;
	cputime64_t prev_cpu_wall;
	unsigned int load;
};

static DEFINE_PER_CPU(struct cpu_time_info, cpu_time);

static struct regulator *int_regulator;

static struct s5pv310_dmc_ppmu_hw dmc[2];

static struct workqueue_struct	*busfreq_wq;
static struct delayed_work busfreq_work;

static unsigned int bus_utilization[2];

enum busfreq_level_idx {
	LV_0,
	LV_1,
	LV_2,
	LV_END
};

static unsigned int time_in_state[LV_END];
static unsigned int p_idx;

struct busfreq_table {
	unsigned int idx;
	unsigned int mem_clk;
	unsigned int volt;
};

static struct busfreq_table s5pv310_busfreq_table[] = {
	{LV_0, 400000, 1100000},
	{LV_1, 267000, 1000000},
	{LV_2, 133000, 1000000},
	{0, 0, 0},
};

static unsigned int clkdiv_dmc0[LV_END][8] = {
	/*
	 * Clock divider value for following
	 * { DIVACP, DIVACP_PCLK, DIVDPHY, DIVDMC, DIVDMCD
	 *		DIVDMCP, DIVCOPY2, DIVCORE_TIMERS }
	 */

	/* DMC LV_0: 400MHz */
	{ 3, 1, 1, 1, 1, 1, 3, 1 },

	/* DMC LV_1: 266.7MHz */
	{ 4, 1, 1, 2, 1, 1, 3, 1 },

	/* DMC LV_1: 133MHz */
	{ 7, 1, 1, 5, 1, 1, 3, 1 },
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
	{ 7, 7, 7, 7, 1 },
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
	{ 7, 1 },
};

void s5pv310_set_busfreq(unsigned int div_index)
{
	unsigned int tmp;

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

}

static unsigned int calc_bus_utilization(struct s5pv310_dmc_ppmu_hw *ppmu)
{
	if (ppmu->ccnt == 0) {
		printk(KERN_ERR "%s: 0 value is not permitted\n", __func__);
		return MAX_LOAD;
	}

	return (ppmu->count[0] / DIVIDING_FACTOR * 100) /
		(ppmu->ccnt / DIVIDING_FACTOR);
}

static int busfreq_target(struct busfreq_table *freq_table,
			unsigned int bus_load,
			unsigned int pre_idx,
			unsigned int *index)
{
	unsigned int i, target_freq, idx;
	if (bus_load > MAX_LOAD)
		return -EINVAL;
    /*
        * Maximum bus_load of S5PV310 is about 40%.
        * Default Up Threshold is 30%, 2% is down level differeantial factor.
        * Policy of Targer Frequency follows ondemand governor policy in CPUFREQ framework.
        */

	if(bus_load > 40) {
		printk(KERN_INFO "Busfreq: Bus Load is larger than 40(%d)\n",bus_load);
		bus_load = 40;
}

	if(bus_load >= up_threshold) {
		target_freq = freq_table[0].mem_clk;
		*index = idx;
	}
	else if(bus_load < (up_threshold - 2)) {
		target_freq = (bus_load*freq_table[pre_idx].mem_clk)/(up_threshold - 2);

		if(target_freq >= freq_table[pre_idx].mem_clk){
			for (i = 0; (freq_table[i].mem_clk != 0); i++) {
				unsigned int freq = freq_table[i].mem_clk;

				if (freq <= target_freq) {
					idx = i;
					break;
				}
			}

		} else{
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
	}
	else {
		idx = pre_idx;
	}

	*index = idx;

	return 0;
}

static int busfreq_ppmu_init(void)
{
	unsigned int i;

	dmc[DMC0].dmc_hw_base = S5P_VA_DMC0;
	dmc[DMC1].dmc_hw_base = S5P_VA_DMC1;

	for (i = 0; i < 2; i++) {
		s5pv310_dmc_ppmu_reset(&dmc[i]);
		s5pv310_dmc_ppmu_setevent(&dmc[i], 0x6);
		s5pv310_dmc_ppmu_start(&dmc[i]);
	}

	return 0;
}

static void busfreq_timer(struct work_struct *work)
{
	unsigned int i, load, ret, index = 0;

	for (i = 0; i < 2; i++) {
		s5pv310_dmc_ppmu_stop(&dmc[i]);
		s5pv310_dmc_ppmu_update(&dmc[i]);
		bus_utilization[i] = calc_bus_utilization(&dmc[i]);
	}

	load = max(bus_utilization[0],bus_utilization[1]);

	/* Change bus frequency */
	ret = busfreq_target(s5pv310_busfreq_table, load, p_idx, &index);
	if (ret)
		printk(KERN_ERR "%s: (%d)\n", __func__, ret);

	if (p_idx != index) {
		unsigned int voltage = s5pv310_busfreq_table[index].volt;
		if (p_idx > index)
			regulator_set_voltage(int_regulator, voltage, voltage);

		s5pv310_set_busfreq(index);

		if (p_idx < index)
			regulator_set_voltage(int_regulator, voltage, voltage);

		p_idx = index;
	}

	switch(index){
		case LV_0:
			time_in_state[LV_0] += 1;
			break;
		case LV_1:
			time_in_state[LV_1] += 1;
			break;
		case LV_2:
			time_in_state[LV_2] += 1;
			break;
		default:
			break;
		}
#ifdef HAVE_DAC

	load = (255 * load) / 100;
	__raw_writeb(load, dac_base);
#endif

	busfreq_ppmu_init();

	queue_delayed_work_on(0, busfreq_wq, &busfreq_work, sampling_rate);
}

static unsigned int high_transition;

static void cpuload_nothing(struct work_struct *work)
{
	high_transition = 0;
	if (hybrid == 1)
		queue_delayed_work_on(0, cpuload_wq, &cpuload_work, 0);
	else
		queue_delayed_work_on(0, cpuload_wq, &dummy_work, 200);
}

static void cpuload_timer(struct work_struct *work)
{
	unsigned int i, avg_load, max_load, load = 0;

	for_each_online_cpu(i) {
		struct cpu_time_info *tmp_info;
		cputime64_t cur_wall_time, cur_idle_time;
		unsigned int idle_time, wall_time;

		tmp_info = &per_cpu(cpu_time, i);

		cur_idle_time = get_cpu_idle_time_us(i, &cur_wall_time);

		idle_time = (unsigned int) cputime64_sub(cur_idle_time,
						tmp_info->prev_cpu_idle);
		tmp_info->prev_cpu_idle = cur_idle_time;

		wall_time = (unsigned int) cputime64_sub(cur_wall_time,
						tmp_info->prev_cpu_wall);
		tmp_info->prev_cpu_wall = cur_wall_time;

		if(wall_time < idle_time)
			idle_time = wall_time;

		tmp_info->load = 100 * (wall_time - idle_time) / wall_time;

		if(tmp_info->load > load)
			load = tmp_info->load;
	}
	max_load=load;
	avg_load = load / num_online_cpus();

	if (high_transition == 0) {
		if (max_load > trans_load) {
			cancel_delayed_work_sync(&busfreq_work);
			high_transition = 1;
			sampling_rate = HZ/25;  //40ms
		}
	} else {
		if (max_load <= trans_load) {
			cancel_delayed_work_sync(&busfreq_work);
			high_transition = 0;
			sampling_rate = HZ/10; //100ms
		}
	}

	queue_delayed_work_on(0, busfreq_wq, &busfreq_work, 0);

	if (hybrid == 1)
		queue_delayed_work_on(0, cpuload_wq, &cpuload_work, HZ/25);
	else
		queue_delayed_work_on(0, cpuload_wq, &dummy_work, HZ);

}

static int __init s5pv310_busfreq_init(void)
{
	int ret,i;

#ifdef HAVE_DAC
	s5pv310_dac_init();
#endif
	ret = busfreq_ppmu_init();
	if (ret)
		return ret;

	p_idx = LV_0;
#ifdef CONFIG_REGULATOR
	int_regulator = regulator_get(NULL, "vdd_int");
	if (IS_ERR(int_regulator)) {
		printk(KERN_ERR "failed to get resource %s\n", "vdd_int");
		goto out;
	}
#endif
	busfreq_wq = create_workqueue("busfreq monitor");
	if (!busfreq_wq) {
		printk(KERN_ERR "Creation of busfreq work failed\n");
		return -EFAULT;
	}
	INIT_DELAYED_WORK(&busfreq_work, busfreq_timer);

	sampling_rate = CHECK_DELAY;
	up_threshold = UP_THRESHOLD_DEFAULT;

	for(i =0; i < LV_END; i++) {
		time_in_state[i]=0;
	}

	hybrid = 1;

	queue_delayed_work_on(0, busfreq_wq, &busfreq_work, CHECK_DELAY);

	high_transition = 0;
	trans_load = DEFAULT_CPU_LOAD_FOR_TRANSITION;

	cpuload_wq = create_workqueue("cpuload monitor");
	if (!cpuload_wq) {
		printk(KERN_ERR "Creation of cpuload monitor work failed\n");
		return -EFAULT;
	}
	INIT_DELAYED_WORK_DEFERRABLE(&cpuload_work, cpuload_timer);
	INIT_DELAYED_WORK_DEFERRABLE(&dummy_work, cpuload_nothing);

	queue_delayed_work_on(0, cpuload_wq, &dummy_work, HZ);

	return 0;
out:
	printk(KERN_ERR "%s: failed initialization\n", __func__);
	return -EINVAL;
}

late_initcall(s5pv310_busfreq_init);

static ssize_t show_up_time_in_state(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	ssize_t len = 0;
	int i;

	for (i = 0; i < LV_END; i++) {
		len += sprintf(buf + len, "%u: %u\n", s5pv310_busfreq_table[i].mem_clk, time_in_state[i]);
	}
	return len;
}

static ssize_t store_up_time_in_state(struct device *dev, struct device_attribute *attr,
			   const char *buf, size_t count)
{

	return count;
}

static DEVICE_ATTR(time_in_state, 0644, show_up_time_in_state, store_up_time_in_state);

static ssize_t show_up_threshold(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	return sprintf(buf, "%u\n", up_threshold);
}

static ssize_t store_up_threshold(struct device *dev, struct device_attribute *attr,
			   const char *buf, size_t count)
{
	int ret;

	ret = sscanf(buf, "%u", &up_threshold);
	if (ret != 1)
		return -EINVAL;
	printk("** Up_Threshold is changed to %u **",up_threshold);
	return count;
}

static DEVICE_ATTR(up_threshold, 0644, show_up_threshold, store_up_threshold);

static ssize_t show_sampling_rate(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	return sprintf(buf, "%u\n", sampling_rate*5);
}

static ssize_t store_sampling_rate(struct device *dev, struct device_attribute *attr,
			   const char *buf, size_t count)
{
	int ret;

    if(hybrid == 0) {
		ret = sscanf(buf, "%u", &sampling_rate);
		if (ret != 1)
		return -EINVAL;
			printk("** Sampling_Rate is changed to %u ms **\n",sampling_rate*5);
    }
	else {
		printk("** Sampling_Rate is hybrid. Disable fixed sample_rate **\n");
	}

	return count;
}

static DEVICE_ATTR(sampling_rate, 0644, show_sampling_rate, store_sampling_rate);

static ssize_t show_hybrid(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	return sprintf(buf, "%u\n", hybrid);
}

static ssize_t store_hybrid(struct device *dev, struct device_attribute *attr,
			   const char *buf, size_t count)
{
	int ret;

	ret = sscanf(buf, "%u", &hybrid);
	if (ret != 1)
		return -EINVAL;

	if (hybrid > 1)
		return -EINVAL;
	if(hybrid == 0)
		printk("** Sampling_Rate is fixed(%u ms) **\n",sampling_rate*5);
	else
		printk("** Sampling_Rate is Hybrid **\n");

	return count;
}

static DEVICE_ATTR(hybrid, 0644, show_hybrid, store_hybrid);

static ssize_t show_trans_load(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	return sprintf(buf, "%u\n", trans_load);
}

static ssize_t store_trans_load(struct device *dev, struct device_attribute *attr,
			   const char *buf, size_t count)
{
	int ret;

	ret = sscanf(buf, "%u", &trans_load);
	if (ret != 1)
		return -EINVAL;

	if (trans_load > 100)
		return -EINVAL;
	return count;
}

static DEVICE_ATTR(trans_load, 0644, show_trans_load, store_trans_load);

static int sysfs_busfreq_create(struct device *dev)
{
	int ret;

	ret = device_create_file(dev, &dev_attr_up_threshold);

	if (ret)
		return ret;

	ret = device_create_file(dev, &dev_attr_sampling_rate);

	if (ret)
		goto failed;

	ret = device_create_file(dev, &dev_attr_hybrid);

	if (ret)
		goto failed_hybrid;

	ret = device_create_file(dev, &dev_attr_trans_load);

	if (ret)
		goto failed_trans_load;

	ret = device_create_file(dev, &dev_attr_time_in_state);

	if (ret)
		goto failed_time_in_state;

		return ret;

failed_time_in_state:
	device_remove_file(dev, &dev_attr_trans_load);
failed_trans_load:
	device_remove_file(dev, &dev_attr_hybrid);
failed_hybrid:
	device_remove_file(dev, &dev_attr_sampling_rate);
failed:
	device_remove_file(dev, &dev_attr_up_threshold);

	return ret;
}

static void sysfs_busfreq_remove(struct device *dev)
{
	device_remove_file(dev, &dev_attr_up_threshold);
	device_remove_file(dev, &dev_attr_sampling_rate);
	device_remove_file(dev, &dev_attr_hybrid);
	device_remove_file(dev, &dev_attr_trans_load);
	device_remove_file(dev, &dev_attr_time_in_state);
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
