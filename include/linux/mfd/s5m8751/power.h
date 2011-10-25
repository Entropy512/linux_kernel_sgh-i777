/*
 * power.h  --  S5M8751 Power-Audio IC ALSA Soc Audio driver
 *
 * Copyright 2009 Samsung Electronics.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#ifndef __LINUX_MFD_S5M8751_POWER_H_
#define __LINUX_MFD_S5M8751_POWER_H_

/* Battery Charger Interface */
#define S5M8751_POWER				12
#define S5M8751_PWR_USB				0
#define S5M8751_PWR_WALL			1

/*
 * CHG_IV_SET (0x15)
 */
#define S5M8751_CONSTANT_VOLTAGE_MASK		0xE0
#define S5M8751_CHG_MODE_MASK			0x10
#define S5M8751_FAST_CHG_CURRENT_MASK		0x0F
#define S5M8751_CHG_MANUAL_MODE			0x10
#define S5M8751_CHG_AUTO_MODE			0x10

#define S5M8751_CHG_CONT_VOLTAGE(x)		(((x - 4100) / 25) << 5)
#define S5M8751_FAST_CHG_CURRENT(x)		((x - 50) / 50)

/*
 * CHG_TEST_WR (0x3C)
 */
#define S5M8751_CHG_TEST_WR			0x3C

/*
 * STATUS (0x44)
 */
#define S5M8751_CHG_STATE_OFF			0
#define S5M8751_CHG_STATE_ON			1
#define S5M8751_CHG_STATUS_MASK			0x1

/*
 * Charger Status (0x48)
 */
#define S5M8751_CHG_STATE_TRICKLE		0
#define S5M8751_CHG_STATE_FAST			1
#define S5M8751_CHG_TYPE_MASK			0x10

#endif
