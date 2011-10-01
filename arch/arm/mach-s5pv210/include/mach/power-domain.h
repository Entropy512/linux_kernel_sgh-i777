/* linux/arch/arm/mach-s5pv210/include/mach/power-domain.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S5PV210 - Power domain support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_POWER_DOMAIN_H
#define __ASM_ARCH_POWER_DOMAIN_H __FILE__

extern struct platform_device s5pv210_pd_audio;
extern struct platform_device s5pv210_pd_cam;
extern struct platform_device s5pv210_pd_tv;
extern struct platform_device s5pv210_pd_lcd;
extern struct platform_device s5pv210_pd_g3d;
extern struct platform_device s5pv210_pd_mfc;

#endif
