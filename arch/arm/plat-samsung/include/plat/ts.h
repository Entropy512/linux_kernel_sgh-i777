/* arch/arm/plat-samsung/include/plat/ts.h
 *
 * Copyright (c) 2005 Arnaud Patard <arnaud.patard@rtp-net.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ASM_ARM_TS_H
#define __ASM_ARM_TS_H

#ifdef CONFIG_HAS_WAKELOCK
#include <linux/wakelock.h>
#include <linux/earlysuspend.h>
#endif

struct s3c2410_ts_mach_info {
    int             delay;
    int             presc;
    int             oversampling_shift;
    void    (*cfg_gpio)(struct platform_device *dev);
};

extern void s3c24xx_ts_set_platdata(struct s3c2410_ts_mach_info *);
extern void s3c24xx_ts1_set_platdata(struct s3c2410_ts_mach_info *);

/* defined by architecture to configure gpio */
extern void s3c24xx_ts_cfg_gpio(struct platform_device *dev);

enum s3c_adc_type {
    ADC_TYPE_0,
    ADC_TYPE_1,     /*    S3C2416, S3C2450 */
    ADC_TYPE_2,     /*    S3C64XX, S5PC1XX */
};

struct s3c_ts_mach_info {
    int                     delay;
    int                     presc;
    int                     oversampling_shift;
    int                     resol_bit;
    enum s3c_adc_type       s3c_adc_con;
};

struct s3c_ts_info {
    struct input_dev        *dev;
    long                    xp;
    long                    yp;
    int                     count;
    int                     shift;
    char                    phys[32];
    int                     resol_bit;
    enum s3c_adc_type       s3c_adc_con;

#ifdef CONFIG_HAS_WAKELOCK
    struct early_suspend early_suspend;
#endif

};

extern void __init s3c_ts_set_platdata(struct s3c_ts_mach_info *pd);


#endif /* __ASM_ARM_TS_H */
