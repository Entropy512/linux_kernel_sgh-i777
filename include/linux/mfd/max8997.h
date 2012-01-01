/*
 * max8997.h - Voltage regulator driver for the Maxim 8997
 *
 *  Copyright (C) 2009-2010 Samsung Electrnoics
 *
 *  based on max8998.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __LINUX_MFD_MAX8997_H
#define __LINUX_MFD_MAX8997_H

#include <linux/regulator/machine.h>

/* MAX 8997 regulator ids */
enum {
	MAX8997_LDO1 = 1,
	MAX8997_LDO2,
	MAX8997_LDO3,
	MAX8997_LDO4,
	MAX8997_LDO5,
	MAX8997_LDO6,
	MAX8997_LDO7,
	MAX8997_LDO8,
	MAX8997_LDO9,
	MAX8997_LDO10,
	MAX8997_LDO11,
	MAX8997_LDO12,
	MAX8997_LDO13,
	MAX8997_LDO14,
	MAX8997_LDO15,
	MAX8997_LDO16,
	MAX8997_LDO17,
	MAX8997_LDO18,
	MAX8997_LDO21,
	MAX8997_BUCK1,
	MAX8997_BUCK2,
	MAX8997_BUCK3,
	MAX8997_BUCK4,
	MAX8997_BUCK5,
	MAX8997_BUCK6,
	MAX8997_BUCK7,
	MAX8997_EN32KHZ_AP,
	MAX8997_EN32KHZ_CP,
	MAX8997_ENVICHG,
	MAX8997_ESAFEOUT1,
	MAX8997_ESAFEOUT2,
	MAX8997_FLASH_CUR,
	MAX8997_MOVIE_CUR,
};

/**
 * max8997_regulator_data - regulator data
 * @id: regulator id
 * @initdata: regulator init data (contraints, supplies, ...)
 */
struct max8997_regulator_data {
	int				id;
	struct regulator_init_data	*initdata;
	int	(*is_valid_regulator)(int, struct regulator_init_data*);
};

struct max8997_power_data {
	int		(*set_charger)(int);
	int		(*topoff_cb) (void);
	unsigned	batt_detect:1;
	unsigned	topoff_threshold:2;
	unsigned	fast_charge:3;	/* charge current */
};

enum {
	MAX8997_MUIC_DETACHED = 0,
	MAX8997_MUIC_ATTACHED
};

enum {
	AP_USB_MODE = 0,
	CP_USB_MODE,
	AUDIO_MODE,
};

enum {
	UART_PATH_CP = 0,
	UART_PATH_AP,
};

typedef enum cable_type {
	CABLE_TYPE_NONE = 0,
	CABLE_TYPE_USB,
	CABLE_TYPE_OTG,
	CABLE_TYPE_TA,
	CABLE_TYPE_DESKDOCK,
	CABLE_TYPE_CARDOCK,
	CABLE_TYPE_JIG_UART_OFF,
	CABLE_TYPE_JIG_UART_OFF_VB,	/* VBUS enabled */
	CABLE_TYPE_JIG_UART_ON,
	CABLE_TYPE_JIG_USB_OFF,
	CABLE_TYPE_JIG_USB_ON,
	CABLE_TYPE_MHL,
	CABLE_TYPE_MHL_VB,
	CABLE_TYPE_UNKNOWN
} cable_type_t;

struct max8997_muic_data {
	void		(*usb_cb) (u8 attached);
	void		(*uart_cb) (u8 attached);
	int		(*charger_cb) (cable_type_t cable_type);
	void		(*deskdock_cb) (bool attached);
	void		(*cardock_cb) (bool attached);
	void		(*mhl_cb) (int attached);
	void		(*init_cb) (void);
	int		(*set_safeout) (int path);
	bool		(*is_mhl_attached) (void);
	int		(*cfg_uart_gpio) (void);
	void		(*jig_uart_cb) (int path);
	int			(*host_notify_cb) (int enable);
#if defined(CONFIG_TARGET_LOCALE_NA)
        int             gpio_uart_sel;
#else
	int 		gpio_usb_sel;
#endif /* CONFIG_TARGET_LOCALE_NA */
	int		sw_path;
	int		uart_path;
};

/**
 * struct max8997_board - packages regulator init data
 * @regulators: array of defined regulators
 * @num_regulators: number of regultors used
 * @irq_base: base IRQ number for max8997, required for IRQs
 * @ono: power onoff IRQ number for max8997
 * @wakeup: configure the irq as a wake-up source
 * @buck1_max_voltage1: BUCK1 maximum alowed voltage register 1
 * @buck1_max_voltage2: BUCK1 maximum alowed voltage register 2
 * @buck2_max_voltage: BUCK2 maximum alowed voltage
 * @buck1_set1: BUCK1 gpio pin 1 to set output voltage
 * @buck1_set2: BUCK1 gpio pin 2 to set output voltage
 * @buck2_set3: BUCK2 gpio pin to set output voltage
 * @buck1_ramp_en: enable BUCK1 RAMP
 * @buck2_ramp_en: enable BUCK2 RAMP
 * @buck3_ramp_en: enable BUCK3 RAMP
 * @buck4_ramp_en: enable BUCK4 RAMP
 * @buck_ramp_reg_val: value of BUCK RAMP register(0x15) BUCKRAMP[4:0]
 * @flash_cntl_val: value of MAX8997_REG_FLASH_CNTL register
 */
struct max8997_platform_data {
	struct max8997_regulator_data	*regulators;
	int				num_regulators;
	int				irq_base;
	int				ono;
	int				wakeup;
	int                             buck1_max_voltage1;
	int                             buck1_max_voltage2;
	int                             buck1_max_voltage3;
	int                             buck1_max_voltage4;
	int                             buck2_max_voltage1;
	int                             buck2_max_voltage2;
	int				buck_set1;
	int				buck_set2;
	int				buck_set3;
	bool				buck1_ramp_en;
	bool				buck2_ramp_en;
	bool				buck4_ramp_en;
	bool				buck5_ramp_en;
	int				buck_ramp_reg_val;
	int				flash_cntl_val;
	struct max8997_power_data	*power;
	struct max8997_muic_data	*muic;
};

#endif /*  __LINUX_MFD_MAX8997_H */
