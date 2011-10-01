/* arch/arm/plat-s5p/include/plat/regs-mfc.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Register definition file for MFC (V4.0 & V5.0) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _REGS_FIMV_H
#define _REGS_FIMV_H

#define S3C_FIMV_REG_SIZE \
				(S3C_FIMV_END_ADDR - S3C_FIMV_START_ADDR)
#define S3C_FIMV_REG_COUNT \
				((S3C_FIMV_END_ADDR - S3C_FIMV_START_ADDR) / 4)

#define S3C_FIMVREG(x)			(x)

#if defined(CONFIG_CPU_S5PC100)
#define S3C_FIMV_START_ADDR		S3C_FIMVREG(0x000)
#define S3C_FIMV_END_ADDR		S3C_FIMVREG(0xd0c)

#define S3C_FIMV_DMA_START		S3C_FIMVREG(0x000)
#define S3C_FIMV_RESERVE1		/* 0x004 */
#define S3C_FIMV_DMA_INTERNAL_ADDR	S3C_FIMVREG(0x008)
#define S3C_FIMV_BOOTCODE_SIZE		S3C_FIMVREG(0x00c)
#define S3C_FIMV_RESERVE2		/* 0x010 */
#define S3C_FIMV_DMA_EXTADDR		S3C_FIMVREG(0x014)
#define S3C_FIMV_EXT_BUF_START_ADDR	S3C_FIMVREG(0x018)
#define S3C_FIMV_EXT_BUF_END_ADDR	S3C_FIMVREG(0x01c)
#define S3C_FIMV_DMA_INTADDR		S3C_FIMVREG(0x020)
#define S3C_FIMV_HOST_PTR		S3C_FIMVREG(0x024)
#define S3C_FIMV_LAST_DEC		S3C_FIMVREG(0x028)
#define S3C_FIMV_DONE_M			S3C_FIMVREG(0x02c)
#define S3C_FIMV_RESERVE3		/* 0x030 */
#define	S3C_FIMV_CODEC_PTR		S3C_FIMVREG(0x034)
#define S3C_FIMV_ENC_ENDADDR		S3C_FIMVREG(0x038)
#define S3C_FIMV_RESERVE4		/* 0x03c - 0x40 */
#define S3C_FIMV_BITS_ENDIAN		S3C_FIMVREG(0x044)
#define S3C_FIMV_RESERVE5		/* [(0x054 -0x044)/4 - 1 ] */

#define S3C_FIMV_DEC_UNIT_SIZE		S3C_FIMVREG(0x054)
#define S3C_FIMV_ENC_UNIT_SIZE		S3C_FIMVREG(0x058)
#define S3C_FIMV_START_BYTE_NUM		S3C_FIMVREG(0x05c)
#define S3C_FIMV_ENC_HEADER_SIZE	S3C_FIMVREG(0x060)
#define S3C_FIMV_RESERVE6		/* [(0x100 -0x060)/4 -1 ] */

/* Commn */
#define S3C_FIMV_STANDARD_SEL		S3C_FIMVREG(0x100)
#define S3C_FIMV_CH_ID			S3C_FIMVREG(0x104)
#define S3C_FIMV_CPU_RESET		S3C_FIMVREG(0x108)
#define S3C_FIMV_FW_END			S3C_FIMVREG(0x10c)
#define S3C_FIMV_BUS_MASTER		S3C_FIMVREG(0x110)
#define S3C_FIMV_FRAME_START		S3C_FIMVREG(0x114)
#define S3C_FIMV_IMG_SIZE_X		S3C_FIMVREG(0x118)
#define S3C_FIMV_IMG_SIZE_Y		S3C_FIMVREG(0x11c)
#define S3C_FIMV_RESERVE7		/* 0x120 */
#define S3C_FIMV_POST_ON		S3C_FIMVREG(0x124)
#define S3C_FIMV_FRAME_RATE		S3C_FIMVREG(0x128)
#define S3C_FIMV_SEQ_START		S3C_FIMVREG(0x12c)
#define S3C_FIMV_SW_RESET		S3C_FIMVREG(0x130)
#define S3C_FIMV_FW_START		S3C_FIMVREG(0x134)
#define S3C_FIMV_ARM_ENDIAN		S3C_FIMVREG(0x138)
#define S3C_FIMV_RESERVE8		/* [(0x200-0x138)/4-1] */

