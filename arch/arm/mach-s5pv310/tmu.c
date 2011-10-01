/* linux/arch/arm/mach-s5pv310/tmu.c
*
* Copyright (c) 2010 Samsung Electronics Co., Ltd.
*      http://www.samsung.com/
*
* S5PV310 - TMU driver
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/slab.h>

#include <asm/irq.h>

#include <mach/regs-tmu.h>
#include <mach/cpufreq.h>
#include <mach/map.h>
#include <plat/s5p-tmu.h>
#include <plat/gpio-cfg.h>

/* Support Bootloader parameter setting by SBL interface */
#undef CONFIG_TMU_DEBUG_ENABLE

/* Selectable one room temperature among 3 kinds */
#undef  OPERATION_TEMP_BASE_78
#define OPERATION_TEMP_BASE_61

#define TIMMING_AREF 0x30
#define AUTO_REFRESH_PERIOD_TQ0		0x2E /* auto refresh preiod 1.95us */
#define AUTO_REFRESH_PERIOD_NORMAL	0x5D /* auto refresh period 3.9us */

/*
 *  For example
 *  test value for room temperature
 *  base operation temp : OPERATION_TEMP_BASE_78
*/

#ifdef OPERATION_TEMP_BASE_78
/* TMU register setting value */
#define THD_TEMP 0x80	  /* 78 degree  : threshold temp */
#define TRIGGER_LEV0 0x9  /* 87 degree  : throttling temperature */
#define	TRIGGER_LEV1 0x19 /* 103 degree  : Waring temperature */
#define TRIGGER_LEV2 0x20 /* 110 degree : Tripping temperature  */
#define	TRIGGER_LEV3 0xFF /* Reserved */

/* interrupt level by celcius degree */
#define TEMP_MIN_CELCIUS	 25
#define TEMP_TROTTLED_CELCIUS	 87
#define TEMP_TQ0_CELCIUS	 85
#define TEMP_WARNING_CELCIUS	103
#define TEMP_TRIPPED_CELCIUS	110
#define TEMP_MAX_CELCIUS	125
#endif

#ifdef OPERATION_TEMP_BASE_61
/* test on 35 celsius base */
#define THD_TEMP     0x6F /* 61 degree: thershold temp */
#define TRIGGER_LEV0 0x3  /* 64	degree: Throttling temperature */
#define TRIGGER_LEV1 0x2A /* 103 degree: Waring temperature */
#define TRIGGER_LEV2 0x31 /* 110 degree: Tripping temperature */
#define TRIGGER_LEV3 0xFF /* Reserved */
#define TEMP_TROTTLED_CELCIUS	 64
#define TEMP_WARNING_CELCIUS	103
#define TEMP_TQ0_CELCIUS	 85
#define TEMP_TRIPPED_CELCIUS	110
#define TEMP_MIN_CELCIUS	 25
#define TEMP_MAX_CELCIUS	125
#endif

#define TMU_SAVE_NUM   10
#define VREF_SLOPE     0x07000F02
#define TMU_EN         0x1
#define TMU_DC_VALUE   25
#define TMU_CODE_25_DEGREE 0x4B
#define EFUSE_MIN_VALUE 60
#define EFUSE_AVG_VALUE 80
#define EFUSE_MAX_VALUE 100
#define FIN	(24*1000*1000)

static struct workqueue_struct  *tmu_monitor_wq;
unsigned int tmu_save[TMU_SAVE_NUM];

enum tmu_status_t {
	TMU_STATUS_NORMAL = 0,
	TMU_STATUS_THROTTLED,
	TMU_STATUS_WARNING,
	TMU_STATUS_TRIPPED,
	TMU_STATUS_INIT,
};

struct tmu_data_band {
	int thr_low;
	int thr_high;
	int warn_low;
	int warn_high;
	int trip_retry;
	int tq0_temp;
};

struct tmu_data_band tmu_temp_band = {
#ifdef OPERATION_TEMP_BASE_61
	/*  61 : low temp of throttling */
	.thr_low	= TEMP_TROTTLED_CELCIUS	- 3,
#else
	/*  83 : low temp of throttling */
	.thr_low	= TEMP_TROTTLED_CELCIUS	- 4,
#endif
	/*  90 : hith temp of warning */
	.thr_high	= TEMP_WARNING_CELCIUS	- 5,
	/*  97 : low temp of warning */
	.warn_low	= TEMP_WARNING_CELCIUS	- 6,
	/* 105 : high temp of warning */
	.warn_high	= TEMP_WARNING_CELCIUS	+ 3,
	/* 113 : trip re-try */
	.trip_retry	= TEMP_TRIPPED_CELCIUS  + 3,
	/* 85: tq0 temp */
	.tq0_temp	= TEMP_TQ0_CELCIUS,
};

static DEFINE_MUTEX(tmu_lock);

struct s5p_tmu_info {
	struct device *dev;

	char *tmu_name;
	struct s5p_tmu *ctz;
	struct tmu_data_band *temp;

	struct delayed_work monitor_work;
	struct delayed_work polling_work;

	unsigned int monitor_period;
	unsigned int sampling_rate;

	struct resource *ioarea;
	unsigned int irq;
	unsigned int reg_save[TMU_SAVE_NUM];
	int tmu_status;
};
struct s5p_tmu_info *tmu_info;

