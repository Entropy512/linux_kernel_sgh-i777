/* linux/arch/arm/mach-s5pv310/dmc.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S5PV310 - DMC support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/io.h>

#include <mach/map.h>
#include <mach/regs-clock.h>
#include <mach/dmc.h>

void s5pv310_dmc_ppmu_reset(struct s5pv310_dmc_ppmu_hw *ppmu)
{
	void __iomem *dmc_base = ppmu->dmc_hw_base;

	__raw_writel(0x8000000f, dmc_base + 0xf010);
	__raw_writel(0x8000000f, dmc_base + 0xf050);
	__raw_writel(0x6, dmc_base + 0xf000);
	__raw_writel(0x0, dmc_base + 0xf100);

	ppmu->ccnt = 0;
	ppmu->event = 0;
	ppmu->count[0] = 0;
	ppmu->count[1] = 0;
	ppmu->count[2] = 0;
	ppmu->count[3] = 0;
}

void s5pv310_dmc_ppmu_setevent(struct s5pv310_dmc_ppmu_hw *ppmu,
				  unsigned int evt)
{
	void __iomem *dmc_base = ppmu->dmc_hw_base;

	ppmu->event = evt;

	__raw_writel(((evt << 12) | 0x1), dmc_base + 0xfc);
}

void s5pv310_dmc_ppmu_start(struct s5pv310_dmc_ppmu_hw *ppmu)
{
	void __iomem *dmc_base = ppmu->dmc_hw_base;

	__raw_writel(0x1, dmc_base + 0xf000);
}

void s5pv310_dmc_ppmu_stop(struct s5pv310_dmc_ppmu_hw *ppmu)
{
	void __iomem *dmc_base = ppmu->dmc_hw_base;

	__raw_writel(0x0, dmc_base + 0xf000);
}

void s5pv310_dmc_ppmu_update(struct s5pv310_dmc_ppmu_hw *ppmu)
{
	void __iomem *dmc_base = ppmu->dmc_hw_base;
	unsigned int i;

	ppmu->ccnt = __raw_readl(dmc_base + 0xf100);

	for (i = 0; i < NUMBER_OF_COUNTER; i++) {
		ppmu->count[i] =
			__raw_readl(dmc_base + (0xf110 + (0x10 * i)));
	}
}
