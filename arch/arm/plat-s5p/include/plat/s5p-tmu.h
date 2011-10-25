/* linux/arch/arm/plat-s5p/include/plat/s5p-tmu.h
*
* Copyright 2010 Samsung Electronics Co., Ltd.
*      http://www.samsung.com/
*
* S5P - TMU driver support
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*/

#ifndef _S5PV310_THERMAL_H
#define _S5PV310_THERMAL_H

struct tmu_data {
	char	te1;			/* e-fused temperature for 25 */
	char	te2;			/* e-fused temperature for 85 */
	int	cooling;
	int	mode;			/* compensation mode */
	int	tmu_flag;
};

struct s5p_tmu {
	int 				id;
	void __iomem		*tmu_base;
	char			temperature;
	struct device		*dev;
	struct tmu_data		data;
};

extern void s5p_tmu_set_platdata(struct tmu_data *pd);
extern struct s5p_tmu *s5p_tmu_get_platdata(void);
extern int s5p_tmu_get_irqno(int num);

#endif /* _S5PV310_THERMAL_H */
