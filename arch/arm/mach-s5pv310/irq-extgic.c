/* linux/arch/arm/mach-s5pv310/irq-extgic.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Based on arch/arm/common/gic.c
 *
 * External GIC support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/smp.h>
#include <linux/cpumask.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/irqnr.h>
#include <linux/interrupt.h>


#include <asm/irq.h>
#include <asm/mach/irq.h>
#include <asm/hardware/gic.h>

#include <plat/pm.h>

#include <mach/ext-gic.h>
#include <mach/map.h>
#include <mach/irqs.h>

void __init ext_gic_dist_init(void __iomem *base)
{
	unsigned int i, max_irq;

	__raw_writel(0, base + GIC_DIST_CTRL);

	max_irq = __raw_readl(base + GIC_DIST_CTR) & 0x1f;
	max_irq = (max_irq + 1) * 32;

	for (i = 32; i < max_irq; i += 16)
		__raw_writel(0, base + GIC_DIST_CONFIG + i * 4 / 16);

	for (i = 32; i < max_irq; i += 4)
		__raw_writel(0x01010101, base + GIC_DIST_TARGET + i * 4 / 4);

	for (i = 0; i < max_irq; i += 4)
		__raw_writel(0xa0a0a0a0, base + GIC_DIST_PRI + i * 4 / 4);

	for (i = 0; i < max_irq; i += 32)
		__raw_writel(0xffffffff,
				base + GIC_DIST_ENABLE_CLEAR + i * 4 / 32);


}

void ext_gic_cpu_init(void __iomem *base)
{
	writel(0xf0, base + GIC_CPU_PRIMASK);
	writel(0, base + GIC_CPU_CTRL);
}

void ext_gic_init(void)
{
	ext_gic_dist_init(S5P_VA_EXTGIC_DIST);
	ext_gic_cpu_init(S5P_VA_EXTGIC_CPU);
}

#define S5P_IGIC(x) (S5P_VA_GIC_DIST + x)
#define S5P_ICOMB(x) (S5P_VA_COMBINER_BASE + x)
#define S5P_EGIC(x) (S5P_VA_EXTGIC_DIST + x)
#define S5P_ECOMB(x) (S5P_VA_EXTCOMBINER_BASE + x)

static struct cached_irq_status ecomb_reg[] = {
	{S5P_ECOMB(0x00), S5P_ECOMB(0x04), 0x0},
	{S5P_ECOMB(0x10), S5P_ECOMB(0x14), 0x0},
	{S5P_ECOMB(0x20), S5P_ECOMB(0x24), 0x0},
	{S5P_ECOMB(0x30), S5P_ECOMB(0x34), 0x0},
};

static struct cached_irq_status egic_reg[] = {
	{S5P_EGIC(0x104), S5P_EGIC(0x184), 0x0},
	{S5P_EGIC(0x108), S5P_EGIC(0x188), 0x0},
	{S5P_EGIC(0x10C), S5P_EGIC(0x18C), 0x0},
	{S5P_EGIC(0x110), S5P_EGIC(0x190), 0x0},
};

static struct cached_irq_status icomb_reg[] = {
	{S5P_ICOMB(0x00), S5P_ICOMB(0x04), 0x0},
	{S5P_ICOMB(0x10), S5P_ICOMB(0x14), 0x0},
	{S5P_ICOMB(0x20), S5P_ICOMB(0x24), 0x0},
	{S5P_ICOMB(0x30), S5P_ICOMB(0x34), 0x0},
	{S5P_ICOMB(0x40), S5P_ICOMB(0x44), 0x0},
	{S5P_ICOMB(0x50), S5P_ICOMB(0x54), 0x0},
	{S5P_ICOMB(0x60), S5P_ICOMB(0x64), 0x0},
	{S5P_ICOMB(0x70), S5P_ICOMB(0x74), 0x0},
	{S5P_ICOMB(0x80), S5P_ICOMB(0x84), 0x0},
	{S5P_ICOMB(0x90), S5P_ICOMB(0x94), 0x0},
	{0x0, 0x0, 0x0},
	{0x0, 0x0, 0x0},
	{S5P_ICOMB(0xC0), S5P_ICOMB(0xC4), 0x0},
	{S5P_ICOMB(0xD0), S5P_ICOMB(0xD4), 0x0},
	{0x0, 0x0, 0x0},
};

static struct cached_irq_status igic_reg[] = {
	{S5P_IGIC(0x104), S5P_IGIC(0x184), 0x0},
	{S5P_IGIC(0x108), S5P_IGIC(0x188), 0x0},
};

static struct int_to_ext_map igic1[32] = {
	[8] = {
		.reg_data = {S5P_EGIC(0x104), S5P_EGIC(0x184), 0x0},
		.shift = 16,
	},
	[9] = {
		.reg_data = {S5P_EGIC(0x104), S5P_EGIC(0x184), 0x0},
		.shift = 17,
	},
	[10] = {
		.reg_data = {S5P_EGIC(0x104), S5P_EGIC(0x184), 0x0},
		.shift = 18,
	},
	[11] = {
		.reg_data = {S5P_EGIC(0x104), S5P_EGIC(0x184), 0x0},
		.shift = 19,
	},
	[12] = {
		.reg_data = {S5P_EGIC(0x10C), S5P_EGIC(0x18C), 0x0},
		.shift = 7,
	},
	[13] = {
		.reg_data = {S5P_EGIC(0x10C), S5P_EGIC(0x18C), 0x0},
		.shift = 6,
	},
	[14] = {
		.reg_data = {S5P_EGIC(0x10C), S5P_EGIC(0x18C), 0x0},
		.shift = 8,
	},
	[15] = {
		.reg_data = {S5P_EGIC(0x10C), S5P_EGIC(0x18C), 0x0},
		.shift = 19,
	},
	[16] = {
		.reg_data = {S5P_EGIC(0x10C), S5P_EGIC(0x18C), 0x0},
		.shift = 24,
	},
	[17] = {
		.reg_data = {S5P_EGIC(0x10C), S5P_EGIC(0x18C), 0x0},
		.shift = 25,
	},
	[18] = {
		.reg_data = {S5P_EGIC(0x10C), S5P_EGIC(0x18C), 0x0},
		.shift = 26,
	},
	[20] = {
		.reg_data = {S5P_EGIC(0x10C), S5P_EGIC(0x18C), 0x0},
		.shift = 30,
	},
	[22] = {
		.reg_data = {S5P_EGIC(0x110), S5P_EGIC(0x190), 0x0},
		.shift = 0,
	},
	[23] = {
		.reg_data = {S5P_EGIC(0x110), S5P_EGIC(0x190), 0x0},
		.shift = 4,
	},
	[24] = {
		.reg_data = {S5P_EGIC(0x110), S5P_EGIC(0x190), 0x0},
		.shift = 8,
	},
	[25] = {
		.reg_data = {S5P_EGIC(0x110), S5P_EGIC(0x190), 0x0},
		.shift = 13,
	},
	[26] = {
		.reg_data = {S5P_EGIC(0x110), S5P_EGIC(0x190), 0x0},
		.shift = 16,
	},
	[27] = {
		.reg_data = {S5P_EGIC(0x110), S5P_EGIC(0x190), 0x0},
		.shift = 17,
	},
	[28] = {
		.reg_data = {S5P_EGIC(0x110), S5P_EGIC(0x190), 0x0},
		.shift = 14,
	},
	[29] = {
		.reg_data = {S5P_EGIC(0x110), S5P_EGIC(0x190), 0x0},
		.shift = 19,
	},
	[30] = {
		.reg_data = {S5P_EGIC(0x110), S5P_EGIC(0x190), 0x0},
		.shift = 20,
	},
	[31] = {
		.reg_data = {S5P_EGIC(0x110), S5P_EGIC(0x190), 0x0},
		.shift = 15,
	},
};

static struct int_to_ext_map icomb3[32] = {
	[8] = {
		.reg_data = {S5P_EGIC(0x110), S5P_EGIC(0x190), 0x0,},
		.shift = 22,
	},
	[9] = {
		.reg_data = {S5P_EGIC(0x110), S5P_EGIC(0x190), 0x0,},
		.shift = 23,
	},
	[10] = {
		.reg_data = {S5P_EGIC(0x110), S5P_EGIC(0x190), 0x0,},
		.shift = 24,
	},
	[11] = {
		.reg_data = {S5P_EGIC(0x110), S5P_EGIC(0x190), 0x0,},
		.shift = 25,
	},
	[12] = {
		.reg_data = {S5P_EGIC(0x110), S5P_EGIC(0x190), 0x0,},
		.shift = 26,
	},
	[16] = {
		.reg_data = {S5P_EGIC(0x110), S5P_EGIC(0x190), 0x0,},
		.shift = 27,
	},
	[17] = {
		.reg_data = {S5P_EGIC(0x110), S5P_EGIC(0x190), 0x0,},
		.shift = 28,
	},
	[18] = {
		.reg_data = {S5P_EGIC(0x110), S5P_EGIC(0x190), 0x0,},
		.shift = 29,
	},
	[19] = {
		.reg_data = {S5P_EGIC(0x110), S5P_EGIC(0x190), 0x0,},
		.shift = 30,
	},
	[20] = {
		.reg_data = {S5P_EGIC(0x110), S5P_EGIC(0x190), 0x0,},
		.shift = 31,
	},
	[21] = {
		.reg_data = {S5P_EGIC(0x110), S5P_EGIC(0x190), 0x0,},
		.shift = 21,
	},
};

static struct int_to_ext_map icomb4[32] = {
	[0] = {
		.reg_data = {S5P_EGIC(0x10C), S5P_EGIC(0x18C), 0x0,},
		.shift = 28,
	},
	[1] = {
		.reg_data = {S5P_EGIC(0x10C), S5P_EGIC(0x18C), 0x0,},
		.shift = 29,
	},
	[2] = {
		.reg_data = {S5P_EGIC(0x110), S5P_EGIC(0x190), 0x0,},
		.shift = 18,
	},
	[8] = {
		.reg_data = {S5P_EGIC(0x110), S5P_EGIC(0x190), 0x0,},
		.shift = 1,
	},
	[9] = {
		.reg_data = {S5P_EGIC(0x110), S5P_EGIC(0x190), 0x0,},
		.shift = 2,
	},
	[10] = {
		.reg_data = {S5P_EGIC(0x110), S5P_EGIC(0x190), 0x0,},
		.shift = 3,
	},
	[16] = {
		.reg_data = {S5P_EGIC(0x110), S5P_EGIC(0x190), 0x0,},
		.shift = 5,
	},
	[17] = {
		.reg_data = {S5P_EGIC(0x110), S5P_EGIC(0x190), 0x0,},
		.shift = 6,
	},
	[18] = {
		.reg_data = {S5P_EGIC(0x110), S5P_EGIC(0x190), 0x0,},
		.shift = 7,
	},
	[24] = {
		.reg_data = {S5P_EGIC(0x110), S5P_EGIC(0x190), 0x0,},
		.shift = 9,
	},
	[25] = {
		.reg_data = {S5P_EGIC(0x110), S5P_EGIC(0x190), 0x0,},
		.shift = 10,
	},
	[26] = {
		.reg_data = {S5P_EGIC(0x110), S5P_EGIC(0x190), 0x0,},
		.shift = 11,
	},
	[27] = {
		.reg_data = {S5P_EGIC(0x110), S5P_EGIC(0x190), 0x0,},
		.shift = 12,
	},
};

static struct int_to_ext_map icomb5[32] = {
	[1] = {
		.reg_data = {S5P_EGIC(0x108), S5P_EGIC(0x188), 0x0,},
		.shift = 2,
	},
	[8] = {
		.reg_data = {S5P_EGIC(0x108), S5P_EGIC(0x188), 0x0,},
		.shift = 3,
	},
	[9] = {
		.reg_data = {S5P_EGIC(0x108), S5P_EGIC(0x188), 0x0,},
		.shift = 4,
	},
	[16] = {
		.reg_data = {S5P_EGIC(0x108), S5P_EGIC(0x188), 0x0,},
		.shift = 5,
	},
	[17] = {
		.reg_data = {S5P_EGIC(0x108), S5P_EGIC(0x188), 0x0,},
		.shift = 6,
	},
	[18] = {
		.reg_data = {S5P_EGIC(0x108), S5P_EGIC(0x188), 0x0,},
		.shift = 7,
	},
	[19] = {
		.reg_data = {S5P_EGIC(0x108), S5P_EGIC(0x188), 0x0,},
		.shift = 8,
	},
	[20] = {
		.reg_data = {S5P_EGIC(0x108), S5P_EGIC(0x188), 0x0,},
		.shift = 9,
	},
	[24] = {
		.reg_data = {S5P_EGIC(0x108), S5P_EGIC(0x188), 0x0,},
		.shift = 12,
	},
	[25] = {
		.reg_data = {S5P_EGIC(0x108), S5P_EGIC(0x188), 0x0,},
		.shift = 13,
	},
};

static struct int_to_ext_map icomb6[32] = {
	[0] = {
		.reg_data = {S5P_EGIC(0x108), S5P_EGIC(0x188), 0x0,},
		.shift = 14,
	},
	[1] = {
		.reg_data = {S5P_EGIC(0x108), S5P_EGIC(0x188), 0x0,},
		.shift = 15,
	},
	[8] = {
		.reg_data = {S5P_EGIC(0x108), S5P_EGIC(0x188), 0x0,},
		.shift = 17,
	},
	[9] = {
		.reg_data = {S5P_EGIC(0x108), S5P_EGIC(0x188), 0x0,},
		.shift = 18,
	},
	[16] = {
		.reg_data = {S5P_EGIC(0x108), S5P_EGIC(0x188), 0x0,},
		.shift = 20,
	},
	[17] = {
		.reg_data = {S5P_EGIC(0x108), S5P_EGIC(0x188), 0x0,},
		.shift = 21,
	},
	[18] = {
		.reg_data = {S5P_EGIC(0x108), S5P_EGIC(0x188), 0x0,},
		.shift = 22,
	},
	[19] = {
		.reg_data = {S5P_EGIC(0x108), S5P_EGIC(0x188), 0x0,},
		.shift = 23,
	},
	[20] = {
		.reg_data = {S5P_EGIC(0x108), S5P_EGIC(0x188), 0x0,},
		.shift = 24,
	},
	[24] = {
		.reg_data = {S5P_EGIC(0x108), S5P_EGIC(0x188), 0x0,},
		.shift = 26,
	},
	[25] = {
		.reg_data = {S5P_EGIC(0x108), S5P_EGIC(0x188), 0x0,},
		.shift = 27,
	},
	[26] = {
		.reg_data = {S5P_EGIC(0x108), S5P_EGIC(0x188), 0x0,},
		.shift = 28,
	},
	[27] = {
		.reg_data = {S5P_EGIC(0x108), S5P_EGIC(0x188), 0x0,},
		.shift = 29,
	},
	[28] = {
		.reg_data = {S5P_EGIC(0x108), S5P_EGIC(0x188), 0x0,},
		.shift = 30,
	},
	[29] = {
		.reg_data = {S5P_EGIC(0x108), S5P_EGIC(0x188), 0x0,},
		.shift = 31,
	},
	[30] = {
		.reg_data = {S5P_EGIC(0x10C), S5P_EGIC(0x18C), 0x0,},
		.shift = 0,
	},
	[31] = {
		.reg_data = {S5P_EGIC(0x10C), S5P_EGIC(0x18C), 0x0,},
		.shift = 1,
	},
};

static struct int_to_ext_map icomb7[32] = {
	[0] = {
		.reg_data = {S5P_EGIC(0x10C), S5P_EGIC(0x18C), 0x0,},
		.shift = 2,
	},
	[1] = {
		.reg_data = {S5P_EGIC(0x10C), S5P_EGIC(0x18C), 0x0,},
		.shift = 3,
	},
	[2] = {
		.reg_data = {S5P_EGIC(0x10C), S5P_EGIC(0x18C), 0x0,},
		.shift = 4,
	},
	[8] = {
		.reg_data = {S5P_EGIC(0x10C), S5P_EGIC(0x18C), 0x0,},
		.shift = 9,
	},
	[9] = {
		.reg_data = {S5P_EGIC(0x10C), S5P_EGIC(0x18C), 0x0,},
		.shift = 10,
	},
	[10] = {
		.reg_data = {S5P_EGIC(0x10C), S5P_EGIC(0x18C), 0x0,},
		.shift = 11,
	},
	[11] = {
		.reg_data = {S5P_EGIC(0x10C), S5P_EGIC(0x18C), 0x0,},
		.shift = 12,
	},
	[12] = {
		.reg_data = {S5P_EGIC(0x10C), S5P_EGIC(0x18C), 0x0,},
		.shift = 13,
	},
	[16] = {
		.reg_data = {S5P_EGIC(0x10C), S5P_EGIC(0x18C), 0x0,},
		.shift = 14,
	},
	[17] = {
		.reg_data = {S5P_EGIC(0x10C), S5P_EGIC(0x18C), 0x0,},
		.shift = 16,
	},
	[24] = {
		.reg_data = {S5P_EGIC(0x10C), S5P_EGIC(0x18C), 0x0,},
		.shift = 15,
	},
	[25] = {
		.reg_data = {S5P_EGIC(0x10C), S5P_EGIC(0x18C), 0x0,},
		.shift = 17,
	},

};

static struct int_to_ext_map icomb8[32] = {
	[0] = {
		.reg_data = {S5P_EGIC(0x10C), S5P_EGIC(0x18C), 0x0,},
		.shift = 20,
	},
	[1] = {
		.reg_data = {S5P_EGIC(0x10C), S5P_EGIC(0x18C), 0x0,},
		.shift = 21,
	},
	[8] = {
		.reg_data = {S5P_EGIC(0x10C), S5P_EGIC(0x18C), 0x0,},
		.shift = 22,
	},
	[9] = {
		.reg_data = {S5P_EGIC(0x10C), S5P_EGIC(0x18C), 0x0,},
		.shift = 23,
	},
	[16] = {
		.reg_data = {S5P_EGIC(0x10C), S5P_EGIC(0x18C), 0x0,},
		.shift = 18,
	},
	[17] = {
		.reg_data = {S5P_EGIC(0x108), S5P_EGIC(0x188), 0x0,},
		.shift = 19,
	},
	[27] = {
		.reg_data = {S5P_EGIC(0x108), S5P_EGIC(0x188), 0x0,},
		.shift = 16,
	},

};

static struct int_to_ext_map icomb9[32] = {
	[0] = {
		.reg_data = {S5P_EGIC(0x10C), S5P_EGIC(0x18C), 0x0,},
		.shift = 27,
	},
	[1] = {
		.reg_data = {S5P_EGIC(0x10C), S5P_EGIC(0x18C), 0x0,},
		.shift = 31,
	},
};

static struct int_to_ext_map icomb12[32] = {
	[24] = {
		.reg_data = {S5P_EGIC(0x108), S5P_EGIC(0x188), 0x0,},
		.shift = 10,
	},
};

static struct int_to_ext_map *icomb_map[] = {
	NULL, NULL, NULL, icomb3, icomb4, icomb5, icomb6,
	icomb7, icomb8, icomb9, NULL, NULL, icomb12, NULL,
};

static struct int_to_ext_map *igic_map[] = {
	NULL, igic1,
};

static void find_and_set_reg(struct int_to_ext_map *icomb, unsigned int pre,
			     unsigned int curr)
{
	struct int_to_ext_map *comb = icomb;
	void __iomem *en_reg, *dis_reg;
	unsigned int shft;
	int tmp, val, i;

	if (pre == curr)
		return;

	if (comb == NULL)
		return;

	tmp = (pre ^ curr);

	for (i = 0; i < 32; i++) {
		/*
		 * Comparing previous interrupt enabling status with current
		 * interrupt status
		 */
		if ((tmp >> i) & 0x1) {
			en_reg = comb[i].reg_data.set_enable_reg;
			if (en_reg == NULL)
				continue;
			dis_reg = comb[i].reg_data.set_disable_reg;
			shft = comb[i].shift;
			if ((curr >> i) & 0x1) {
				val = __raw_readl(en_reg);
				val |= (0x1 << shft);
				__raw_writel(val, en_reg);
			} else {
				val = __raw_readl(dis_reg);
				val |= (0x1 << shft);
				__raw_writel(val, dis_reg);
			}
		}

	}
}