#ifdef CONFIG_TMU_DEBUG_ENABLE
static int set_tmu_test;
#ifdef OPERATION_TEMP_BASE_61
static int set_thr_stop		= (TEMP_TROTTLED_CELCIUS - 3);
#else
static int set_thr_stop		= (TEMP_TROTTLED_CELCIUS - 4);
#endif
static int set_thr_temp		= TEMP_TROTTLED_CELCIUS;
static int set_warn_stop	= (TEMP_WARNING_CELCIUS	- 6);
static int set_warn_temp	= TEMP_WARNING_CELCIUS;
static int set_trip_temp	= TEMP_TRIPPED_CELCIUS;
static int set_tq0_temp		= TEMP_TQ0_CELCIUS;

static int set_sampling_rate;
static int set_cpu_level	= 3;

static int __init tmu_test_param(char *str)
{
	int tmu_temp[7] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL,};

	get_options(str, 7, tmu_temp);

	set_tmu_test	= tmu_temp[0];
	printk(KERN_INFO "@@@tmu_test enable = %d\n", set_tmu_test);

	if (tmu_temp[1])
		set_thr_stop	= tmu_temp[1];
	printk(KERN_INFO "@@@1st throttling stop temp = %d\n", set_thr_stop);

	if (tmu_temp[2])
		set_thr_temp	= tmu_temp[2];
	printk(KERN_INFO "@@@1st throttling start temp = %d\n", set_thr_temp);

	if (tmu_temp[3])
		set_warn_stop   = tmu_temp[3];
	printk(KERN_INFO "@@@2nd throttling stop temp = %d\n", set_warn_stop);

	if (tmu_temp[4])
		set_warn_temp	= tmu_temp[4];
	printk(KERN_INFO "@@@2nd throttling start temp = %d\n", set_warn_temp);

	if (tmu_temp[5])
		set_trip_temp	= tmu_temp[5];
	printk(KERN_INFO "@@@tripping temp = %d\n", set_trip_temp);

	if (tmu_temp[6])
		set_tq0_temp	= tmu_temp[6];
	printk(KERN_INFO "@@@memory throttling temp = %d\n", set_tq0_temp);

	return 0;
}
early_param("tmu_test", tmu_test_param);

static int __init limit_param(char *str)
{
	get_option(&str, &set_cpu_level);
	if (set_cpu_level < 0)
		set_cpu_level = 0;

	return 0;
}
early_param("max_cpu_level", limit_param);

static int __init sampling_rate_param(char *str)
{
	get_option(&str, &set_sampling_rate);
	if (set_sampling_rate < 0)
		set_sampling_rate = 0;

	return 0;
}
early_param("tmu_sampling_rate", sampling_rate_param);

static void tmu_start_testmode(struct platform_device *pdev)
{
	struct s5p_tmu *tz = platform_get_drvdata(pdev);
	unsigned int con;
	unsigned int thresh_temp_adc, thr_temp_adc, trip_temp_adc;
	unsigned int warn_temp_adc = 0xFF;

	/* To use handling routine, change temperature date of tmu info */
	tmu_info->temp->thr_low		 = set_thr_stop;
	tmu_info->temp->thr_high	 = set_warn_temp - 5;
	tmu_info->temp->warn_low	 = set_warn_stop;
	tmu_info->temp->warn_high	 = set_warn_temp + 5;
	tmu_info->temp->trip_retry	 = set_trip_temp + 3;
	tmu_info->temp->tq0_temp	 = set_tq0_temp;

	pr_info("1st throttling stop_temp  = %d, start_temp = %d\n,\
		2nd throttling stop_temp = %d, start_tmep = %d\n,\
		tripping temp = %d, tripping retry_temp = %d\n,\
		memory throttling stop_temp = %d, start_temp = %d\n",
		tmu_info->temp->thr_low, tmu_info->temp->thr_high - 4,
		tmu_info->temp->warn_low, tmu_info->temp->warn_high - 3,
		tmu_info->temp->trip_retry - 3, tmu_info->temp->trip_retry,
		tmu_info->temp->tq0_temp -5, tmu_info->temp->tq0_temp);

	/* Compensation temperature THD_TEMP */
	thresh_temp_adc	= set_thr_stop + tz->data.te1 - TMU_DC_VALUE;
	thr_temp_adc	= set_thr_temp + tz->data.te1 - TMU_DC_VALUE
				- thresh_temp_adc;
	warn_temp_adc   = set_warn_temp + tz->data.te1 - TMU_DC_VALUE
				- thresh_temp_adc;
	trip_temp_adc	= set_trip_temp + tz->data.te1 - TMU_DC_VALUE
				- thresh_temp_adc;
	pr_info("Compensated Threshold: 0x%2x\n", thresh_temp_adc);

	/* Set interrupt trigger level */
	__raw_writel(thresh_temp_adc, tz->tmu_base + THRESHOLD_TEMP);
	__raw_writel(thr_temp_adc, tz->tmu_base + TRG_LEV0);
	__raw_writel(warn_temp_adc, tz->tmu_base + TRG_LEV1);
	__raw_writel(trip_temp_adc, tz->tmu_base + TRG_LEV2);
	__raw_writel(TRIGGER_LEV3, tz->tmu_base + TRG_LEV3);

	pr_info("Cooling: %dc  THD_TEMP:0x%02x:  TRIG_LEV0: 0x%02x\
		TRIG_LEV1: 0x%02x TRIG_LEV2: 0x%02x, TRIG_LEV3: 0x%02x\n",
		set_thr_stop,
		__raw_readl(tz->tmu_base + THRESHOLD_TEMP),
		__raw_readl(tz->tmu_base + TRG_LEV0),
		__raw_readl(tz->tmu_base + TRG_LEV1),
		__raw_readl(tz->tmu_base + TRG_LEV2),
		__raw_readl(tz->tmu_base + TRG_LEV3));

	mdelay(50);
	/* TMU core enable */
	con = __raw_readl(tz->tmu_base + TMU_CON0);
	con |= TMU_EN;

	__raw_writel(con, tz->tmu_base + TMU_CON0);

	/*LEV0 LEV1 LEV2 interrupt enable */
	__raw_writel(INTEN0 | INTEN1 | INTEN2, tz->tmu_base + INTEN);

	return;
}
#endif

