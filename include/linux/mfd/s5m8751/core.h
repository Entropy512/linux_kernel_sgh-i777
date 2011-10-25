/*
 * core.h  --  S5M8751 Advanced PMIC with AUDIO DAC
 *
 * Copyright (c) 2009 Samsung Electronics
 *
 * Author: Jongbin Won <jongbin.won@samsung.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#ifndef __LINUX_MFD_S5M8751_CORE_H_
#define __LINUX_MFD_S5M8751_CORE_H_

#include <linux/mfd/s5m8751/pdata.h>
#include <linux/interrupt.h>

/*
 * Register values.
 */
#define S5M8751_FRT_REGISTER			0x00
#define S5M8751_LAT_REGISTER			0x45
#define S5M8751_SLAVE_ADDRESS			0xD0

#define S5M8751_IRQB_EVENT1			0x00
#define	S5M8751_IRQB_EVENT2			0x01
#define S5M8751_IRQB_MASK1			0x02
#define S5M8751_IRQB_MASK2			0x03

#define S5M8751_ONOFF1				0x04
#define S5M8751_ONOFF2				0x05
#define S5M8751_ONOFF3				0x06

#define S5M8751_SLEEP_CNTL1			0x07
#define S5M8751_SLEEP_CNTL2			0x08
#define S5M8751_UVLO				0x09

#define S5M8751_LDO6_VSET			0x0A
#define S5M8751_LDO1_VSET			0x0B
#define S5M8751_LDO2_VSET			0x0C
#define S5M8751_LDO3_VSET			0x0D
#define S5M8751_LDO4_VSET			0x0E
#define S5M8751_LDO5_VSET			0x0F
#define S5M8751_BUCK1_V1_SET			0x10
#define S5M8751_BUCK1_V2_SET			0x11
#define S5M8751_BUCK2_V1_SET			0x12
#define S5M8751_BUCK2_V2_SET			0x13

#define S5M8751_WLED_CNTRL			0x14
#define S5M8751_CHG_IV_SET			0x15
#define S5M8751_CHG_CNTRL			0x16

#define S5M8751_DA_PDB1				0x17
#define S5M8751_DA_AMIX1			0x18
#define S5M8751_DA_AMIX2			0x19
#define S5M8751_DA_ANA				0x1A
#define S5M8751_DA_DWA				0x1B
#define S5M8751_DA_VOLL				0x1C
#define S5M8751_DA_VOLR				0x1D
#define S5M8751_DA_DIG1				0x1E
#define S5M8751_DA_DIG2				0x1F
#define S5M8751_DA_LIM1				0x20
#define S5M8751_DA_LIM2				0x21
#define S5M8751_DA_LOF				0x22
#define S5M8751_DA_ROF				0x23
#define S5M8751_DA_MUX				0x24
#define S5M8751_DA_LGAIN			0x25
#define S5M8751_DA_RGAIN			0x26
#define S5M8751_IN1_CTRL1			0x27
#define S5M8751_IN1_CTRL2			0x28
#define S5M8751_IN1_CTRL3			0x29
#define S5M8751_SLOT_L2				0x2A
#define S5M8751_SLOT_L1				0x2B
#define S5M8751_SLOT_R2				0x2C
#define S5M8751_SLOT_R1				0x2D
#define S5M8751_TSLOT				0x2E
#define S5M8751_TEST				0x2F

#define S5M8751_SPK_SLOPE			0x30
#define S5M8751_SPK_DT				0x31
#define S5M8751_SPK_S2D				0x32
#define S5M8751_SPK_CM				0x33
#define S5M8751_SPK_DUM				0x34
#define S5M8751_HP_VOL1				0x35
#define S5M8751_HP_VOL2				0x36
#define S5M8751_AMP_EN				0x37
#define S5M8751_AMP_MUTE			0x38
#define S5M8751_AMP_CTRL			0x39
#define S5M8751_AMP_VMID			0x3A
#define S5M8751_LINE_CTRL			0x3B

#define S5M8751_CHG_TEST_WR			0x3C
#define S5M8751_CHG_TRIM			0x3D
#define S5M8751_BUCK_TEST1			0x3E
#define S5M8751_VREF_TEST			0x3F
#define S5M8751_BUCK_TEST2			0x40
#define S5M8751_LDO_OCPEN			0x42
#define S5M8751_CHIP_ID				0x43
#define S5M8751_STATUS				0x44
#define S5M8751_AUDIO_STATUS			0x45
#define S5M8751_CHG_TEST_R			0x46

#define S5M8751_NUMREGS				(S5M8751_CHG_TEST_R+1)
#define S5M8751_MAX_REGISTER			0xFF

/* S5M8751 IRQ events */
#define S5M8751_IRQ_PWRKEY1B			0
#define S5M8751_MASK_PWRKEY1B			0x08
#define S5M8751_IRQ_PWRKEY2B			1
#define S5M8751_MASK_PWRKEY2B			0x04
#define S5M8751_IRQ_PWRKEY3			2
#define S5M8751_MASK_PWRKEY3			0x02
#define S5M8751_IRQ_VCHG_DETECTION		3
#define S5M8751_MASK_VCHG_DET			0x10
#define S5M8751_IRQ_VCHG_REMOVAL		4
#define S5M8751_MASK_VCHG_REM			0x08
#define S5M8751_IRQ_VCHG_TIMEOUT		5
#define S5M8751_MASK_CHG_T_OUT			0x04
#define S5M8751_NUM_IRQ				6

/* S5M8751 SLEEPB_PIN enable */
#define S5M8751_SLEEPB_PIN_ENABLE		0x02
#define S5M8751_SLEEPB_ENABLE			1
#define S5M8751_SLEEPB_DISABLE			0

struct s5m8751;

struct s5m8751 {
	struct device *dev;

	/* device IO */
	struct i2c_client *i2c_client;

	int (*read_dev)(struct s5m8751 *s5m8751, uint8_t reg, uint8_t *val);
	int (*write_dev)(struct s5m8751 *s5m8751, uint8_t reg, uint8_t val);

	int (*read_block_dev)(struct s5m8751 *s5m8751, uint8_t reg, int len,
							uint8_t *val);
	int (*write_block_dev)(struct s5m8751 *s5m8751, uint8_t reg, int len,
							uint8_t *val);
	uint8_t *reg_cache;

	void *control_data;

	/* Interrupt handling */
	struct mutex irq_lock;
	int irq_base;
	int core_irq;
};

/*
 * S5M8751 device initialisation and exit.
 */
int s5m8751_device_init(struct s5m8751 *s5m8751, int irq,
						struct s5m8751_pdata *pdata);
void s5m8751_device_exit(struct s5m8751 *s5m8751);

/*
 * S5M8751 device IO
 */
int s5m8751_clear_bits(struct s5m8751 *s5m8751, uint8_t reg, uint8_t mask);
int s5m8751_set_bits(struct s5m8751 *s5m8751, uint8_t reg, uint8_t mask);
int s5m8751_reg_read(struct s5m8751 *s5m8751, uint8_t reg, uint8_t *val);
int s5m8751_reg_write(struct s5m8751 *s5m8751, uint8_t reg, uint8_t val);
int s5m8751_block_read(struct s5m8751 *s5m8751, uint8_t reg, int len,
							uint8_t *val);
int s5m8751_block_write(struct s5m8751 *s5m8751, uint8_t reg, int len,
							uint8_t *val);

#endif /* __LINUX_MFD_S5M8751_H_ */
