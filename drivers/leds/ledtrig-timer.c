/*
 * LED Kernel Timer Trigger
 *
 * Copyright 2005-2006 Openedhand Ltd.
 *
 * Author: Richard Purdie <rpurdie@openedhand.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
 #ifdef CONFIG_TARGET_LOCALE_NA
#define CONFIG_RTC_LEDTRIG_TIMER 1//chief.rtc.ledtrig
#endif /* CONFIG_TARGET_LOCALE_NA */

#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/device.h>
#include <linux/sysdev.h>
#include <linux/timer.h>
#include <linux/ctype.h>
#include <linux/leds.h>
#include <linux/slab.h>
#include "leds.h"

 #ifdef CONFIG_TARGET_LOCALE_NA
#if defined(CONFIG_RTC_LEDTRIG_TIMER)
#include <linux/android_alarm.h>
#include <linux/wakelock.h>
#include <asm/mach/time.h>
#include <linux/hrtimer.h>
#include <linux/hrtimer.h>
#endif

#if defined(CONFIG_RTC_LEDTRIG_TIMER)
static struct wake_lock ledtrig_rtc_timer_wakelock;
#endif
#endif /* CONFIG_TARGET_LOCALE_NA */

struct timer_trig_data {
	int brightness_on;		/* LED brightness during "on" period.
					 * (LED_OFF < brightness_on <= LED_FULL)
					 */
	unsigned long delay_on;		/* milliseconds on */
	unsigned long delay_off;	/* milliseconds off */
	struct timer_list timer;
#ifdef CONFIG_TARGET_LOCALE_NA
#if (CONFIG_RTC_LEDTRIG_TIMER==1)
	struct alarm alarm;
	struct wake_lock wakelock;
#elif (CONFIG_RTC_LEDTRIG_TIMER==2)
	struct hrtimer hrtimer;
#endif
#endif /* CONFIG_TARGET_LOCALE_NA */
};

#ifdef CONFIG_TARGET_LOCALE_NA
#if (CONFIG_RTC_LEDTRIG_TIMER==1)
static int led_rtc_set_alarm(struct led_classdev *led_cdev, unsigned long msec)
{
	struct timer_trig_data *timer_data = led_cdev->trigger_data;
	ktime_t expire;
	ktime_t now;
	
	now = ktime_get_real();
	expire = ktime_add(now, ns_to_ktime((u64)msec*1000*1000));

	alarm_start_range(&timer_data->alarm, expire, expire);
	if(msec < 1500) {
		/* If expire time is less than 1.5s keep a wake lock to prevent constant
		 * suspend fail. RTC alarm fails to suspend if the earliest expiration
		 * time is less than a second. Keep the wakelock just a jiffy more than
		 * the expire time to prevent wake lock timeout. */
		wake_lock_timeout(&timer_data->wakelock, (msec*HZ/1000)+1);
	}
	return 0;
}

static void led_rtc_timer_function(struct alarm *alarm)
{
	struct timer_trig_data *timer_data = container_of(alarm, struct timer_trig_data, alarm);

	/* let led_timer_function do the actual work */
	mod_timer(&timer_data->timer, jiffies + 1);
}
#elif (CONFIG_RTC_LEDTRIG_TIMER==2)
extern int msm_pm_request_wakeup(struct hrtimer *timer);
static enum hrtimer_restart ledtrig_hrtimer_function(struct hrtimer *timer)
{
	struct timer_trig_data *timer_data = container_of(timer, struct timer_trig_data, hrtimer);
	
	/* let led_timer_function do the actual work */
	mod_timer(&timer_data->timer, jiffies + 1);
}
#endif
#endif /* CONFIG_TARGET_LOCALE_NA */

static void led_timer_function(unsigned long data)
{
	struct led_classdev *led_cdev = (struct led_classdev *) data;
	struct timer_trig_data *timer_data = led_cdev->trigger_data;
	unsigned long brightness;
	unsigned long delay;

	if (!timer_data->delay_on || !timer_data->delay_off) {
		led_set_brightness(led_cdev, LED_OFF);
		return;
	}

	brightness = led_get_brightness(led_cdev);
	if (!brightness) {
		/* Time to switch the LED on. */
		brightness = timer_data->brightness_on;
		delay = timer_data->delay_on;
	} else {
		/* Store the current brightness value to be able
		 * to restore it when the delay_off period is over.
		 */
		timer_data->brightness_on = brightness;
		brightness = LED_OFF;
		delay = timer_data->delay_off;
	}

	led_set_brightness(led_cdev, brightness);
#ifdef CONFIG_TARGET_LOCALE_NA	
#if (CONFIG_RTC_LEDTRIG_TIMER==1)
	led_rtc_set_alarm(led_cdev, delay);
#elif (CONFIG_RTC_LEDTRIG_TIMER==2)
	hrtimer_start(&timer_data->hrtimer, ns_to_ktime((u64)delay*1000*1000), HRTIMER_MODE_REL);
#else
	mod_timer(&timer_data->timer, jiffies + msecs_to_jiffies(delay));
#endif
#else
	mod_timer(&timer_data->timer, jiffies + msecs_to_jiffies(delay));
#endif /* CONFIG_TARGET_LOCALE_NA */

}

static ssize_t led_delay_on_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct timer_trig_data *timer_data = led_cdev->trigger_data;

	return sprintf(buf, "%lu\n", timer_data->delay_on);
}

