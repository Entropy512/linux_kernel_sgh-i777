/* linux/include/linux/regulator/samsung_pd.h
 *
 * Based on linux/include/linux/regulator/fixed.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __REGULATOR_SAMSUNG_PD_H
#define __REGULATOR_SAMSUNG_PD_H

struct regulator_init_data;

struct clk_should_be_running {
	const char* clk_name;
	int id;
};

/**
 * struct samsung_pd_config - samsung_pd_config structure
 * @supply_name:		Name of the regulator supply
 * @microvolts:			Output voltage of regulator
 * @startup_delay:		Start-up time in microseconds
 * @enabled_at_boot:		Whether regulator has been enabled at
 *				boot or not. 1 = Yes, 0 = No
 *				This is used to keep the regulator at
 *				the default state
 * @init_data:			regulator_init_data
 * @clk_should_be_running:	the clocks for the IPs in the power domain
 *				should be running when the power domain
 *				is turned on
 * @enable:			regulator enable function
 * @disable:			regulator disable function
 *
 * This structure contains samsung power domain regulator configuration
 * information that must be passed by platform code to the samsung
 * power domain regulator driver.
 */
struct samsung_pd_config {
	const char *supply_name;
	int microvolts;
	unsigned startup_delay;
	unsigned enabled_at_boot:1;
	struct regulator_init_data *init_data;
	struct clk_should_be_running *clk_run;
	void (*enable)(void);
	void (*disable)(void);
};

#endif
