/* linux/arch/arm/mach-s5pv310/include/mach/regs-thermal.h

* Copyright (c) 2010 Samsung Electronics Co., Ltd.
*      http://www.samsung.com/
*
* S5PV310 - Clock register definitions
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_REGS_THERMAL_H
#define __ASM_ARCH_REGS_THERMAL_H __FILE__

#define TRIMINFO			 (0x0)

#define TMU_CON0			 (0x20)
#define TMU_STATUS			 (0x28)
#define SAMPLING_INTERNAL	 (0x2C)
#define CNT_VALUE0			 (0x30)
#define CNT_VALUE1			 (0x34)

#define CURRENT_TEMP		 (0x40)
#define THRESHOLD_TEMP		 (0x44)

#define TRG_LEV0			 (0x50)
#define TRG_LEV1			 (0x54)
#define TRG_LEV2			 (0x58)
#define TRG_LEV3			 (0x5C)

#define PAST_TMEP0			 (0x60)
#define PAST_TMEP1			 (0x64)
#define PAST_TMEP2			 (0x68)
#define PAST_TMEP3			 (0x6C)

#define INTEN				 (0x70)
#define INTSTAT				 (0x74)
#define INTCLEAR			 (0x78)

#define INTEN0              (1)
#define INTEN1              (1<<4)
#define INTEN2              (1<<8)
#define INTEN3              (1<<12)

#define INTSTAT0			(1)
#define INTSTAT1			(1<<4)
#define INTSTAT2			(1<<8)
#define INTSTAT3			(1<<12)

#define TRIM_TEMP_MASK		(0xFF)

#define INTCLEAR0			(1)
#define INTCLEAR1			(1<<4)
#define INTCLEAR2			(1<<8)
#define	INTCLEAR3			(1<<12)
#define INTCLEARALL			(INTCLEAR0 | INTCLEAR1 |\
							 INTCLEAR2 | INTCLEAR2)

#endif
