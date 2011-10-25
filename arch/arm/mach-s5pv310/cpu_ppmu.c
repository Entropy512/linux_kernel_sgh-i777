/* linux/arch/arm/mach-s5pv310/cpu_ppmu.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S5PV310 - CPU PPMU support
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

void s5pv310_cpu_ppmu_reset(struct s5pv310_cpu_ppmu_hw *ppmu)
{
	void __iomem *cpu_ppmu_base = ppmu->cpu_hw_base;

	__raw_writel(0x3 << 1, cpu_ppmu_base);
	__raw_writel(0x8000000f, cpu_ppmu_base + 0x0010);
	__raw_writel(0x8000000f, cpu_ppmu_base + 0x0030);

	__raw_writel(0x0, cpu_ppmu_base + DEVT0_ID);
	__raw_writel(0x0, cpu_ppmu_base + DEVT0_IDMSK);

	__raw_writel(0x0, cpu_ppmu_base + DEVT1_ID);
	__raw_writel(0x0, cpu_ppmu_base + DEVT1_IDMSK);

	ppmu->ccnt = 0;
	ppmu->event = 0;
	ppmu->count[0] = 0;
	ppmu->count[1] = 0;
	ppmu->count[2] = 0;
	ppmu->count[3] = 0;
}

void s5pv310_cpu_ppmu_setevent(struct s5pv310_cpu_ppmu_hw *ppmu,
				  unsigned int evt, unsigned int evt_num)
{
	void __iomem *cpu_ppmu_base = ppmu->cpu_hw_base;

	ppmu->event = evt;

	__raw_writel(evt , cpu_ppmu_base + DEVT0_SEL + (evt_num * 0x100));
}

void s5pv310_cpu_ppmu_start(struct s5pv310_cpu_ppmu_hw *ppmu)
{
	void __iomem *cpu_ppmu_base = ppmu->cpu_hw_base;

	__raw_writel(0x1, cpu_ppmu_base);
}

void s5pv310_cpu_ppmu_stop(struct s5pv310_cpu_ppmu_hw *ppmu)
{
	void __iomem *cpu_ppmu_base = ppmu->cpu_hw_base;

	__raw_writel(0x0, cpu_ppmu_base);
}

void s5pv310_cpu_ppmu_update(struct s5pv310_cpu_ppmu_hw *ppmu)
{
	void __iomem *cpu_ppmu_base = ppmu->cpu_hw_base;
	unsigned int i, percent;

	ppmu->ccnt = __raw_readl(cpu_ppmu_base + 0x0100);


	for (i = 0; i < NUMBER_OF_COUNTER; i++) {
		ppmu->count[i] =
			__raw_readl(cpu_ppmu_base + (0x110 + (0x10 * i)));
	}
}