/* Firmare loading */
/* MPEG4 encoder */
#define S3C_FIMV_FW_STT_ADR_0		S3C_FIMVREG(0x200)
/* MPEG4 decoder */
#define S3C_FIMV_FW_STT_ADR_1		S3C_FIMVREG(0x204)
/* H.264 encoder */
#define S3C_FIMV_FW_STT_ADR_2		S3C_FIMVREG(0x208)
/* H.264 decoder */
#define S3C_FIMV_FW_STT_ADR_3		S3C_FIMVREG(0x20c)
/* VC-1 decoder */
#define S3C_FIMV_FW_STT_ADR_4		S3C_FIMVREG(0x210)
/* MPEG2 decoder */
#define S3C_FIMV_FW_STT_ADR_5		S3C_FIMVREG(0x214)
/* H.263 decoder  */
#define S3C_FIMV_FW_STT_ADR_6		S3C_FIMVREG(0x218)
#define S3C_FIMV_RESERVE9		/* [(0x230-0x218)/4-1] */
#define S3C_FIMV_VSP_BUF_ADDR		S3C_FIMVREG(0x230)
#define S3C_FIMV_DB_STT_ADDR		S3C_FIMVREG(0x234)
#define S3C_FIMV_RESERVE10		/* [(0x300-0x234)/4-1] */

/* Initencoder */
#define S3C_FIMV_PROFILE		S3C_FIMVREG(0x300)
#define S3C_FIMV_IDR_PERIOD		S3C_FIMVREG(0x304)
#define S3C_FIMV_I_PERIOD		S3C_FIMVREG(0x308)
#define S3C_FIMV_FRAME_QP_INIT		S3C_FIMVREG(0x30c)
#define S3C_FIMV_ENTROPY_CON		S3C_FIMVREG(0x310)
#define S3C_FIMV_DEBLOCK_FILTER_OPTION	S3C_FIMVREG(0x314)
#define S3C_FIMV_SHORT_HD_ON		S3C_FIMVREG(0x318)
#define S3C_FIMV_MSLICE_ENA		S3C_FIMVREG(0x31c)
#define S3C_FIMV_MSLICE_SEL		S3C_FIMVREG(0x320)
#define S3C_FIMV_MSLICE_MB		S3C_FIMVREG(0x324)
#define S3C_FIMV_MSLICE_BYTE		S3C_FIMVREG(0x328)
#define S3C_FIMV_RESERVE11		/* [(0x400-0x328)/4-1] */

/* Initdecoder */
#define S3C_FIMV_DISPLAY_Y_ADR		S3C_FIMVREG(0x400)
#define S3C_FIMV_DISPLAY_C_ADR		S3C_FIMVREG(0x404)
#define S3C_FIMV_DISPLAY_STATUS		S3C_FIMVREG(0x408)
#define S3C_FIMV_HEADER_DONE		S3C_FIMVREG(0x40c)
#define S3C_FIMV_FRAME_NUM		S3C_FIMVREG(0x410)
#define S3C_FIMV_RESERVE12		/* [(0x500-0x410)/4-1] */

/* Commn Interrupt */
#define S3C_FIMV_INT_OFF		S3C_FIMVREG(0x500)
#define S3C_FIMV_INT_MODE		S3C_FIMVREG(0x504)
#define S3C_FIMV_INT_DONE_CLEAR		S3C_FIMVREG(0x508)
#define S3C_FIMV_OPERATION_DONE		S3C_FIMVREG(0x50c)
#define S3C_FIMV_FW_DONE		S3C_FIMVREG(0x510)
#define S3C_FIMV_INT_STATUS		S3C_FIMVREG(0x514)
#define S3C_FIMV_INT_MASK		S3C_FIMVREG(0x518)
#define S3C_FIMV_RESERVE13		/* [(0x600-0x518)/4-1] */