static ssize_t led_delay_on_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct timer_trig_data *timer_data = led_cdev->trigger_data;
	int ret = -EINVAL;
	char *after;
	unsigned long state = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;

	if (isspace(*after))
		count++;

	if (count == size) {
		if (timer_data->delay_on != state) {
			/* the new value differs from the previous */
			timer_data->delay_on = state;

			/* deactivate previous settings */
			del_timer_sync(&timer_data->timer);

			/* try to activate hardware acceleration, if any */
			if (!led_cdev->blink_set ||
			    led_cdev->blink_set(led_cdev,
			      &timer_data->delay_on, &timer_data->delay_off)) {
				/* no hardware acceleration, blink via timer */
				mod_timer(&timer_data->timer, jiffies + 1);
			}
		}
		ret = count;
	}

	return ret;
}

static ssize_t led_delay_off_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct timer_trig_data *timer_data = led_cdev->trigger_data;

	return sprintf(buf, "%lu\n", timer_data->delay_off);
}

static ssize_t led_delay_off_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct timer_trig_data *timer_data = led_cdev->trigger_data;
	int ret = -EINVAL;
	char *after;
	unsigned long state = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;

	if (isspace(*after))
		count++;

	if (count == size) {
		if (timer_data->delay_off != state) {
			/* the new value differs from the previous */
			timer_data->delay_off = state;

			/* deactivate previous settings */
			del_timer_sync(&timer_data->timer);

			/* try to activate hardware acceleration, if any */
			if (!led_cdev->blink_set ||
			    led_cdev->blink_set(led_cdev,
			      &timer_data->delay_on, &timer_data->delay_off)) {
				/* no hardware acceleration, blink via timer */
				mod_timer(&timer_data->timer, jiffies + 1);
			}
		}
		ret = count;
	}

	return ret;
}

static DEVICE_ATTR(delay_on, 0644, led_delay_on_show, led_delay_on_store);
static DEVICE_ATTR(delay_off, 0644, led_delay_off_show, led_delay_off_store);

static void timer_trig_activate(struct led_classdev *led_cdev)
{
	struct timer_trig_data *timer_data;
	int rc;

	timer_data = kzalloc(sizeof(struct timer_trig_data), GFP_KERNEL);
	if (!timer_data)
		return;

	timer_data->brightness_on = led_get_brightness(led_cdev);
	if (timer_data->brightness_on == LED_OFF)
		timer_data->brightness_on = led_cdev->max_brightness;
	led_cdev->trigger_data = timer_data;

	init_timer(&timer_data->timer);
	timer_data->timer.function = led_timer_function;
	timer_data->timer.data = (unsigned long) led_cdev;
#ifdef CONFIG_TARGET_LOCALE_NA		
#if (CONFIG_RTC_LEDTRIG_TIMER==1)
	alarm_init(&timer_data->alarm, ANDROID_ALARM_RTC_WAKEUP,
			led_rtc_timer_function);
	wake_lock_init(&timer_data->wakelock, WAKE_LOCK_SUSPEND, "ledtrig_rtc_timer");
#elif (CONFIG_RTC_LEDTRIG_TIMER==2)
	hrtimer_init(&timer_data->hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	timer_data->hrtimer.function = ledtrig_hrtimer_function;
	msm_pm_request_wakeup(&timer_data->hrtimer);
#endif
#endif /* CONFIG_TARGET_LOCALE_NA */
	rc = device_create_file(led_cdev->dev, &dev_attr_delay_on);
	if (rc)
		goto err_out;
	rc = device_create_file(led_cdev->dev, &dev_attr_delay_off);
	if (rc)
		goto err_out_delayon;

	/* If there is hardware support for blinking, start one
	 * user friendly blink rate chosen by the driver.
	 */
	if (led_cdev->blink_set)
		led_cdev->blink_set(led_cdev,
			&timer_data->delay_on, &timer_data->delay_off);

	return;

err_out_delayon:
	device_remove_file(led_cdev->dev, &dev_attr_delay_on);
err_out:
	led_cdev->trigger_data = NULL;
	kfree(timer_data);
}

static void timer_trig_deactivate(struct led_classdev *led_cdev)
{
	struct timer_trig_data *timer_data = led_cdev->trigger_data;
	unsigned long on = 0, off = 0;

	if (timer_data) {
		device_remove_file(led_cdev->dev, &dev_attr_delay_on);
		device_remove_file(led_cdev->dev, &dev_attr_delay_off);
		del_timer_sync(&timer_data->timer);
#ifdef CONFIG_TARGET_LOCALE_NA		
#if (CONFIG_RTC_LEDTRIG_TIMER==1)
		alarm_cancel(&timer_data->alarm);
		wake_lock_destroy(&timer_data->wakelock);
#elif (CONFIG_RTC_LEDTRIG_TIMER==2)
		msm_pm_request_wakeup(NULL);
		hrtimer_cancel(&timer_data->hrtimer);
#endif
#endif /* CONFIG_TARGET_LOCALE_NA */
		kfree(timer_data);
	}

	/* If there is hardware support for blinking, stop it */
	if (led_cdev->blink_set)
		led_cdev->blink_set(led_cdev, &on, &off);
}

static struct led_trigger timer_led_trigger = {
	.name     = "timer",
	.activate = timer_trig_activate,
	.deactivate = timer_trig_deactivate,
};

static int __init timer_trig_init(void)
{
	return led_trigger_register(&timer_led_trigger);
}

static void __exit timer_trig_exit(void)
{
	led_trigger_unregister(&timer_led_trigger);
}

module_init(timer_trig_init);
module_exit(timer_trig_exit);

MODULE_AUTHOR("Richard Purdie <rpurdie@openedhand.com>");
MODULE_DESCRIPTION("Timer LED trigger");
MODULE_LICENSE("GPL");