static void set_refresh_rate(unsigned int auto_refresh)
{
	/*
	 * uRlk = FIN / 100000;
	 * refresh_usec =  (unsigned int)(fMicrosec * 10);
	 * uRegVal = ((unsigned int)(uRlk * uMicroSec / 100)) - 1;
	*/
	/* change auto refresh period of dmc0 */
	__raw_writel(auto_refresh, S5P_VA_DMC0 + TIMMING_AREF);

	/* change auto refresh period of dmc1 */
	__raw_writel(auto_refresh, S5P_VA_DMC1 + TIMMING_AREF);
}

static int tmu_tripped_cb(int state)
{
	struct power_supply *psy = power_supply_get_by_name("battery");
	union power_supply_propval value;

	if (!psy) {
		pr_err("%s:fail to get batter ps\n", __func__);
		return -ENODEV;
	}
	pr_info("%s:is pass, state %d.\n", __func__, state);

	switch(state) {
	case TMU_STATUS_NORMAL:
		value.intval = TMU_STATUS_NORMAL;
		break;
	case TMU_STATUS_THROTTLED:
		value.intval = TMU_STATUS_THROTTLED;
		break;
	case TMU_STATUS_WARNING:
		value.intval = TMU_STATUS_WARNING;
		break;
	case TMU_STATUS_TRIPPED:
		value.intval = TMU_STATUS_TRIPPED;
		break;
	default:
		pr_warn("value is not correct.\n");
		return -EINVAL;
	}

	return psy->set_property(psy, POWER_SUPPLY_PROP_HEALTH, &value);
}

#ifdef CONFIG_TMU_DEBUG_ENABLE
static void tmu_mon_timer(struct work_struct *work)
{
	unsigned char cur_temp_adc;
	int cur_temp;

	/* Compensation temperature */
	cur_temp_adc =
		__raw_readl(tmu_info->ctz->tmu_base + CURRENT_TEMP) & 0xff;
	cur_temp = cur_temp_adc - tmu_info->ctz->data.te1 + TMU_DC_VALUE;
	if (cur_temp < 25) {
		/* temperature code range is from 25 to 125 */
		pr_info("current temp is under 25 celsius degree!\n");
		cur_temp = 0;
	}

	pr_info("cur temp = %d, adc_value = 0x%02x\n", cur_temp, cur_temp_adc);

	if (set_tmu_test) {
		pr_info("Current: %d c, Cooling: %d c, Throttling: %d c, \
				Warning: %d c, Tripping: %d c\n",
			cur_temp, tmu_info->temp->thr_low,
			set_thr_temp, set_warn_temp, set_trip_temp);
	} else {
		pr_info("Current: %d c, Cooling: %d c  Throttling: %d c \
				Warning: %d c  Tripping: %d c\n",
			cur_temp, tmu_info->temp->thr_low,
			TEMP_TROTTLED_CELCIUS,
			TEMP_WARNING_CELCIUS,
			TEMP_TRIPPED_CELCIUS);
	}

	queue_delayed_work_on(0, tmu_monitor_wq, &tmu_info->monitor_work,
			tmu_info->monitor_period);
}

