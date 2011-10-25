/*
 *  arch/arm/include/asm/hardware/gic.h
 *
 *  Copyright (C) 2002 ARM Limited, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __EXT_GIC_H
#define __EXT_GIC_H

struct cached_irq_status {
	void __iomem *set_enable_reg;
	void __iomem *set_disable_reg;
	unsigned int val;
};

struct int_to_ext_map {
	struct cached_irq_status reg_data;
	unsigned int shift;
};

void ext_gic_init(void); 

void remap_ext_gic(void);
#endif