/* Mem onfig */
#define S3C_FIMV_TILE_MODE		S3C_FIMVREG(0x600)
#define S3C_FIMV_RESERVE14		/* [(0x700-0x600)/4-1] */

/* DEBU (will be removed) */
#define S3C_FIMV_ENC_STATUS_RDY		S3C_FIMVREG(0x700)
#define S3C_FIMV_DEC_STATUS_RDY		S3C_FIMVREG(0x704)
#define S3C_FIMV_RESERVE15		/* [(0x800-0x704)/4-1] */

/* Run ncoder */
#define S3C_FIMV_ENC_CUR_Y_ADR		S3C_FIMVREG(0x800)
#define S3C_FIMV_ENC_CUR_CBCR_ADR	S3C_FIMVREG(0x804)
#define S3C_FIMV_RESERVE16		/* 0x808 */
#define S3C_FIMV_ENC_DPB_ADR		S3C_FIMVREG(0x80c)
#define S3C_FIMV_CIR_MB_NUM		S3C_FIMVREG(0x810)
#define S3C_FIMV_RESERVE17		/* [(0x900-0x810)/4-1] */

/* Run ecoder */
#define S3C_FIMV_DEC_DPB_ADR		S3C_FIMVREG(0x900)
#define S3C_FIMV_DPB_COMV_ADR		S3C_FIMVREG(0x904)
#define S3C_FIMV_POST_ADR		S3C_FIMVREG(0x908)
#define S3C_FIMV_DPB_SIZE		S3C_FIMVREG(0x90c)
#define S3C_FIMV_RESERVE18		/* [(0xA00-0x90C)/4-1] */

/* Rate Control */
#define S3C_FIMV_RC_CONFIG		S3C_FIMVREG(0xa00)
#define S3C_FIMV_RC_FRAME_RATE		S3C_FIMVREG(0xa04)
#define S3C_FIMV_RC_BIT_RATE		S3C_FIMVREG(0xa08)
#define S3C_FIMV_RC_QBOUND		S3C_FIMVREG(0xa0c)
#define S3C_FIMV_RC_RPARA		S3C_FIMVREG(0xa10)
#define S3C_FIMV_RC_MB_CTRL		S3C_FIMVREG(0xa14)
#define S3C_FIMV_RC_QOUT		S3C_FIMVREG(0xa18)
#define S3C_FIMV_RC_FQCTRL		S3C_FIMVREG(0xa1c)
#define S3C_FIMV_RC_VBFULL		S3C_FIMVREG(0xa20)
#define S3C_FIMV_RC_RSEQ		S3C_FIMVREG(0xa24)
#define S3C_FIMV_RC_ACT_CONTROL		S3C_FIMVREG(0xa28)
#define S3C_FIMV_RC_ACT_SCALE		S3C_FIMVREG(0xa2C)
#define S3C_FIMV_RC_ACT_LIMIT		S3C_FIMVREG(0xa30)
#define S3C_FIMV_RC_ACT_SUM		S3C_FIMVREG(0xa34)
#define S3C_FIMV_RC_ACT_STATIC		S3C_FIMVREG(0xa38)
#define S3C_FIMV_RC_FLAT		S3C_FIMVREG(0xa3c)
#define S3C_FIMV_RC_FLAT_LIMIT		S3C_FIMVREG(0xa40)
#define S3C_FIMV_RC_DARK		S3C_FIMVREG(0xa44)
#define S3C_FIMV_RESERVE19		/* [(0xC00-0xA44)/4-1] */

/* Left[15:0]Right[31:16] OFFSET */
#define S3C_FIMV_CROP_INFO1		S3C_FIMVREG(0xc00)
/* Top[15:0]Bottom[31:16] OFFSET */
#define S3C_FIMV_CROP_INFO2		S3C_FIMVREG(0xc04)
/* decoding size for decoder, frame vop type for encoder */
#define S3C_FIMV_RET_VALUE		S3C_FIMVREG(0xc08)
#define S3C_FIMV_FRAME_TYPE		S3C_FIMVREG(0xc0C)
#define S3C_FIMV_RESERVE20		/* [(0xD00-0xC0C)/4-1] */

