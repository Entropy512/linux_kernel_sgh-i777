/*
 * wled.h  --  S5M8751 Advanced PMIC with AUDIO DAC
 *
 * Copyright 2009 Samsung Electronics.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#ifndef __LINUX_MFD_S5M8751_WLED_H_
#define __LINUX_MFD_S5M8751_WLED_H_

#include <linux/platform_device.h>
#include <linux/backlight.h>

#define pwm_600kHz				0
#define pwm_1MHz				1

#define WLED_OFF				0
#define WLED_ON					1

/* Backlight Interface */
#define S5M8751_WLED				10
#define S5M8751_WLED_EN				0x80
#define S5M8751_WLED_CCFDIM			0x1F
#define S5M8751_WLED_CCFDIM_SHIFT		0
#define S5M8751_WLED_TRIM(x)			((x) & S5M8751_WLED_CCFDIM)
#define S5M8751_WLED_MAX_BRIGHTNESS		0x1F
#define S5M8751_WLED_DEF_BRIGHTNESS		0x1F

struct s5m8751_wled {
	struct platform_device *pdev;
	struct backlight_properties *properties;
};

extern struct backlight_properties s5m8751_wled_data;

#endif