static void tmu_poll_testmode(void)
{
	unsigned char cur_temp_adc;
	int cur_temp;
	int thr_temp, trip_temp, warn_temp;
	static int cpufreq_limited_thr	= 0;
	static int cpufreq_limited_warn	= 0;
	static int send_msg_battery = 0;
	static int auto_refresh_changed = 0;

	thr_temp  = set_thr_temp;
	warn_temp = set_warn_temp;
	trip_temp = set_trip_temp;

	/* Compensation temperature */
	cur_temp_adc =
		__raw_readl(tmu_info->ctz->tmu_base + CURRENT_TEMP) & 0xff;
	cur_temp = cur_temp_adc - tmu_info->ctz->data.te1 + TMU_DC_VALUE;
	if (cur_temp < 25) {
		/* temperature code range is from 25 to 125 */
		pr_info("current temp is under 25 celsius degree!\n");
		cur_temp = 0;
	}

	pr_info("current temp = %d, tmu_state = %d\n",
			cur_temp, tmu_info->ctz->data.tmu_flag);

	switch (tmu_info->ctz->data.tmu_flag) {
	case TMU_STATUS_NORMAL:
		if (cur_temp <= tmu_info->temp->thr_low) {
			//cancel_delayed_work(&tmu_info->polling_work);
			if (tmu_tripped_cb(TMU_STATUS_NORMAL) < 0)
				pr_err("Error inform to battery driver !\n");
			else
				pr_info("normal: interrupt enable.\n");

			/* To prevent from interrupt by current pending bit */
			__raw_writel(INTCLEARALL,
					tmu_info->ctz->tmu_base + INTCLEAR);
			enable_irq(tmu_info->irq);
			return;
		}

		if (cur_temp >= set_thr_temp) { /* 85 */
			tmu_info->ctz->data.tmu_flag = TMU_STATUS_THROTTLED;
			s5pv310_cpufreq_upper_limit(DVFS_LOCK_ID_TMU, CPU_L2);
			cpufreq_limited_thr = 1;
			if (tmu_tripped_cb(TMU_STATUS_THROTTLED) < 0)
				pr_err("Error inform to battery driver !\n");
			else
				pr_info("normal->throttle:\
						set cpufreq upper limit.\n");
		}
		break;

	case TMU_STATUS_THROTTLED:
		if (cur_temp >= set_thr_temp && !(cpufreq_limited_thr)) {
			s5pv310_cpufreq_upper_limit(DVFS_LOCK_ID_TMU, CPU_L2);
			cpufreq_limited_thr = 1;
			if (tmu_tripped_cb(TMU_STATUS_THROTTLED) < 0)
				pr_err("Error inform to battery driver !\n");
			else
				pr_info("throttling:\
						set cpufreq upper limit.\n");
		}

		if (cur_temp <= tmu_info->temp->thr_low) {
			s5pv310_cpufreq_upper_limit_free(DVFS_LOCK_ID_TMU);
			tmu_info->ctz->data.tmu_flag = TMU_STATUS_NORMAL;
			cpufreq_limited_thr = 0;
			pr_info("throttling->normal: free cpufreq upper limit.\n");
		}

		if (cur_temp >= set_warn_temp) { /* 100 */
			tmu_info->ctz->data.tmu_flag = TMU_STATUS_WARNING;
			s5pv310_cpufreq_upper_limit_free(DVFS_LOCK_ID_TMU);
			cpufreq_limited_thr = 0;
			if (set_cpu_level == 3)
				s5pv310_cpufreq_upper_limit(DVFS_LOCK_ID_TMU,
						CPU_L3); /* CPU_L3 */
			else
				s5pv310_cpufreq_upper_limit(DVFS_LOCK_ID_TMU,
						CPU_L4); /* CPU_L4 */

			cpufreq_limited_warn = 1;
			if (tmu_tripped_cb(TMU_STATUS_WARNING) < 0)
				pr_err("Error inform to battery driver !\n");
			else
				pr_info("throttling->warning:\
						up cpufreq upper limit.\n");
		}
		break;

	case TMU_STATUS_WARNING:
		if (cur_temp >= set_warn_temp && !(cpufreq_limited_warn)) { /* 100 */
			s5pv310_cpufreq_upper_limit_free(DVFS_LOCK_ID_TMU);
			cpufreq_limited_thr = 0;
			if (set_cpu_level == 3)
				s5pv310_cpufreq_upper_limit(DVFS_LOCK_ID_TMU,
						CPU_L3); /* CPU_L3 */
			else
				s5pv310_cpufreq_upper_limit(DVFS_LOCK_ID_TMU,
						CPU_L4); /* CPU_L4 */

			cpufreq_limited_warn = 1;
			if (tmu_tripped_cb(TMU_STATUS_WARNING) < 0)
				pr_err("Error inform to battery driver !\n");
			else
				pr_info("warning: set cpufreq upper limit.\n");
		}

		/* if (cur_temp < tmu_info->band->warn_low) { */
		if (cur_temp < set_warn_stop) {
			tmu_info->ctz->data.tmu_flag = TMU_STATUS_THROTTLED;
			s5pv310_cpufreq_upper_limit_free(DVFS_LOCK_ID_TMU);
			cpufreq_limited_warn = 0;
			s5pv310_cpufreq_upper_limit(DVFS_LOCK_ID_TMU,
					CPU_L2); /* CPU_L2 */
			cpufreq_limited_thr = 1;
			if (tmu_tripped_cb(TMU_STATUS_THROTTLED) < 0)
				pr_err("Error inform to battery driver !\n");
			else
				pr_info("warning->throttling:\
						down cpufreq upper limit.\n");
		}

		if (cur_temp >= set_trip_temp) { /* 110 */
			tmu_info->ctz->data.tmu_flag = TMU_STATUS_TRIPPED;
			if (tmu_tripped_cb(TMU_STATUS_TRIPPED) < 0)
				pr_err("Error inform to battery driver !\n");
			else
				pr_info("warning->tripping:\
						waiting shutdown !!!\n");
		}
		break;

	case TMU_STATUS_INIT: /* sned tmu initial status to battery drvier */
		disable_irq(tmu_info->irq);

		if (cur_temp <= tmu_info->temp->thr_low)
			tmu_info->ctz->data.tmu_flag = TMU_STATUS_NORMAL;
		else
			tmu_info->ctz->data.tmu_flag = TMU_STATUS_THROTTLED;
		break;

	case TMU_STATUS_TRIPPED:
		if (cur_temp >= set_trip_temp && !(send_msg_battery)) {
			if (tmu_tripped_cb(TMU_STATUS_TRIPPED) < 0)
				pr_err("Error inform to battery driver !\n");
			else {
				pr_info("tripping: waiting shutdown.\n");
				send_msg_battery = 1;
			}
		}

		if (cur_temp >= (set_trip_temp + 5)) {
			panic("Emergency!!!!\
				tmu tripping event is not treated! \n");
		}

		if (cur_temp >= tmu_info->temp->trip_retry) {
			pr_warn("WARNING!!: try to send msg to\
					battery driver again\n");
			send_msg_battery = 0;
		}

		/* safety code */
		if (cur_temp <= tmu_info->temp->thr_low) {
			s5pv310_cpufreq_upper_limit_free(DVFS_LOCK_ID_TMU);
			tmu_info->ctz->data.tmu_flag = TMU_STATUS_NORMAL;
			cpufreq_limited_thr = 0;
			pr_info("tripping->normal:\
					check! occured only test mode.\n");
		}
		break;

	default:
		pr_warn("bug: checked tmu_state.\n");
		if (cur_temp < tmu_info->temp->warn_high) {
			tmu_info->ctz->data.tmu_flag = TMU_STATUS_WARNING;
		} else {
			tmu_info->ctz->data.tmu_flag = TMU_STATUS_TRIPPED;
			if (tmu_tripped_cb(TMU_STATUS_TRIPPED) < 0)
				pr_err("Error inform to battery driver !\n");
		}
		break;
	}

	if (cur_temp >= tmu_info->temp->tq0_temp) {
		if (!(auto_refresh_changed)) {
			pr_info("set auto_refresh 1.95us\n");
			set_refresh_rate(AUTO_REFRESH_PERIOD_TQ0);
			auto_refresh_changed = 1;
		}
	} else if (cur_temp <= (tmu_info->temp->tq0_temp - 5)) {
		if (auto_refresh_changed) {
			pr_info("set auto_refresh 3.9us\n");
			set_refresh_rate(AUTO_REFRESH_PERIOD_NORMAL);
			auto_refresh_changed = 0;
		}
	}

	/* rescheduling next work */
	queue_delayed_work_on(0, tmu_monitor_wq, &tmu_info->polling_work,
			tmu_info->sampling_rate);
}
#endif