#define S3C_FIMV_COMMAND_TYPE		S3C_FIMVREG(0xd00)
#define S3C_FIMV_NUM_EXTRA_BUF		S3C_FIMVREG(0xd04)
#define S3C_FIMV_CODEC_COMMAND		S3C_FIMVREG(0xd08)

#elif defined(CONFIG_CPU_S5PV210)
#define S3C_FIMV_START_ADDR		S3C_FIMVREG(0x0000)
#define S3C_FIMV_END_ADDR		S3C_FIMVREG(0xe008)

#define S3C_FIMV_SW_RESET		S3C_FIMVREG(0x0000)
#define S3C_FIMV_RISC_HOST_INT		S3C_FIMVREG(0x0008)
/* Command from HOST to RISC */
#define S3C_FIMV_HOST2RISC_CMD		S3C_FIMVREG(0x0030)
#define S3C_FIMV_HOST2RISC_ARG1		S3C_FIMVREG(0x0034)
#define S3C_FIMV_HOST2RISC_ARG2		S3C_FIMVREG(0x0038)
#define S3C_FIMV_HOST2RISC_ARG3		S3C_FIMVREG(0x003c)
#define S3C_FIMV_HOST2RISC_ARG4		S3C_FIMVREG(0x0040)
/* Command from RISC to HOST */
#define S3C_FIMV_RISC2HOST_CMD		S3C_FIMVREG(0x0044)
#define S3C_FIMV_RISC2HOST_ARG1		S3C_FIMVREG(0x0048)
#define S3C_FIMV_RISC2HOST_ARG2		S3C_FIMVREG(0x004c)
#define S3C_FIMV_RISC2HOST_ARG3		S3C_FIMVREG(0x0050)
#define S3C_FIMV_RISC2HOST_ARG4		S3C_FIMVREG(0x0054)

#define S3C_FIMV_FW_VERSION		S3C_FIMVREG(0x0058)
#define S3C_FIMV_SYS_MEM_SZ		S3C_FIMVREG(0x005c)
#define S3C_FIMV_FW_STATUS		S3C_FIMVREG(0x0080)
/* Memory controller register */
#define S3C_FIMV_MC_DRAMBASE_ADR_A	S3C_FIMVREG(0x0508)
#define S3C_FIMV_MC_DRAMBASE_ADR_B	S3C_FIMVREG(0x050c)
#define S3C_FIMV_MC_STATUS		S3C_FIMVREG(0x0510)
#define S3C_FIMV_MC_RS_IBASE		S3C_FIMVREG(0x0514)

/* Common register */
/* firmware buffer */
#define S3C_FIMV_SYS_MEM_ADR		S3C_FIMVREG(0x0600)
/* stream buffer */
#define S3C_FIMV_CPB_BUF_ADR		S3C_FIMVREG(0x0604)
/* descriptor buffer */
#define S3C_FIMV_DESC_BUF_ADR		S3C_FIMVREG(0x0608)
/* Luma0 ~ Luma18 */
#define S3C_FIMV_LUMA_ADR		S3C_FIMVREG(0x0700)
/* Chroma0 ~ Chroma18 */
#define S3C_FIMV_CHROMA_ADR		S3C_FIMVREG(0x0600)


/* H264 decoding */
/* vertical neighbor motion vector */
#define S3C_FIMV_VERT_NB_MV_ADR		S3C_FIMVREG(0x068c)
/* neighbor pixels for intra pred */
#define S3C_FIMV_VERT_NB_IP_ADR		S3C_FIMVREG(0x0690)
/* H264 motion vector */
#define S3C_FIMV_MV_ADR			S3C_FIMVREG(0x0780)

