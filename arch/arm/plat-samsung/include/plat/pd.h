/* linux/arch/arm/plat-samsung/include/plat/pd.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_PLAT_SAMSUNG_PD_H
#define __ASM_PLAT_SAMSUNG_PD_H __FILE__

struct samsung_pd_info {
	int (*init)(struct device *dev);
	int (*enable)(struct device *dev);
	int (*disable)(struct device *dev);
	void __iomem *base;
	void *data;
};

struct s5pv310_pd_data {
	void __iomem *clk_base;
	void __iomem *read_base;
	unsigned long read_phy_addr;
};

enum s5pv310_pd_block {
	PD_MFC,
	PD_G3D,
	PD_LCD0,
	PD_LCD1,
	PD_TV,
	PD_CAM,
	PD_GPS
};

int s5pv310_pd_enable(struct device *dev);
#endif /* __ASM_PLAT_SAMSUNG_PD_H */
