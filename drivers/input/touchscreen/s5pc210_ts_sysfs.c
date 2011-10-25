/*
 * Touch Sensor Interface driver
 */
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/sysfs.h>

#include "s5pc210_ts_gpio_i2c.h"
#include "s5pc210_ts.h"

/* function prototype define */
int s5pv310_ts_sysfs_create (struct platform_device *pdev);
void s5pv310_ts_sysfs_remove (struct platform_device *pdev);

/*  sysfs function prototype define */
/*  screen hold control (on -> hold, off -> normal mode) */
static ssize_t show_hold_state (struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t set_hold_state (struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static DEVICE_ATTR(hold_state, S_IRWXUGO, show_hold_state, set_hold_state);

/* touch sampling rate control (5, 10, 20 : unit msec) */
static ssize_t show_sampling_rate (struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t set_sampling_rate (struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static DEVICE_ATTR(sampling_rate, S_IRWXUGO, show_sampling_rate, set_sampling_rate);

/* touch threshold control (range 0 - 10) : default 3 */
#define	THRESHOLD_MAX	10

static 	ssize_t show_threshold_x		(struct device *dev, struct device_attribute *attr, char *buf);
static 	ssize_t set_threshold_x			(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static	DEVICE_ATTR(threshold_x, S_IRWXUGO, show_threshold_x, set_threshold_x);

static 	ssize_t show_threshold_y		(struct device *dev, struct device_attribute *attr, char *buf);
static 	ssize_t set_threshold_y			(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static	DEVICE_ATTR(threshold_y, S_IRWXUGO, show_threshold_y, set_threshold_y);

/* touch calibration */
#if defined(CONFIG_TOUCHSCREEN_S5PV310_MT)
static ssize_t set_ts_cal(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static DEVICE_ATTR(ts_cal, S_IWUGO, NULL, set_ts_cal);
#endif
static struct attribute *s5pv310_ts_sysfs_entries[] = {
	&dev_attr_hold_state.attr,
	&dev_attr_sampling_rate.attr,
	&dev_attr_threshold_x.attr,
	&dev_attr_threshold_y.attr,
#if defined(CONFIG_TOUCHSCREEN_S5PV310_MT)
	&dev_attr_ts_cal.attr,
#endif
	NULL
};

static struct attribute_group s5pv310_ts_attr_group = {
	.name   = NULL,
	.attrs  = s5pv310_ts_sysfs_entries,
};
static 	ssize_t show_hold_state (struct device *dev, struct device_attribute *attr, char *buf)
{
	if(s5pv310_ts.hold_status)
		return	sprintf(buf, "on\n");
	else
		return	sprintf(buf, "off\n");
}
static ssize_t set_hold_state (struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long flags;
	unsigned char wdata;

	local_irq_save(flags);

	if(!strcmp(buf, "on\n"))
		s5pv310_ts.hold_status = 1;
	else {
#if defined(CONFIG_TOUCHSCREEN_S5PV310_MT)
		/* INT_mode : disable interrupt, low-active, finger moving */
		wdata = 0x01;
		s5pv310_ts_write(MODULE_INTMODE, &wdata, 1);
		mdelay(10);
		/* INT_mode : enable interrupt, low-active, finger moving */
		wdata = 0x09;
		s5pv310_ts_write(MODULE_INTMODE, &wdata, 1);
		mdelay(10);
#endif
		s5pv310_ts.hold_status = 0;
	}

	local_irq_restore(flags);

	return count;
}
static ssize_t show_sampling_rate (struct device *dev, struct device_attribute *attr, char *buf)
{
	switch(s5pv310_ts.sampling_rate) {
	default	:
		s5pv310_ts.sampling_rate = 0;
	case	0:
		return	sprintf(buf, "10 msec\n");
	case	1:
		return	sprintf(buf, "20 msec\n");
	case	2:
		return	sprintf(buf, "50 msec\n");
	}
}

static ssize_t set_sampling_rate(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long 	flags;
	unsigned int	val;

	if(!(sscanf(buf, "%u\n", &val)))
		return -EINVAL;

	local_irq_save(flags);
	if(val > 20)
		s5pv310_ts.sampling_rate = 2;
	else if(val > 10)
		s5pv310_ts.sampling_rate = 1;
	else
		s5pv310_ts.sampling_rate = 0;

	local_irq_restore(flags);

	return count;
}
static ssize_t show_threshold_x(struct device *dev, struct device_attribute *attr, char *buf)
{
	if(s5pv310_ts.threshold_x > THRESHOLD_MAX)
		s5pv310_ts.threshold_x = THRESHOLD_MAX;

	return	sprintf(buf, "%d\n", s5pv310_ts.threshold_x);
}

static ssize_t set_threshold_x(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long 	flags;
	unsigned int	val;

	if(!(sscanf(buf, "%u\n", &val)))	return	-EINVAL;

	local_irq_save(flags);

	if(val < 0)
		val *= (-1);

	if(val > THRESHOLD_MAX)
		val = THRESHOLD_MAX;

	s5pv310_ts.threshold_x = val;

	local_irq_restore(flags);

	return count;
}
static ssize_t show_threshold_y(struct device *dev, struct device_attribute *attr, char *buf)
{
	if(s5pv310_ts.threshold_y > THRESHOLD_MAX)
		s5pv310_ts.threshold_y = THRESHOLD_MAX;

	return	sprintf(buf, "%d\n", s5pv310_ts.threshold_y);
}
static ssize_t set_threshold_y(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long flags;
	unsigned int val;

	if(!(sscanf(buf, "%u\n", &val)))
		return	-EINVAL;

	local_irq_save(flags);

	if(val < 0)
		val *= (-1);

	if(val > THRESHOLD_MAX)
		val = THRESHOLD_MAX;

	s5pv310_ts.threshold_y = val;

	local_irq_restore(flags);

	return count;
}

#if defined(CONFIG_TOUCHSCREEN_S5PV310_MT)
static ssize_t set_ts_cal (struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{

	unsigned char wdata;
	unsigned long flags;

	local_irq_save(flags);

	/* INT_mode : disable interrupt */
	wdata = 0x00;
	s5pv310_ts_write(MODULE_INTMODE, &wdata, 1);

	/* touch calibration */

	wdata =0x03;
	s5pv310_ts_write(MODULE_CALIBRATION, &wdata, 1);	/* set mode */

	mdelay(500);

	/* INT_mode : enable interrupt, low-active, periodically*/

	wdata = 0x09;
	s5pv310_ts_write(MODULE_INTMODE, &wdata, 1);

	local_irq_restore(flags);

	return count;
}

#endif

int s5pv310_ts_sysfs_create (struct platform_device *pdev)
{
	/* variable init */

	s5pv310_ts.hold_status = 0;

	s5pv310_ts.sampling_rate	= 0;	/* 5 msec sampling */
	s5pv310_ts.threshold_x	= TS_X_THRESHOLD;	/* x data threshold (0~10) */
	s5pv310_ts.threshold_y	= TS_Y_THRESHOLD;	/* y data threshold (0~10) */

	return	sysfs_create_group(&pdev->dev.kobj, &s5pv310_ts_attr_group);
}
void s5pv310_ts_sysfs_remove (struct platform_device *pdev)
{
	sysfs_remove_group(&pdev->dev.kobj, &s5pv310_ts_attr_group);
}