static void tmu_poll_timer(struct work_struct *work)
{
	int cur_temp;
	static int cpufreq_limited_thr	= 0;
	static int cpufreq_limited_warn	= 0;
	static int send_msg_battery = 0;
	static int auto_refresh_changed = 0;

#ifdef CONFIG_TMU_DEBUG_ENABLE
	if (set_tmu_test) {
		tmu_poll_testmode();
		return;
	}
#endif
	mutex_lock(&tmu_lock);

	/* Compensation temperature */
	cur_temp = (__raw_readl(tmu_info->ctz->tmu_base + CURRENT_TEMP) & 0xff)
			- tmu_info->ctz->data.te1 + TMU_DC_VALUE;
	if (cur_temp < 25) {
		/* temperature code range is from 25 to 125 */
		pr_info("current temp is under 25 celsius degree!\n");
		cur_temp = 0;
	}
	pr_info("current temp = %d, tmu_state = %d\n",
			cur_temp, tmu_info->ctz->data.tmu_flag);

	switch (tmu_info->ctz->data.tmu_flag) {
	case TMU_STATUS_NORMAL:
		if (cur_temp <= tmu_info->temp->thr_low) {
			if (tmu_tripped_cb(TMU_STATUS_NORMAL) < 0)
				pr_err("Error inform to battery driver !\n");
			else
				pr_info("normal: interrupt enable.\n");

			/* clear to prevent from interfupt by peindig bit */
			__raw_writel(INTCLEARALL,
					tmu_info->ctz->tmu_base + INTCLEAR);
			enable_irq(tmu_info->irq);
			mutex_unlock(&tmu_lock);
			return;
		}
		if (cur_temp >= TEMP_TROTTLED_CELCIUS) { /* 87 */
			tmu_info->ctz->data.tmu_flag = TMU_STATUS_THROTTLED;
			s5pv310_cpufreq_upper_limit(DVFS_LOCK_ID_TMU, CPU_L2);
			cpufreq_limited_thr = 1;
			if (tmu_tripped_cb(TMU_STATUS_THROTTLED) < 0)
				pr_err("Error inform to battery driver !\n");
			else
				pr_info("normal->throttle:\
						set cpufreq upper limit.\n");
		}
		break;

	case TMU_STATUS_THROTTLED:
		if (cur_temp >= TEMP_TROTTLED_CELCIUS &&
				!(cpufreq_limited_thr)) {
			s5pv310_cpufreq_upper_limit(DVFS_LOCK_ID_TMU, CPU_L2);
			cpufreq_limited_thr = 1;
			if (tmu_tripped_cb(TMU_STATUS_THROTTLED) < 0)
				pr_err("Error inform to battery driver !\n");
			else
				pr_info("throttling:\
						set cpufreq upper limit.\n");
		}
		if (cur_temp <= tmu_info->temp->thr_low) {
			s5pv310_cpufreq_upper_limit_free(DVFS_LOCK_ID_TMU);
			tmu_info->ctz->data.tmu_flag = TMU_STATUS_NORMAL;
			cpufreq_limited_thr = 0;
			pr_info("throttling->normal:\
					free cpufreq upper limit.\n");
		}
		if (cur_temp >= TEMP_WARNING_CELCIUS) { /* 103 */
			tmu_info->ctz->data.tmu_flag = TMU_STATUS_WARNING;
			s5pv310_cpufreq_upper_limit_free(DVFS_LOCK_ID_TMU);
			cpufreq_limited_thr = 0;
			s5pv310_cpufreq_upper_limit(DVFS_LOCK_ID_TMU, CPU_L4); /* CPU_L4 */
			cpufreq_limited_warn = 1;
			if (tmu_tripped_cb(TMU_STATUS_WARNING) < 0)
				pr_err("Error inform to battery driver !\n");
			else
				pr_info("throttling->warning:\
						set cpufreq upper limit.\n");
		}
		break;

	case TMU_STATUS_WARNING:
		if (cur_temp >= TEMP_WARNING_CELCIUS &&
				!(cpufreq_limited_warn)) { /* 103 */
			s5pv310_cpufreq_upper_limit_free(DVFS_LOCK_ID_TMU);
			cpufreq_limited_thr = 0;
			s5pv310_cpufreq_upper_limit(DVFS_LOCK_ID_TMU, CPU_L4);

			cpufreq_limited_warn = 1;
			if (tmu_tripped_cb(TMU_STATUS_WARNING) < 0)
				pr_err("Error inform to battery driver !\n");
			else
				pr_info("warning: set cpufreq upper limit.\n");
		}
		if (cur_temp <= tmu_info->temp->warn_low) {
			tmu_info->ctz->data.tmu_flag = TMU_STATUS_THROTTLED;
			s5pv310_cpufreq_upper_limit_free(DVFS_LOCK_ID_TMU);
			cpufreq_limited_warn = 0;
			s5pv310_cpufreq_upper_limit(DVFS_LOCK_ID_TMU, CPU_L2);
			cpufreq_limited_thr = 1;
			if (tmu_tripped_cb(TMU_STATUS_THROTTLED) < 0)
				pr_err("Error inform to battery driver !\n");
			else
				pr_info("warning->throttling:\
						up cpufreq upper limit.\n");
		}
		if (cur_temp >= TEMP_TRIPPED_CELCIUS) { /* 110 */
			tmu_info->ctz->data.tmu_flag = TMU_STATUS_TRIPPED;
			if (tmu_tripped_cb(TMU_STATUS_TRIPPED) < 0)
				pr_err("Error inform to battery driver !\n");
			else
				pr_info("warning->tripping:\
						waiting shutdown !!!\n");
		}
		break;

	case TMU_STATUS_TRIPPED:
		/* 1st throttling 110 */
		if (cur_temp >= TEMP_TRIPPED_CELCIUS && !(send_msg_battery)) {
			if (tmu_tripped_cb(TMU_STATUS_TRIPPED) < 0)
				pr_err("Error inform to battery driver !\n");
			else {
				pr_info("tripping: waiting shutdown.\n");
				send_msg_battery = 1;
			}
		}
		if (cur_temp >= (TEMP_MAX_CELCIUS - 5)) { /* 120 */
			panic("Emergency!!!!\
					tmu tripping event is not treated! \n");
		}

		if (cur_temp >= tmu_info->temp->trip_retry) {
			pr_warn("WARNING!!: try to send msg to\
					battery driver again\n");
			send_msg_battery = 0;
		}

		/* safety code */
		if (cur_temp <= tmu_info->temp->thr_low) {
			s5pv310_cpufreq_upper_limit_free(DVFS_LOCK_ID_TMU);
			tmu_info->ctz->data.tmu_flag = TMU_STATUS_NORMAL;
			cpufreq_limited_thr = 0;
			pr_info("tripping->normal:\
					Check! occured only test mode.\n");
		}
		break;

	case TMU_STATUS_INIT: /* sned tmu initial status to battery drvier */
		disable_irq(tmu_info->irq);

		if (cur_temp <= tmu_info->temp->thr_low)
			tmu_info->ctz->data.tmu_flag = TMU_STATUS_NORMAL;
		else
			tmu_info->ctz->data.tmu_flag = TMU_STATUS_THROTTLED;
		break;

	default:
		pr_warn("Bug: checked tmu_state.\n");
		if (cur_temp < tmu_info->temp->warn_high) {
			tmu_info->ctz->data.tmu_flag = TMU_STATUS_WARNING;
		} else {
			tmu_info->ctz->data.tmu_flag = TMU_STATUS_TRIPPED;
			if (tmu_tripped_cb(TMU_STATUS_TRIPPED) < 0)
				pr_err("Error inform to battery driver !\n");
		}
		break;
	} /* end */

	if (cur_temp >= TEMP_TQ0_CELCIUS) { /* 85 */
		if (!(auto_refresh_changed)) {
			pr_info("set auto_refresh 1.95us\n");
			set_refresh_rate(AUTO_REFRESH_PERIOD_TQ0);
			auto_refresh_changed = 1;
		}
	} else if (cur_temp <= (TEMP_TQ0_CELCIUS - 5)) { /* 80 */
		if (auto_refresh_changed) {
			pr_info("set auto_refresh 3.9us\n");
			set_refresh_rate(AUTO_REFRESH_PERIOD_NORMAL);
			auto_refresh_changed = 0;
		}
	}

	/* rescheduling next work */
	queue_delayed_work_on(0, tmu_monitor_wq, &tmu_info->polling_work,
			tmu_info->sampling_rate);

	mutex_unlock(&tmu_lock);

	return;
}

