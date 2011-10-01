/* linux/arch/arm/mach-s5pv310/include/mach/regs-iem.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S5PV310 - IEM(INTELLIGENT ENERGY MANAGEMENT) register discription
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_REGS_IEM_H
#define __ASM_ARCH_REGS_IEM_H __FILE__

/* Register for IEC  */
#define S5PV310_IECDPCCR		(0x00000)

/* Register for APC */
#define S5PV310_APC_CONTROL		(0x10010)
#define S5PV310_APC_PREDLYSEL		(0x10024)
#define S5PV310_APC_DBG_DLYCODE		(0x100E0)

#define APC_HPM_EN			(1 << 4)
#define IEC_EN				(1 << 0)

#endif /* __ASM_ARCH_REGS_IEM_H */