/* H263/MPEG4/MPEG2/VC-1/ decoding */
/* neighbor AC/DC coeff. buffer */
#define S3C_FIMV_NB_DCAC_ADR		S3C_FIMVREG(0x068c)
/* upper neighbor motion vector buffer */
#define S3C_FIMV_UP_NB_MV_ADR		S3C_FIMVREG(0x0690)
/* subseq. anchor motion vector buffer */
#define S3C_FIMV_SA_MV_ADR		S3C_FIMVREG(0x0694)
/* overlap transform line buffer */
#define S3C_FIMV_OT_LINE_ADR		S3C_FIMVREG(0x0698)
/* bitplane3 addr */
#define S3C_FIMV_BITPLANE3_ADR		S3C_FIMVREG(0x069c)
/* bitplane2 addr */
#define S3C_FIMV_BITPLANE2_ADR		S3C_FIMVREG(0x06a0)
/* bitplane1 addr */
#define S3C_FIMV_BITPLANE1_ADR		S3C_FIMVREG(0x06a4)
/* syntax parser addr */
#define S3C_FIMV_SP_ADR			S3C_FIMVREG(0x06a8)

/* Encoder register */
/* upper motion vector addr */
#define S3C_FIMV_ENC_UP_MV_ADR		S3C_FIMVREG(0x0600)
/* direct cozero flag addr */
#define S3C_FIMV_ENC_COZERO_FLAG_ADR	S3C_FIMVREG(0x0610)
/* upper intra MD addr */
#define S3C_FIMV_ENC_UP_INTRA_MD_ADR	S3C_FIMVREG(0x0608)

/* upper intra PRED addr */
#define S3C_FIMV_ENC_UP_INTRA_PRED_ADR	S3C_FIMVREG(0x0740)


/* entropy engine's neighbor inform and AC/DC coeff. */
#define S3C_FIMV_ENC_NB_DCAC_ADR	S3C_FIMVREG(0x0604)
/* ref0 Luma addr */
#define S3C_FIMV_ENC_REF0_LUMA_ADR	S3C_FIMVREG(0x061c)

/* MFC fw 10/30, EVT0 */
#if defined(CONFIG_CPU_S5PV210_EVT0)
/* ref B Luma addr */
#define S3C_FIMV_ENC_REF_B_LUMA_ADR	S3C_FIMVREG(0x062c)
/* ref B Chroma addr */
#define S3C_FIMV_ENC_REF_B_CHROMA_ADR	S3C_FIMVREG(0x0630)
#endif

/* ref0 Chroma addr */
#define S3C_FIMV_ENC_REF0_CHROMA_ADR	S3C_FIMVREG(0x0700)
/* ref1 Luma addr */
#define S3C_FIMV_ENC_REF1_LUMA_ADR	S3C_FIMVREG(0x0620)
/* ref1 Chroma addr */
#define S3C_FIMV_ENC_REF1_CHROMA_ADR	S3C_FIMVREG(0x0704)
/* ref2 Luma addr */
#define S3C_FIMV_ENC_REF2_LUMA_ADR	S3C_FIMVREG(0x0710)
/* ref2 Chroma addr */
#define S3C_FIMV_ENC_REF2_CHROMA_ADR	S3C_FIMVREG(0x0708)
/* ref3 Luma addr */
#define S3C_FIMV_ENC_REF3_LUMA_ADR	S3C_FIMVREG(0x0714)
/* ref3 Chroma addr */
#define S3C_FIMV_ENC_REF3_CHROMA_ADR	S3C_FIMVREG(0x070c)

/* Codec common register */
/* frame width at encoder */
#define S3C_FIMV_ENC_HSIZE_PX		S3C_FIMVREG(0x0818)
/* frame height at encoder */
#define S3C_FIMV_ENC_VSIZE_PX		S3C_FIMVREG(0x081c)
/* profile register */
#define S3C_FIMV_ENC_PROFILE		S3C_FIMVREG(0x0830)
/* picture field/frame flag */
#define S3C_FIMV_ENC_PIC_STRUCT		S3C_FIMVREG(0x083c)
/* loop filter control */
#define S3C_FIMV_ENC_LF_CTRL		S3C_FIMVREG(0x0848)
/* loop filter alpha offset */
#define S3C_FIMV_ENC_ALPHA_OFF		S3C_FIMVREG(0x084c)
/* loop filter beta offset */
#define S3C_FIMV_ENC_BETA_OFF		S3C_FIMVREG(0x0850)
/* hidden, bus interface ctrl */
#define S3C_FIMV_MR_BUSIF_CTRL		S3C_FIMVREG(0x0854)
/* pixel cache control */
#define S3C_FIMV_ENC_PXL_CACHE_CTRL	S3C_FIMVREG(0x0a00)

