/* linux/arch/arm/mach-s5pv210/include/mach/pd.h
 *
 * Copyright (c) 2009 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_PD_H
#define __ASM_ARCH_PD_H __FILE__

#include <mach/regs-clock.h>

#define S5PV210_PD_IROM		(1 << 20)
#define S5PV210_PD_AUDIO	(1 << 7)
#define S5PV210_PD_CAM		(1 << 5)
#define S5PV210_PD_TV		(1 << 4)
#define S5PV210_PD_LCD		(1 << 3)
#define S5PV210_PD_G3D		(1 << 2)
#define S5PV210_PD_MFC		(1 << 1)

#define PD_ACTIVE		1
#define PD_INACTIVE		0

#define PD_CLOCK_TIME		100

extern int s5pv210_pd_enable(const char *id);
extern int s5pv210_pd_disable(const char *id);

extern spinlock_t pd_lock;

struct power_domain {
	int			nr_clks;
	int			usage;
	unsigned long		ctrlbit;
};

struct pd_domain {
	struct list_head	list;
	struct power_domain	*parent_pd;
	void __iomem		*clk_reg;
	int			clk_bit;
	const char		*name;
	int			(*enable)(struct pd_domain *, int enable);
};

#endif /* __ASM_ARCH_PD_H */
