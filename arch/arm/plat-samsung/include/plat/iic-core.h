/* arch/arm/mach-s3c2410/include/mach/iic-core.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * S3C - I2C Controller core functions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_IIC_CORE_H
#define __ASM_ARCH_IIC_CORE_H __FILE__

/* These functions are only for use with the core support code, such as
 * the cpu specific initialisation code
 */

/* re-define device name depending on support. */
static inline void s3c_i2c0_setname(char *name)
{
	/* currently this device is always compiled in */
	s3c_device_i2c0.name = name;
}

static inline void s3c_i2c1_setname(char *name)
{
#ifdef CONFIG_S3C_DEV_I2C1
	s3c_device_i2c1.name = name;
#endif
}

static inline void s3c_i2c2_setname(char *name)
{
#ifdef CONFIG_S3C_DEV_I2C2
	s3c_device_i2c2.name = name;
#endif
}
static inline void s3c_i2c3_setname(char *name)
{
#ifdef CONFIG_S3C_DEV_I2C3
	s3c_device_i2c3.name = name;
#endif
}
static inline void s3c_i2c4_setname(char *name)
{
#ifdef CONFIG_S3C_DEV_I2C4
	s3c_device_i2c4.name = name;
#endif
}
static inline void s3c_i2c5_setname(char *name)
{
#ifdef CONFIG_S3C_DEV_I2C5
	s3c_device_i2c5.name = name;
#endif
}
static inline void s3c_i2c6_setname(char *name)
{
#ifdef CONFIG_S3C_DEV_I2C6
	s3c_device_i2c6.name = name;
#endif
}
static inline void s3c_i2c7_setname(char *name)
{
#ifdef CONFIG_S3C_DEV_I2C7
	s3c_device_i2c7.name = name;
#endif
}

#endif /* __ASM_ARCH_IIC_H */