/* Number of master port */
#define S3C_FIMV_NUM_MASTER		S3C_FIMVREG(0x0b14)

/* Channel & stream interface register */
/* Return CH instance ID register */
#define S3C_FIMV_SI_RTN_CHID		S3C_FIMVREG(0x2000)
/* codec instance ID */
#define S3C_FIMV_SI_CH1_INST_ID		S3C_FIMVREG(0x2040)
/* codec instance ID */
#define S3C_FIMV_SI_CH2_INST_ID		S3C_FIMVREG(0x2080)

/* Decoder */
/* vertical resolution of decoder */
#define S3C_FIMV_SI_VRESOL		S3C_FIMVREG(0x2004)
/* horizontal resolution of decoder */
#define S3C_FIMV_SI_HRESOL		S3C_FIMVREG(0x2008)
/* number of frames in the decoded pic */
#define S3C_FIMV_SI_BUF_NUMBER		S3C_FIMVREG(0x200c)
/* luma address of displayed pic */
#define S3C_FIMV_SI_DISPLAY_Y_ADR	S3C_FIMVREG(0x2010)
/* chroma address of displayed pic */
#define S3C_FIMV_SI_DISPLAY_C_ADR	S3C_FIMVREG(0x2014)
/* the number of frames so far decoded */
#define S3C_FIMV_SI_FRM_COUNT		S3C_FIMVREG(0x2018)
/* status of decoded picture */
#define S3C_FIMV_SI_DISPLAY_STATUS	S3C_FIMVREG(0x201c)
/* frame type such as skip/I/P/B */
#define S3C_FIMV_SI_FRAME_TYPE		S3C_FIMVREG(0x2020)

/* start addr of stream buf */
#define S3C_FIMV_SI_CH1_SB_ST_ADR	S3C_FIMVREG(0x2044)
/* size of stream buf */
#define S3C_FIMV_SI_CH1_SB_FRM_SIZE	S3C_FIMVREG(0x2048)
/* addr of descriptor buf */
#define S3C_FIMV_SI_CH1_DESC_ADR	S3C_FIMVREG(0x204c)
/* max size of coded pic. buf */
#define S3C_FIMV_SI_CH1_CPB_SIZE	S3C_FIMVREG(0x2058)
/* max size of descriptor buf */
#define S3C_FIMV_SI_CH1_DESC_SIZE	S3C_FIMVREG(0x205c)
/* release buffer register */
#define S3C_FIMV_SI_CH1_RELEASE_BUF	S3C_FIMVREG(0x2060)
/* Shared memory address */
#define S3C_FIMV_SI_CH1_HOST_WR_ADR	S3C_FIMVREG(0x2064)
/* DPB Configuration Control Register */
#define S3C_FIMV_SI_CH1_DPB_CONF_CTRL	S3C_FIMVREG(0x2068)

/* start addr of stream buf */
#define S3C_FIMV_SI_CH2_SB_ST_ADR	S3C_FIMVREG(0x2084)
/* size of stream buf */
#define S3C_FIMV_SI_CH2_SB_FRM_SIZE	S3C_FIMVREG(0x2088)
/* addr of descriptor buf */
#define S3C_FIMV_SI_CH2_DESC_ADR	S3C_FIMVREG(0x208c)
/* max size of coded pic. buf */
#define S3C_FIMV_SI_CH2_CPB_SIZE	S3C_FIMVREG(0x2098)
/* max size of descriptor buf */
#define S3C_FIMV_SI_CH2_DESC_SIZE	S3C_FIMVREG(0x209c)
/* release buffer register */
#define S3C_FIMV_SI_CH2_RELEASE_BUF	S3C_FIMVREG(0x20a0)