static int tmu_initialize(struct platform_device *pdev)
{
	struct s5p_tmu *tz = platform_get_drvdata(pdev);
	unsigned int en;
	unsigned int te_temp;

	__raw_writel(INTCLEAR2, tz->tmu_base + INTCLEAR);

	en = (__raw_readl(tz->tmu_base + TMU_STATUS) & 0x1);

	if (!en) {
		dev_err(&pdev->dev, "failed to start tmu drvier\n");
		return -ENOENT;
	}

	/* get the compensation parameter */
	te_temp = __raw_readl(tz->tmu_base + TRIMINFO);
	tz->data.te1 = te_temp & TRIM_TEMP_MASK;
	tz->data.te2 = ((te_temp >> 8) & TRIM_TEMP_MASK);

	pr_info("%s: te_temp = 0x%08x, low 8bit = %d, high 24 bit = %d\n",
			__func__, te_temp, tz->data.te1, tz->data.te2);

	if ((EFUSE_MIN_VALUE > tz->data.te1) || (tz->data.te1 > EFUSE_MAX_VALUE)
		||  (tz->data.te2 != 0))
		tz->data.te1 = EFUSE_AVG_VALUE;

	/* Need to initial regsiter setting after getting parameter info */
	/* [28:23] vref [11:8] slope - Tunning parameter */
	__raw_writel(VREF_SLOPE, tz->tmu_base + TMU_CON0);

	return 0;
}