/*
 * Remap external GIC and combiner with internal GIC and combiner's
 * configuration. AFTR mode requires wakeup by external GIC. This work is
 * needed.
 */
void remap_ext_gic(void)
{
	unsigned int tmp, prev, i;

	/*
	 * int combiner to ext combiner/GIC remapping
	 */
	for (i = 0; i < (sizeof(icomb_reg)/
				sizeof(struct cached_irq_status)); i++) {

		if (icomb_reg[i].set_enable_reg == NULL)
			continue;
		prev = icomb_reg[i].val;

		tmp = __raw_readl(icomb_reg[i].set_enable_reg);
		icomb_reg[i].val = tmp;

		if (tmp != prev) {

			find_and_set_reg(icomb_map[i], prev, tmp);

			if (i < 4) {
				if (i == 3)
					tmp &= ~0xffff00;
				__raw_writel(tmp, ecomb_reg[i].set_enable_reg);

				if (i == 3) {
					tmp = icomb_reg[i].val;
					tmp |= 0xffff00;
				}
				__raw_writel(~tmp,
						ecomb_reg[i].set_disable_reg);
			}
		}
	}

	/*
	 * int GIC to ext GIC remapping
	 */
	for (i = 0; i < (sizeof(igic_reg)/
				sizeof(struct cached_irq_status)); i++) {

		prev = igic_reg[i].val;

		tmp = __raw_readl(igic_reg[i].set_enable_reg);
		if (tmp != prev)
			find_and_set_reg(igic_map[i], prev, tmp);
		igic_reg[i].val = tmp;
	}

}