/* vertical resolution */
#define S3C_FIMV_SI_FIMV1_VRESOL	S3C_FIMVREG(0x2050)
/* horizontal resolution */
#define S3C_FIMV_SI_FIMV1_HRESOL	S3C_FIMVREG(0x2054)
/* luma crc data per frame(or top field) */
#define S3C_FIMV_CRC_LUMA0		S3C_FIMVREG(0x2030)
/* chroma crc data per frame(or top field) */
#define S3C_FIMV_CRC_CHROMA0		S3C_FIMVREG(0x2034)
/* luma crc data per bottom field */
#define S3C_FIMV_CRC_LUMA1		S3C_FIMVREG(0x2038)
/* chroma crc data per bottom field */
#define S3C_FIMV_CRC_CHROMA1		S3C_FIMVREG(0x203c)

/* Encoder */
/* stream size */
#define S3C_FIMV_ENC_SI_STRM_SIZE	S3C_FIMVREG(0x2004)
/* picture count */
#define S3C_FIMV_ENC_SI_PIC_CNT		S3C_FIMVREG(0x2008)
/* write pointer */
#define S3C_FIMV_ENC_SI_WRITE_PTR	S3C_FIMVREG(0x200c)
/* slice type(I/P/B/IDR) */
#define S3C_FIMV_ENC_SI_SLICE_TYPE	S3C_FIMVREG(0x2010)
/* the address of the encoded luminance picture*/
#define S3C_FIMV_ENCODED_Y_ADDR		S3C_FIMVREG(0x2014)
/* the address of the encoded chrominance picture*/
#define S3C_FIMV_ENCODED_C_ADDR		S3C_FIMVREG(0x2018)

/* addr of upper stream buf */
#define S3C_FIMV_ENC_SI_CH1_SB_U_ADR	S3C_FIMVREG(0x2044)
/* addr of lower stream buf */
#define S3C_FIMV_ENC_SI_CH1_SB_L_ADR	S3C_FIMVREG(0x2048)
/* size of stream buf */
#define S3C_FIMV_ENC_SI_CH1_SB_SIZE	S3C_FIMVREG(0x204c)
/* current Luma addr */
#define S3C_FIMV_ENC_SI_CH1_CUR_Y_ADR	S3C_FIMVREG(0x2050)
/* current Chroma addr */
#define S3C_FIMV_ENC_SI_CH1_CUR_C_ADR	S3C_FIMVREG(0x2054)
/* frame insertion control register */
#define S3C_FIMV_ENC_SI_CH1_FRAME_INS	S3C_FIMVREG(0x2058)
/* slice argument */
#define S3C_FIMV_ENC_SI_CH1_SLICE_ARG	S3C_FIMVREG(0x205c)

/* addr of upper stream buf */
#define S3C_FIMV_ENC_SI_CH2_SB_U_ADR	S3C_FIMVREG(0x2084)
/* addr of lower stream buf */
#define S3C_FIMV_ENC_SI_CH2_SB_L_ADR	S3C_FIMVREG(0x2088)
/* size of stream buf */
#define S3C_FIMV_ENC_SI_CH2_SB_SIZE	S3C_FIMVREG(0x208c)
/* current Luma addr */
#define S3C_FIMV_ENC_SI_CH2_CUR_Y_ADR	S3C_FIMVREG(0x2090)
/* current Chroma addr */
#define S3C_FIMV_ENC_SI_CH2_CUR_C_ADR	S3C_FIMVREG(0x2094)
/* frame QP */
#define S3C_FIMV_ENC_SI_CH2_FRAME_QP	S3C_FIMVREG(0x2098)
/* slice argument */
#define S3C_FIMV_ENC_SI_CH2_SLICE_ARG	S3C_FIMVREG(0x209c)


