/*
 * linux/drivers/media/video/samsung/mfc5x/mfc_shm.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Shared memory interface for Samsung MFC (Multi Function Codec - FIMV) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __MFC_SHM_H
#define __MFC_SHM_H __FILE__

enum MFC_SHM_OFS
{
	EXTENEDED_DECODE_STATUS		= 0x00,	/* D */
	SET_FRAME_TAG			= 0x04, /* D */
	GET_FRAME_TAG_TOP		= 0x08, /* D */
	GET_FRAME_TAG_BOT		= 0x0C, /* D */
	PIC_TIME_TOP			= 0x10, /* D */
	PIC_TIME_BOT			= 0x14, /* D */
	START_BYTE_NUM			= 0x18, /* D */
	/*
	DEC_FRM_SIZE			= 0x1C,
	*/
	CROP_INFO1			= 0x20, /* D */
	CROP_INFO2			= 0x24, /* D */
	EXT_ENC_CONTROL			= 0x28,	/* E */
	ENC_PARAM_CHANGE		= 0x2C,	/* E */
	VOP_TIMING			= 0x30,	/* E, MPEG4 */
	HEC_PERIOD			= 0x34,	/* E, MPEG4 */
	METADATA_ENABLE			= 0x38, /* C */
	METADATA_STATUS			= 0x3C, /* C */
	METADATA_DISPLAY_INDEX		= 0x40,	/* C */
	EXT_METADATA_START_ADDR		= 0x44, /* C */
	PUT_EXTRADATA			= 0x48, /* C */
	EXTRADATA_ADDR			= 0x4C, /* C */
	/*
	DBG_INFO_OUTPUT1		= 0x50,
	DBG_INFO_INPUT0			= 0x54,
	DBG_INFO_INPUT1			= 0x58,
	REF_L0_PHY_IDX			= 0x5C,
	REF_L1_PHY_IDX			= 0x60,
	*/
	ALLOCATED_LUMA_DPB_SIZE		= 0x64,	/* D */
	ALLOCATED_CHROMA_DPB_SIZE	= 0x68,	/* D */
	ALLOCATED_MV_SIZE		= 0x6C,	/* D */
	P_B_FRAME_QP			= 0x70,	/* E */
	ASPECT_RATIO_IDC		= 0x74, /* E, H.264, depend on ASPECT_RATIO_VUI_ENABLE in EXT_ENC_CONTROL */
	EXTENDED_SAR			= 0x78, /* E, H.264, depned on ASPECT_RATIO_VUI_ENABLE in EXT_ENC_CONTROL */
	DISP_PIC_PROFILE		= 0x7C, /* D */
	FLUSH_CMD_TYPE			= 0x80, /* C */
	FLUSH_CMD_INBUF1		= 0x84, /* C */
	FLUSH_CMD_INBUF2		= 0x88, /* C */
	FLUSH_CMD_OUTBUF		= 0x8C, /* E */
	NEW_RC_BIT_RATE			= 0x90, /* E, format as RC_BIT_RATE(0xC5A8) depend on RC_BIT_RATE_CHANGE in ENC_PARAM_CHANGE */
	NEW_RC_FRAME_RATE		= 0x94, /* E, format as RC_FRAME_RATE(0xD0D0) depend on RC_FRAME_RATE_CHANGE in ENC_PARAM_CHANGE */
	NEW_I_PERIOD			= 0x98, /* E, format as I_FRM_CTRL(0xC504) depend on I_PERIOD_CHANGE in ENC_PARAM_CHANGE */
	H264_I_PERIOD			= 0x9C, /* E, H.264, open GOP */
	RC_CONTROL_CONFIG		= 0xA0, /* E */
	BATCH_INPUT_ADDR		= 0xA4, /* E */
	BATCH_OUTPUT_ADDR		= 0xA8, /* E */
	BATCH_OUTPUT_SIZE		= 0xAC, /* E */
	MIN_LUMA_DPB_SIZE		= 0xB0, /* D */
	DEVICE_FORMAT_ID		= 0xB4, /* C */
	H264_POC_TYPE			= 0xB8, /* D */
	MIN_CHROMA_DPB_SIZE		= 0xBC, /* D */
	DISP_PIC_FRAME_TYPE		= 0xC0, /* D */
	FREE_LUMA_DPB			= 0xC4, /* D, VC1 MPEG4 */
	ASPECT_RATIO_INFO		= 0xC8, /* D, MPEG4 */
	EXTENDED_PAR			= 0xCC, /* D, MPEG4 */
	DBG_HISTORY_INPUT0		= 0xD0, /* C */
	DBG_HISTORY_INPUT1		= 0xD4,	/* C */
	DBG_HISTORY_OUTPUT		= 0xD8,	/* C */
	HIERARCHICAL_P_QP		= 0xE0, /* E, H.264 */
};

int init_shm(struct mfc_inst_ctx *ctx);
void write_shm(struct mfc_inst_ctx *ctx, unsigned int data, unsigned int offset);
unsigned int read_shm(struct mfc_inst_ctx *ctx, unsigned int offset);

#endif /* __MFC_SHM_H */
