/*
 * regulator.h --  S5M8751 Advanced PMIC with AUDIO DAC
 *
 * Copyright 2009 Samsung Electronics.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#ifndef __LINUX_S5M8751_REGULATOR_H_
#define __LINUX_S5M8751_REGULATOR_H_

#define NUM_S5M8751_REGULATORS			10

/* LDOs */
#define S5M8751_LDO1				0
#define S5M8751_LDO2				1
#define S5M8751_LDO3				2
#define S5M8751_LDO4				3
#define S5M8751_LDO5				4
#define S5M8751_LDO6				5

/* BUCKs */
#define S5M8751_BUCK1_1				6
#define S5M8751_BUCK1_2				7
#define S5M8751_BUCK2_1				8
#define S5M8751_BUCK2_2				9

/* ONOFF1 (0x04) */
#define S5M8751_SLEEPB_PIN_EN			(1 << 1)
#define S5M8751_SLEEP_I2C			(1 << 0)

/* ONOFF2 (0x05) */
#define S5M8751_LDO5_SHIFT			0
#define S5M8751_LDO1_SHIFT			1
#define S5M8751_LDO2_SHIFT			2
#define S5M8751_LDO3_SHIFT			3
#define S5M8751_LDO4_SHIFT			4

/* ONOFF3 (0x06) */
#define S5M8751_LDO6_SHIFT			0
#define S5M8751_BUCK1_SHIFT			1
#define S5M8751_BUCK2_SHIFT			2

/* LDO6_VSET (0x0A) */
#define S5M8751_LDO6_VSET_MASK			0x0F

/* LDO1_VSET (0x0B) */
#define S5M8751_LDO1_VSET_MASK			0x0F

/* LDO2_VSET (0x0C) */
#define S5M8751_LDO2_VSET_MASK			0x0F

/* LDO3_VSET (0x0D) */
#define S5M8751_LDO3_VSET_MASK			0x0F

/* LDO4_VSET (0x0E) */
#define S5M8751_LDO4_VSET_MASK			0x0F

/* LDO5_VSET (0x0F) */
#define S5M8751_LDO5_VSET_MASK			0x0F

/* BUCK1_V1_SET (0x10) */
#define S5M8751_BUCK1_V1_SET_MASK		0x3F
#define S5M8751_BUCK_RAMP_TIME_SHIFT		6

/* BUCK1 V2 SET (0x11) */
#define S5M8751_BUCK1_V2_SET_MASK		0x3F
#define S5M8751_BUCK_MODE_SHIFT			6
#define S5M8751_BUCK_PFM_MODE			0
#define S5M8751_BUCK_PWM_MODE			1
#define S5M8751_BUCK_AUTO_MODE			2


/* BUCK2_V1_SET (0x10) */
#define S5M8751_BUCK2_V1_SET_MASK		0x3F
#define S5M8751_BUCK_RAMP_TIME_SHIFT		6

/* BUCK2 V2 SET (0x11) */
#define S5M8751_BUCK2_V2_SET_MASK		0x3F
#define S5M8751_BUCK_MODE_SHIFT			6

#endif

