/* sound/soc/s3c24xx/s5p-rp_reg.h
 *
 * Audio RP Registers for Samsung s5pc210
 *
 * Copyright (c) 2010 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _RP_REG_H_
#define _RP_REG_H_


#define RP_IRAM_BASE			(0x02020000)
#define RP_SRAM_BASE			(0x03000000)
/*#define RP_BUF_BASE			(0xEEA00000)*/
#define RP_ASSCLK_BASE			(0x03810000)
#define RP_COMMBOX_BASE			(0x03820000)

/*
 * SRAM & Commbox base address
 */

#define RP_DMEM_ADDR			(RP_SRAM_BASE + 0x00000000)
#define RP_ICACHE_ADDR			(RP_SRAM_BASE + 0x00020000)
#define RP_CMEM_ADDR			(RP_SRAM_BASE + 0x00030000)

/*
 * Internal Memory Offset
 */
#define RP_DMEM				(0x00000)
#define RP_ICACHE			(0x20000)
#define RP_CMEM				(0x30000)

/*
 * Commbox Offset
 */

#define RP_CONT				(0x0000)
#define RP_CFGR				(0x0004)
#define RP_INTERRUPT			(0x0008)
#define RP_PENDING			(0x000C)
#define RP_INTERRUPT_CODE		(0x0010)
#define RP_INFORMATION			(0x0014)
#define RP_ERROR_CODE			(0x0018)
#define RP_ARM_INTERRUPT_CODE		(0x001C)
#define RP_EFFECT_DEF			(0x0020)
#define RP_EQ_USER_DEF			(0x0024)
#define RP_FRAME_INDEX			(0x0028)
#define RP_PCM_BUFF_SIZE		(0x002C)
#define RP_PCM_BUFF0			(0x0030)
#define RP_PCM_BUFF1			(0x0034)
#define RP_READ_BITSTREAM_SIZE		(0x0038)

#define RP_LOAD_CGA_SA_ADDR		(0x0100)
#define RP_BITSTREAM_BUFF_DRAM_ADDR1	(0x0104)
#define RP_BITSTREAM_SIZE		(0x0108)
#define RP_BITSTREAM_BUFF_DRAM_ADDR0	(0x010C)
#define RP_CODE_START_ADDR		(0x0110)
#define RP_PCM_DUMP_ADDR		(0x0114)
#define RP_DATA_START_ADDR		(0x0118)
#define RP_BITSTREAM_BUFF_DRAM_SIZE	(0x011C)
#define RP_CONF_START_ADDR		(0x0120)
#define RP_GAIN_CTRL_FACTOR_L		(0x0124)
#define RP_UART_INFORMATION		(0x0128)
#define RP_GAIN_CTRL_FACTOR_R		(0x012C)

/*
 * Interrupt Code & Information
 */

/* for RP_INTERRUPT_CODE */
#define RP_INTR_CODE_MASK		(0x07FF)

#define RP_INTR_CODE_PLAYDONE		(0x0001 << 0)
#define RP_INTR_CODE_ERROR		(0x0001 << 1)
#define RP_INTR_CODE_REQUEST		(0x0001 << 2)
#define RP_INTR_CODE_POLLINGWAIT	(0x0001 << 9)
#define RP_INTR_CODE_UART_OUTPUT	(0x0001 << 10)

#define RP_INTR_CODE_REQUEST_MASK	(0x0007 << 6)
#define RP_INTR_CODE_NOTIFY_INFO	(0x0007 << 6)
#define RP_INTR_CODE_IBUF_REQUEST_ULP	(0x0006 << 6)
#define RP_INTR_CODE_IBUF_REQUEST	(0x0005 << 6)
#define RP_INTR_CODE_ULP		(0x0004 << 6)
#define RP_INTR_CODE_PENDING_ULP	(0x0003 << 6)
#define RP_INTR_CODE_PLAYEND		(0x0002 << 6)
#define RP_INTR_CODE_DRAM_REQUEST	(0x0001 << 6)

#define RP_INTR_CODE_IBUF_MASK		(0x0003 << 4)
#define RP_INTR_CODE_IBUF0_EMPTY	(0x0001 << 4)
#define RP_INTR_CODE_IBUF1_EMPTY	(0x0002 << 4)

/* for RP_INFORMATION */
#define RP_INTR_INFO_MASK		(0xFFFF)

/* for RP_ARM_INTERRUPT_CODE */
#define RP_ARM_INTR_CODE_MASK		(0x007F)
/* ARM to RP */
#define RP_ARM_INTR_CODE_SA_ON		(0x0001 << 0)
#define RP_ARM_INTR_CODE_PAUSE_REQ	(0x0001 << 1)
#define RP_ARM_INTR_CODE_PAUSE_STA	(0x0001 << 2)
#define RP_ARM_INTR_CODE_ULP_ATYPE	(0x0000 << 3)
#define RP_ARM_INTR_CODE_ULP_BTYPE	(0x0001 << 3)
#define RP_ARM_INTR_CODE_ULP_CTYPE	(0x0002 << 3)
/* RP to ARM */
#define RP_ARM_INTR_CODE_CHINF_MASK	(0x0003)
#define RP_ARM_INTR_CODE_CHINF_SHIFT	(5)
/* ARM to RP */
#define RP_ARM_INTR_CODE_PCM_DUMP_ON	(0x0001 << 7)
/* RP to ARM */
#define RP_ARM_INTR_CODE_FRAME_MASK	(0x0007 << 8)
#define RP_ARM_INTR_CODE_FRAME_NULL	(0x0000 << 8)
#define RP_ARM_INTR_CODE_FRAME_576	(0x0001 << 8)
#define RP_ARM_INTR_CODE_FRAME_1152	(0x0002 << 8)
#define RP_ARM_INTR_CODE_FRAME_384	(0x0003 << 8)
#define RP_ARM_INTR_CODE_FRAME_1024	(0x0004 << 8)

#endif