/* upper stream buf full status */
#define S3C_FIMV_ENC_STR_BF_U_FULL	S3C_FIMVREG(0xc004)
/* upper stream buf empty status */
#define S3C_FIMV_ENC_STR_BF_U_EMPTY	S3C_FIMVREG(0xc008)
/* lower stream buf full status */
#define S3C_FIMV_ENC_STR_BF_L_FULL	S3C_FIMVREG(0xc00c)
/* lower stream buf empty status */
#define S3C_FIMV_ENC_STR_BF_L_EMPTY	S3C_FIMVREG(0xc010)
/* stream buf interrupt status */
#define S3C_FIMV_ENC_STR_STATUS		S3C_FIMVREG(0xc018)
/* stream control */
#define S3C_FIMV_ENC_SF_EPB_ON_CTRL	S3C_FIMVREG(0xc054)
/* buffer control */
#define S3C_FIMV_ENC_SF_BUF_CTRL	S3C_FIMVREG(0xc058)
/* fifo level control */
#define S3C_FIMV_ENC_BF_MODE_CTRL	S3C_FIMVREG(0xc05c)

/* pic type level control */
#define S3C_FIMV_ENC_PIC_TYPE_CTRL	S3C_FIMVREG(0xc504)
/* B frame recon data write cotrl */
#define S3C_FIMV_ENC_B_RECON_WRITE_ON	S3C_FIMVREG(0xc508)
/* multi slice control */
#define S3C_FIMV_ENC_MSLICE_CTRL	S3C_FIMVREG(0xc50c)
/* MB number in the one slice */
#define S3C_FIMV_ENC_MSLICE_MB		S3C_FIMVREG(0xc510)
/* byte count number for one slice */
#define S3C_FIMV_ENC_MSLICE_BYTE	S3C_FIMVREG(0xc514)
/* number of intra refresh MB */
#define S3C_FIMV_ENC_CIR_CTRL		S3C_FIMVREG(0xc518)
/* linear or 64x32 tiled mode */
#define S3C_FIMV_ENC_MAP_FOR_CUR	S3C_FIMVREG(0xc51c)
/* padding control */
#define S3C_FIMV_ENC_PADDING_CTRL	S3C_FIMVREG(0xc520)
/* interrupt mask */
#define S3C_FIMV_ENC_INT_MASK		S3C_FIMVREG(0xc528)

/* RC config */
#define S3C_FIMV_ENC_RC_CONFIG		S3C_FIMVREG(0xc5a0)
/* bit rate */
#define S3C_FIMV_ENC_RC_BIT_RATE	S3C_FIMVREG(0xc5a8)
/* max/min QP */
#define S3C_FIMV_ENC_RC_QBOUND		S3C_FIMVREG(0xc5ac)
/* rate control reaction coeff. */
#define S3C_FIMV_ENC_RC_RPARA		S3C_FIMVREG(0xc5b0)
/* MB adaptive scaling */
#define S3C_FIMV_ENC_RC_MB_CTRL		S3C_FIMVREG(0xc5b4)

/* for MFC fw 2010/04/09, addr changed from 0xc5a4 to 0xd0d0 */
/* frame rate */
#define S3C_FIMV_ENC_RC_FRAME_RATE	S3C_FIMVREG(0xd0d0)

/* Encoder for H264 */
/* CAVLC or CABAC */
#define S3C_FIMV_ENC_ENTRP_MODE		S3C_FIMVREG(0xd004)
/* loop filter alpha offset */
#define S3C_FIMV_ENC_H264_ALPHA_OFF	S3C_FIMVREG(0xd008)
/* loop filter beta offset */
#define S3C_FIMV_ENC_H264_BETA_OFF	S3C_FIMVREG(0xd00c)
/* number of reference for P/B */
#define S3C_FIMV_ENC_H264_NUM_OF_REF	S3C_FIMVREG(0xd010)
/* inter weighted parameter */
#define S3C_FIMV_ENC_H264_MDINTER_WGT	S3C_FIMVREG(0xd01c)
/* intra weighted parameter */
#define S3C_FIMV_ENC_H264_MDINTRA_WGT	S3C_FIMVREG(0xd020)
/* 8x8 transform flag in PPS & high profile */
#define S3C_FIMV_ENC_H264_TRANS_FLAG	S3C_FIMVREG(0xd034)


/* Encoder for MPEG4 */
/* quarter pel interpolation control */
#define S3C_FIMV_ENC_MPEG4_QUART_PXL	S3C_FIMVREG(0xe008)

#endif /* CONFIG_CPU_S5PC1xx */

#endif
