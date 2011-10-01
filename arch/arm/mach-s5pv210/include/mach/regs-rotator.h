/* linux/arch/arm/plat-s5pc11x/include/plat/regs-rotator2.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 * http://www.samsung.com/
 *
 * Register file for Samsung S5P Image Rotator
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef	_REGS_S5P_ROTATOR_H_
#define	_REGS_S5P_ROTATOR_H_

/*************************************************************************
 * Register part
 ************************************************************************/
#define	S5P_ROTATOR(x)	((x))
#define	S5P_ROT_CONFIG			S5P_ROTATOR(0x00)
#define	S5P_ROT_CTRL			S5P_ROTATOR(0x10)
#define	S5P_ROT_STATUS			S5P_ROTATOR(0x20)

#define	S5P_ROT_SRCBASEADDR0		S5P_ROTATOR(0x30)
#define	S5P_ROT_SRCBASEADDR1		S5P_ROTATOR(0x34)
#define	S5P_ROT_SRCBASEADDR2		S5P_ROTATOR(0x38)
#define	S5P_ROT_SRCIMGSIZE		S5P_ROTATOR(0x3C)
#define	S5P_ROT_SRC_XY			S5P_ROTATOR(0x40)
#define	S5P_ROT_SRCROTSIZE		S5P_ROTATOR(0x44)
#define	S5P_ROT_DSTBASEADDR0		S5P_ROTATOR(0x50)
#define	S5P_ROT_DSTBASEADDR1		S5P_ROTATOR(0x54)
#define	S5P_ROT_DSTBASEADDR2		S5P_ROTATOR(0x58)
#define	S5P_ROT_DSTIMGSIZE		S5P_ROTATOR(0x5C)
#define	S5P_ROT_DST_XY			S5P_ROTATOR(0x60)
#define	S5P_ROT_WRITE_PATTERN		S5P_ROTATOR(0x70)


/*************************************************************************
 * Macro part
 ************************************************************************/
#define	S5P_ROT_WIDTH(x)			((x) <<	0)
#define	S5P_ROT_HEIGHT(x)			((x) <<	16)
#define	S5P_ROT_LEFT(x)				((x) <<	0)
#define	S5P_ROT_TOP(x)				((x) <<	16)

#define	S5P_ROT_DEGREE(x)			((x) <<	4)
#define	S5P_ROT_FLIP(x)				((x) <<	6)

#define	S5P_ROT_SRC_FMT(x)			((x) <<	8)

/*************************************************************************
 * Bit definition part
 ************************************************************************/
#define	S5P_ROT_CONFIG_ENABLE_INT		(1 << 8)
#define	S5P_ROT_CONFIG_STATUS_MASK		(3 << 0)

#define	S5P_ROT_CTRL_PATTERN_WRITING		(1 << 16)

#define	S5P_ROT_CTRL_INPUT_FMT_YCBCR420_3P	(0 << 8)
#define	S5P_ROT_CTRL_INPUT_FMT_YCBCR420_2P	(1 << 8)
#define	S5P_ROT_CTRL_INPUT_FMT_YCBCR422_1P	(3 << 8)
#define	S5P_ROT_CTRL_INPUT_FMT_RGB565		(4 << 8)
#define	S5P_ROT_CTRL_INPUT_FMT_RGB888		(6 << 8)
#define	S5P_ROT_CTRL_INPUT_FMT_MASK		(7 << 8)

#define	S5P_ROT_CTRL_DEGREE_MASK		(0xF <<	4)

#define	S5P_ROT_CTRL_START_ROTATE		(1 << 0)
#define	S5P_ROT_CTRL_START_MASK			(1 << 0)

#define	S5P_ROT_STATREG_INT_PENDING		(1 << 8)
#define	S5P_ROT_STATREG_STATUS_IDLE		(0 << 0)
#define	S5P_ROT_STATREG_STATUS_BUSY		(2 << 0)
#define	S5P_ROT_STATREG_STATUS_HAS_ONE_MORE_JOB	(3 << 0)

#endif /* _REGS_S5P_ROTATOR_H_ */