static struct sleep_save s5pv310_egic_save[] = {
	SAVE_ITEM(S5P_EGIC(0x104)),
	SAVE_ITEM(S5P_EGIC(0x108)),
	SAVE_ITEM(S5P_EGIC(0x10C)),
	SAVE_ITEM(S5P_EGIC(0x110)),
	SAVE_ITEM(S5P_ECOMB(0x00)),
	SAVE_ITEM(S5P_ECOMB(0x10)),
	SAVE_ITEM(S5P_ECOMB(0x20)),
	SAVE_ITEM(S5P_ECOMB(0x30)),
};

static int s5pv310_egic_probe(struct platform_device *pdev)
{
	return 0;
}

static int __devexit s5pv310_egic_remove(struct platform_device *pdev)
{
	return 0;
}

#ifdef CONFIG_PM
static int s5pv310_egic_suspend(struct platform_device *pdev,
				pm_message_t state)
{
	s3c_pm_do_save(s5pv310_egic_save, ARRAY_SIZE(s5pv310_egic_save));
	return 0;
}

static int s5pv310_egic_resume(struct platform_device *pdev)
{
	ext_gic_init();
	s3c_pm_do_restore_core(s5pv310_egic_save,
				ARRAY_SIZE(s5pv310_egic_save));

	return 0;
}

#else
#define s5pv310_egic_suspend	NULL
#define s5pv310_egic_resume	NULL
#endif

static struct platform_driver s5pv310_egic_driver = {
	.driver		= {
		.name	= "s5pv310-external-GIC",
		.owner	= THIS_MODULE,
	},
	.probe		= s5pv310_egic_probe,
	.remove		= __devexit_p(s5pv310_egic_remove),
	.suspend	= s5pv310_egic_suspend,
	.resume		= s5pv310_egic_resume,
};

static int __init s5pv310_egic_init(void)
{
	int ret;

	ret = platform_driver_register(&s5pv310_egic_driver);
	if (ret)
		printk(KERN_ERR "%s: failed to register driver\n", __func__);

	return ret;
}

device_initcall(s5pv310_egic_init);