static void tmu_start(struct platform_device *pdev)
{
	struct s5p_tmu *tz = platform_get_drvdata(pdev);
	unsigned int con;
	unsigned int thresh_temp_adc;

	__raw_writel(INTCLEARALL, tz->tmu_base + INTCLEAR);

#ifdef CONFIG_TMU_DEBUG_ENABLE
	if (set_tmu_test) {
		tmu_start_testmode(pdev);
		return;
	}
#endif

	pr_info("1st throttling stop_temp  = %d, start_temp = %d\n\
		2nd throttling stop_temp = %d, start_tmep = %d\n\
		tripping temp = %d, tripping retry_temp = %d\n\
		memory throttling stop_temp = %d, start_temp = %d\n",
		tmu_info->temp->thr_low, tmu_info->temp->thr_high - 4,
		tmu_info->temp->warn_low, tmu_info->temp->warn_high - 3,
		tmu_info->temp->trip_retry - 3, tmu_info->temp->trip_retry,
		tmu_info->temp->tq0_temp -5, tmu_info->temp->tq0_temp);

	/* Compensation temperature THD_TEMP */
	thresh_temp_adc = THD_TEMP + tz->data.te1 - TMU_CODE_25_DEGREE;
	pr_info("Compensated Threshold: 0x%2x\n", thresh_temp_adc);

	/* Set interrupt trigger level */
	__raw_writel(thresh_temp_adc, tz->tmu_base + THRESHOLD_TEMP);
	__raw_writel(TRIGGER_LEV0, tz->tmu_base + TRG_LEV0);
	__raw_writel(TRIGGER_LEV1, tz->tmu_base + TRG_LEV1);
	__raw_writel(TRIGGER_LEV2, tz->tmu_base + TRG_LEV2);
	__raw_writel(TRIGGER_LEV3, tz->tmu_base + TRG_LEV3);

	mdelay(50);
	/* TMU core enable */
	con = __raw_readl(tz->tmu_base + TMU_CON0);
	con |= TMU_EN;

	__raw_writel(con, tz->tmu_base + TMU_CON0);

	/*LEV0 LEV1 LEV2 interrupt enable */
	__raw_writel(INTEN0 | INTEN1 | INTEN2, tz->tmu_base + INTEN);

	pr_info("Cooling: %dc  THD_TEMP:0x%02x:  TRIG_LEV0: 0x%02x\
		TRIG_LEV1: 0x%02x TRIG_LEV2: 0x%02x\n",
		tz->data.cooling,
		THD_TEMP,
		THD_TEMP + TRIGGER_LEV0,
		THD_TEMP + TRIGGER_LEV1,
		THD_TEMP + TRIGGER_LEV2);
}

static irqreturn_t s5p_tmu_irq(int irq, void *id)
{
	struct s5p_tmu *tz = id;
	unsigned int status;

	disable_irq_nosync(irq);

	status = __raw_readl(tz->tmu_base + INTSTAT);

	pr_info("TMU interrupt occured : status = 0x%08x\n", status);

	if (status & INTSTAT2) {
		tz->data.tmu_flag = TMU_STATUS_TRIPPED;
		__raw_writel(INTCLEAR2, tz->tmu_base + INTCLEAR);
	} else if (status & INTSTAT1) {
		tz->data.tmu_flag = TMU_STATUS_WARNING;
		__raw_writel(INTCLEAR1, tz->tmu_base + INTCLEAR);
	} else if (status & INTSTAT0) {
		tz->data.tmu_flag = TMU_STATUS_THROTTLED;
		__raw_writel(INTCLEAR0, tz->tmu_base + INTCLEAR);
	} else {
		pr_err("%s: TMU interrupt error\n", __func__);
		__raw_writel(INTCLEARALL, tz->tmu_base + INTCLEAR);
		queue_delayed_work_on(0, tmu_monitor_wq,
			&tmu_info->polling_work, tmu_info->sampling_rate / 2);
		return -ENODEV;
	}

	queue_delayed_work_on(0, tmu_monitor_wq, &tmu_info->polling_work,
		tmu_info->sampling_rate);

	return IRQ_HANDLED;
}

static int __devinit s5p_tmu_probe(struct platform_device *pdev)
{
	struct s5p_tmu *tz = platform_get_drvdata(pdev);
	struct resource *res;
	int ret = 0;

	pr_debug("%s: probe=%p\n", __func__, pdev);

	tmu_info = kzalloc(sizeof(struct s5p_tmu_info), GFP_KERNEL);
	if (!tmu_info) {
		dev_err(&pdev->dev, "failed to alloc memory!\n");
		ret = -ENOMEM;
		goto err_nomem;
	}
	tmu_info->dev = &pdev->dev;
	tmu_info->ctz = tz;
	tmu_info->ctz->data.tmu_flag = TMU_STATUS_INIT;
	tmu_info->temp = &tmu_temp_band;
	/* To poll current temp, set sampling rate to ONE second sampling */
	tmu_info->sampling_rate  = usecs_to_jiffies(1000 * 1000);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "failed to get memory region resource\n");
		ret = -ENODEV;
		goto err_nores;
	}

	tmu_info->ioarea = request_mem_region(res->start,
					res->end-res->start+1,
					pdev->name);
	if (!(tmu_info->ioarea)) {
		dev_err(&pdev->dev, "failed to reserve memory region\n");
		ret = -EBUSY;
		goto err_nores;
	}

	tz->tmu_base = ioremap(res->start, (res->end - res->start) + 1);
	if (!(tz->tmu_base)) {
		dev_err(&pdev->dev, "failed ioremap()\n");
		ret = -EINVAL;
		goto err_nomap;
	}

	tmu_monitor_wq = create_freezeable_workqueue(dev_name(&pdev->dev));
	if (!tmu_monitor_wq) {
		pr_info("Creation of tmu_monitor_wq failed\n");
		return -EFAULT;
	}

