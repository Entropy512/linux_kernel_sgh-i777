/*
 * pdata.h  --  S5M8751 Advanced PMIC with AUDIO DAC
 *
 * Copyright 2009 Samsung Electronics.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#ifndef __MFD_S5M8751_PDATA_H__
#define __MFD_S5M8751_PDATA_H__

struct s5m8751;
struct regulator_init_data;

struct s5m8751_backlight_pdata {
	int brightness;
};

struct s5m8751_power_pdata {
	int constant_voltage;
	int fast_chg_current;
	unsigned int gpio_hadp_lusb;
};

struct s5m8751_audio_pdata {
	uint8_t dac_vol;
	uint8_t hp_vol;
};

#define S5M8751_MAX_REGULATOR			10

struct s5m8751_pdata {
	struct regulator_init_data *regulator[S5M8751_MAX_REGULATOR];
	struct s5m8751_backlight_pdata *backlight;
	struct s5m8751_power_pdata *power;
	struct s5m8751_audio_pdata *audio;

	int irq_base;
	int gpio_pshold;
};

#endif