#ifdef CONFIG_TMU_DEBUG_ENABLE
	if (set_sampling_rate) {
		tmu_info->sampling_rate =
			usecs_to_jiffies(set_sampling_rate * 1000);
		tmu_info->monitor_period =
			usecs_to_jiffies(set_sampling_rate * 10 * 1000);
	} else {
		/* 10sec monitroing */
		tmu_info->monitor_period = usecs_to_jiffies(10000 * 1000);
	}

	INIT_DELAYED_WORK_DEFERRABLE(&tmu_info->monitor_work, tmu_mon_timer);
	queue_delayed_work_on(0, tmu_monitor_wq, &tmu_info->monitor_work,
			tmu_info->monitor_period);
#endif

	INIT_DELAYED_WORK_DEFERRABLE(&tmu_info->polling_work, tmu_poll_timer);

	/* rescheduling next work to inform tmu status to battery driver */
	queue_delayed_work_on(0, tmu_monitor_wq, &tmu_info->polling_work,
		tmu_info->sampling_rate * 10);

	tmu_info->irq = platform_get_irq(pdev, 0);
	if (tmu_info->irq < 0) {
		dev_err(&pdev->dev, "no irq for thermal\n");
		ret = tmu_info->irq;
		goto err_irq;
	}

	ret = request_irq(tmu_info->irq, s5p_tmu_irq,
			IRQF_DISABLED,  "s5p-tmu interrupt", tz);
	if (ret) {
		dev_err(&pdev->dev, "IRQ%d error %d\n", tmu_info->irq, ret);
		goto err_irq;
	}

	ret = tmu_initialize(pdev);
	if (ret)
		goto err_init;

	tmu_start(pdev);

	return ret;

err_init:
	if (tmu_info->irq >= 0)
		free_irq(tmu_info->irq, tz);

err_irq:
	iounmap(tz->tmu_base);

err_nomap:
	release_resource(tmu_info->ioarea);
	kfree(tmu_info->ioarea);

err_nores:
	kfree(tmu_info);
	tmu_info = NULL;

err_nomem:
	dev_err(&pdev->dev, "initialization failed.\n");

	return ret;
}

static int __devinit s5p_tmu_remove(struct platform_device *pdev)
{
	struct s5p_tmu *tz = platform_get_drvdata(pdev);

	cancel_delayed_work(&tmu_info->polling_work);

	if (tmu_info->irq >= 0)
		free_irq(tmu_info->irq, tz);

	iounmap(tz->tmu_base);

	release_resource(tmu_info->ioarea);
	kfree(tmu_info->ioarea);

	kfree(tmu_info);
	tmu_info = NULL;

	pr_info("%s is removed\n", dev_name(&pdev->dev));
	return 0;
}

#ifdef CONFIG_PM
static int s5p_tmu_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct s5p_tmu *tz = platform_get_drvdata(pdev);

	/* save tmu register value */
	tmu_info->reg_save[0] = __raw_readl(tz->tmu_base + TMU_CON0);
	tmu_info->reg_save[1] = __raw_readl(tz->tmu_base + SAMPLING_INTERNAL);
	tmu_info->reg_save[2] = __raw_readl(tz->tmu_base + CNT_VALUE0);
	tmu_info->reg_save[3] = __raw_readl(tz->tmu_base + CNT_VALUE1);
	tmu_info->reg_save[4] = __raw_readl(tz->tmu_base + THRESHOLD_TEMP);
	tmu_info->reg_save[5] = __raw_readl(tz->tmu_base + INTEN);
	tmu_info->reg_save[6] = __raw_readl(tz->tmu_base + TRG_LEV0);
	tmu_info->reg_save[7] = __raw_readl(tz->tmu_base + TRG_LEV1);
	tmu_info->reg_save[8] = __raw_readl(tz->tmu_base + TRG_LEV2);
	tmu_info->reg_save[9] = __raw_readl(tz->tmu_base + TRG_LEV3);

	disable_irq(tmu_info->irq);

	return 0;
}

static int s5p_tmu_resume(struct platform_device *pdev)
{
	struct s5p_tmu *tz = platform_get_drvdata(pdev);

	/* save tmu register value */
	__raw_writel(tmu_info->reg_save[0], tz->tmu_base + TMU_CON0);
	__raw_writel(tmu_info->reg_save[1], tz->tmu_base + SAMPLING_INTERNAL);
	__raw_writel(tmu_info->reg_save[2], tz->tmu_base + CNT_VALUE0);
	__raw_writel(tmu_info->reg_save[3], tz->tmu_base + CNT_VALUE1);
	__raw_writel(tmu_info->reg_save[4], tz->tmu_base + THRESHOLD_TEMP);
	__raw_writel(tmu_info->reg_save[5], tz->tmu_base + INTEN);
	__raw_writel(tmu_info->reg_save[6], tz->tmu_base + TRG_LEV0);
	__raw_writel(tmu_info->reg_save[7], tz->tmu_base + TRG_LEV1);
	__raw_writel(tmu_info->reg_save[8], tz->tmu_base + TRG_LEV2);
	__raw_writel(tmu_info->reg_save[9], tz->tmu_base + TRG_LEV3);

	enable_irq(tmu_info->irq);

	return 0;
}
#else
#define s5p_tmu_suspend	NULL
#define s5p_tmu_resume	NULL
#endif

static struct platform_driver s5p_tmu_driver = {
	.probe		= s5p_tmu_probe,
	.remove		= s5p_tmu_remove,
	.suspend	= s5p_tmu_suspend,
	.resume		= s5p_tmu_resume,
	.driver		= {
		.name   = "s5p-tmu",
		.owner  = THIS_MODULE,
	},
};

static int __init s5p_tmu_driver_init(void)
{
	return platform_driver_register(&s5p_tmu_driver);
}

arch_initcall(s5p_tmu_driver_init);
