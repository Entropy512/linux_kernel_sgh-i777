/*
 * linux/drivers/media/video/samsung/mfc5x/mfc_enc.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Encoder interface for Samsung MFC (Multi Function Codec - FIMV) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <asm/cacheflush.h>

#include <linux/kernel.h>
#include <linux/slab.h>
#ifdef CONFIG_CPU_FREQ
#include <mach/cpufreq.h>
#endif


#if defined(CONFIG_ARCH_S5PV310)
#include <mach/regs-mfc.h>
#elif defined(CONFIG_ARCH_S5PV210)
#include <plat/regs-mfc.h>
#endif

#include "mfc_enc.h"
#include "mfc_cmd.h"
#include "mfc_log.h"

#include "mfc_shm.h"
#include "mfc_reg.h"
#include "mfc_mem.h"
#include "mfc_buf.h"
#include "mfc_interface.h"

static LIST_HEAD(mfc_encoders);

/*
 * [1] alloc_ctx_buf() implementations
 */
 static int alloc_ctx_buf(struct mfc_inst_ctx *ctx)
{
	struct mfc_alloc_buffer *alloc;

	alloc = _mfc_alloc_buf(ctx, MFC_CTX_SIZE, ALIGN_2KB, MBT_CTX | PORT_A);
	if (alloc == NULL) {
		mfc_err("failed alloc context buffer\n");

		return -1;
	}

	ctx->ctxbufofs = mfc_mem_base_ofs(alloc->real) >> 11;
	ctx->ctxbufsize = alloc->size;

	memset((void *)alloc->addr, 0, alloc->size);

	mfc_mem_cache_clean((void *)alloc->addr, alloc->size);

	return 0;
}

/*
 * [2] get_init_arg() implementations
 */
int get_init_arg(struct mfc_inst_ctx *ctx, void *arg)
{
	struct mfc_enc_init_arg *init_arg;
	struct mfc_enc_ctx *enc_ctx;
	unsigned int reg;

	init_arg = (struct mfc_enc_init_arg *)arg;
	enc_ctx = (struct mfc_enc_ctx *)ctx->c_priv;

	/* Check input stream mode NV12_LINEAR OR NV12_TILE */
	if (init_arg->cmn.in_frame_map == NV12_TILE)
		enc_ctx->framemap = 3;	/* MFC_ENC_MAP_FOR_CUR 0: Linear mode 3: Tile mode */
	else
		enc_ctx->framemap = 0;	/* Default is Linear mode */

	/* width */
	write_reg(init_arg->cmn.in_width, MFC_ENC_HSIZE_PX);
	/* height */
	write_reg(init_arg->cmn.in_height, MFC_ENC_VSIZE_PX);

	/* FIXME: MFC_B_RECON_*_ADR */
	write_reg(0, MFC_ENC_B_RECON_WRITE_ON);

	/* multi-slice control 0 / 1 / 3 */
	/* multi-slice MB number or multi-slice bit size */
	if (init_arg->cmn.in_ms_mode == 1) {
		write_reg((0 << 1) | 0x1, MFC_ENC_MSLICE_CTRL);
		write_reg(init_arg->cmn.in_ms_arg & 0xFFFF, MFC_ENC_MSLICE_MB);
	} else if (init_arg->cmn.in_ms_mode == 2) {
		write_reg((1 << 1) | 0x1, MFC_ENC_MSLICE_CTRL);
		if (init_arg->cmn.in_ms_arg < 1900)
			init_arg->cmn.in_ms_arg = 1900;
		write_reg(init_arg->cmn.in_ms_arg, MFC_ENC_MSLICE_BIT);
	} else {
		write_reg(0, MFC_ENC_MSLICE_CTRL);
		write_reg(0, MFC_ENC_MSLICE_MB);
		write_reg(0, MFC_ENC_MSLICE_BIT);
	}

	/* cyclic intra refresh */
	write_reg(init_arg->cmn.in_mb_refresh & 0xFFFF, MFC_ENC_CIR_CTRL);
	/* memory structure of the current frame - 0 -> Linear  or 3 -> Tile mode */
	write_reg(enc_ctx->framemap, MFC_ENC_MAP_FOR_CUR);

	/* padding control & value */
	reg = read_reg(MFC_ENC_PADDING_CTRL);
	if (init_arg->cmn.in_pad_ctrl_on > 0) {
		/** enable */
		reg |= (1 << 31);
		/** cr value */
		reg &= ~(0xFF << 16);
		reg |= ((init_arg->cmn.in_cr_pad_val & 0xFF) << 16);
		/** cb value */
		reg &= ~(0xFF << 8);
		reg |= ((init_arg->cmn.in_cb_pad_val & 0xFF) << 8);
		/** y value */
		reg &= ~(0xFF << 0);
		reg |= ((init_arg->cmn.in_y_pad_val & 0xFF) << 0);
	} else {
		/** disable & all value clear */
		reg = 0;
	}
	write_reg(reg, MFC_ENC_PADDING_CTRL);

	/* reaction coefficient */
	if (init_arg->cmn.in_rc_fr_en > 0) {
		if (init_arg->cmn.in_rc_rpara != 0)
			write_reg(init_arg->cmn.in_rc_rpara & 0xFFFF, MFC_ENC_RC_RPARA);
	} else {
		write_reg(0, MFC_ENC_RC_RPARA);
	}

	/* FIXME: update shm parameters? */

	return 0;
}

int h263_get_init_arg(struct mfc_inst_ctx *ctx, void *arg)
{
	struct mfc_enc_init_arg *init_arg;
	struct mfc_enc_init_h263_arg *init_h263_arg;
	unsigned int reg;
	unsigned int shm;

	get_init_arg(ctx, arg);

	init_arg = (struct mfc_enc_init_arg *)arg;
	init_h263_arg = &init_arg->codec.h263;

	/* pictype : number of B, IDR period */
	reg = read_reg(MFC_ENC_PIC_TYPE_CTRL);
	/** enable - 0 / 1*/
	reg |= (1 << 18);
	/** numbframe - 0 ~ 2 */
	reg &= ~(0x3 << 16);
	/** idrperiod - 0 ~ */
	reg &= ~(0xFFFF << 0);
	reg |= ((init_arg->cmn.in_gop_num & 0xFFFF) << 0);
	write_reg(reg, MFC_ENC_PIC_TYPE_CTRL);

	/* rate control config. */
	reg = read_reg(MFC_ENC_RC_CONFIG);
	/** frame-level rate control */
	reg &= ~(0x1 << 9);
	reg |= ((init_arg->cmn.in_rc_fr_en & 0x1) << 9);
	/** macroblock-level rate control */
	reg &= ~(0x1 << 8);
	/** frame QP */
	if (init_arg->cmn.in_vop_quant < 1)
		init_arg->cmn.in_vop_quant = 1;
	else if (init_arg->cmn.in_vop_quant > 31)
		init_arg->cmn.in_vop_quant = 31;
	reg &= ~(0x3F << 0);
	reg |= ((init_arg->cmn.in_vop_quant & 0x3F) << 0);
	write_reg(reg, MFC_ENC_RC_CONFIG);

	/* frame rate and bit rate */
	if (init_arg->cmn.in_rc_fr_en > 0) {
		if (init_h263_arg->in_rc_framerate != 0)
			write_reg(init_h263_arg->in_rc_framerate * 1000,
				MFC_ENC_RC_FRAME_RATE);

		if (init_arg->cmn.in_rc_bitrate != 0)
			write_reg(init_arg->cmn.in_rc_bitrate,
				MFC_ENC_RC_BIT_RATE);
	} else {
		write_reg(0, MFC_ENC_RC_FRAME_RATE);
		write_reg(0, MFC_ENC_RC_BIT_RATE);
	}

	/* max & min value of QP */
	reg = read_reg(MFC_ENC_RC_QBOUND);
	/** max QP */
	if (init_arg->cmn.in_rc_qbound_max < 1)
		init_arg->cmn.in_rc_qbound_max = 1;
	else if (init_arg->cmn.in_rc_qbound_max > 31)
		init_arg->cmn.in_rc_qbound_max = 31;
	reg &= ~(0x3F << 8);
	reg |= ((init_arg->cmn.in_rc_qbound_max & 0x3F) << 8);
	/** min QP */
	if (init_arg->cmn.in_rc_qbound_min < 1)
		init_arg->cmn.in_rc_qbound_min = 1;
	else if (init_arg->cmn.in_rc_qbound_min > 31)
		init_arg->cmn.in_rc_qbound_min = 31;
	if (init_arg->cmn.in_rc_qbound_min > init_arg->cmn.in_rc_qbound_max)
		init_arg->cmn.in_rc_qbound_min = init_arg->cmn.in_rc_qbound_max;
	reg &= ~(0x3F << 0);
	reg |= ((init_arg->cmn.in_rc_qbound_min & 0x3F) << 0);
	write_reg(reg, MFC_ENC_RC_QBOUND);

	if (init_arg->cmn.in_rc_fr_en > 0) {
		shm = read_shm(ctx, P_B_FRAME_QP);
		shm &= ~(0xFFF << 0);
		shm |= ((init_arg->cmn.in_vop_quant_p & 0x3F) << 0);
		write_shm(ctx, shm, P_B_FRAME_QP);
	}

	return 0;
}

int mpeg4_get_init_arg(struct mfc_inst_ctx *ctx, void *arg)
{
	struct mfc_enc_init_arg *init_arg;
	struct mfc_enc_init_mpeg4_arg *init_mpeg4_arg;
	unsigned int reg;
	unsigned int shm;

	get_init_arg(ctx, arg);

	init_arg = (struct mfc_enc_init_arg *)arg;
	init_mpeg4_arg = &init_arg->codec.mpeg4;

	/* profile & level */
	reg = read_reg(MFC_ENC_PROFILE);
	/** level */
	reg &= ~(0xFF << 8);
	reg |= ((init_mpeg4_arg->in_level & 0xFF) << 8);
	/** profile - 0 ~ 2 */
	reg &= ~(0x3 << 0);
	reg |= ((init_mpeg4_arg->in_profile & 0x3) << 0);
	write_reg(reg, MFC_ENC_PROFILE);

	/* pictype : number of B, IDR period */
	reg = read_reg(MFC_ENC_PIC_TYPE_CTRL);
	/** enable - 0 / 1*/
	reg |= (1 << 18);
	/** numbframe - 0 ~ 2 */
	reg &= ~(0x3 << 16);
	reg |= ((init_mpeg4_arg->in_bframenum & 0x3) << 16);
	/** idrperiod - 0 ~ */
	reg &= ~(0xFFFF << 0);
	reg |= ((init_arg->cmn.in_gop_num & 0xFFFF) << 0);
	write_reg(reg, MFC_ENC_PIC_TYPE_CTRL);

	/* rate control config. */
	reg = read_reg(MFC_ENC_RC_CONFIG);
	/** frame-level rate control */
	reg &= ~(0x1 << 9);
	reg |= ((init_arg->cmn.in_rc_fr_en & 0x1) << 9);
	/** macroblock-level rate control */
	reg &= ~(0x1 << 8);
	/** frame QP */
	if (init_arg->cmn.in_vop_quant < 1)
		init_arg->cmn.in_vop_quant = 1;
	else if (init_arg->cmn.in_vop_quant > 31)
		init_arg->cmn.in_vop_quant = 31;
	reg &= ~(0x3F << 0);
	reg |= ((init_arg->cmn.in_vop_quant & 0x3F) << 0);
	write_reg(reg, MFC_ENC_RC_CONFIG);

	/* frame rate and bit rate */
	if (init_arg->cmn.in_rc_fr_en > 0) {
		if (init_mpeg4_arg->in_VopTimeIncreament > 0)
			write_reg((init_mpeg4_arg->in_TimeIncreamentRes /
				init_mpeg4_arg->in_VopTimeIncreament) * 1000,
				MFC_ENC_RC_FRAME_RATE);

		if (init_arg->cmn.in_rc_bitrate != 0)
			write_reg(init_arg->cmn.in_rc_bitrate,
				MFC_ENC_RC_BIT_RATE);
	} else {
		write_reg(0, MFC_ENC_RC_FRAME_RATE);
		write_reg(0, MFC_ENC_RC_BIT_RATE);
	}

	/* max & min value of QP */
	reg = read_reg(MFC_ENC_RC_QBOUND);
	/** max QP */
	if (init_arg->cmn.in_rc_qbound_max < 1)
		init_arg->cmn.in_rc_qbound_max = 1;
	else if (init_arg->cmn.in_rc_qbound_max > 31)
		init_arg->cmn.in_rc_qbound_max = 31;
	reg &= ~(0x3F << 8);
	reg |= ((init_arg->cmn.in_rc_qbound_max & 0x3F) << 8);
	/** min QP */
	if (init_arg->cmn.in_rc_qbound_min < 1)
		init_arg->cmn.in_rc_qbound_min = 1;
	else if (init_arg->cmn.in_rc_qbound_min > 31)
		init_arg->cmn.in_rc_qbound_min = 31;
	if (init_arg->cmn.in_rc_qbound_min > init_arg->cmn.in_rc_qbound_max)
		init_arg->cmn.in_rc_qbound_min = init_arg->cmn.in_rc_qbound_max;
	reg &= ~(0x3F << 0);
	reg |= ((init_arg->cmn.in_rc_qbound_min & 0x3F) << 0);
	write_reg(reg, MFC_ENC_RC_QBOUND);

	write_reg(init_mpeg4_arg->in_quart_pixel, MFC_ENC_MPEG4_QUART_PXL);

	if (init_arg->cmn.in_rc_fr_en > 0) {
		shm = read_shm(ctx, P_B_FRAME_QP);
		shm &= ~(0xFFF << 0);
		shm |= ((init_mpeg4_arg->in_vop_quant_b & 0x3F) << 6);
		shm |= ((init_arg->cmn.in_vop_quant_p & 0x3F) << 0);
		write_shm(ctx, shm, P_B_FRAME_QP);
	}

	return 0;
}

int h264_get_init_arg(struct mfc_inst_ctx *ctx, void *arg)
{
	struct mfc_enc_init_arg *init_arg;
	struct mfc_enc_init_h264_arg *init_h264_arg;
	unsigned int reg;
	unsigned int shm;

	get_init_arg(ctx, arg);

	init_arg = (struct mfc_enc_init_arg *)arg;
	init_h264_arg = &init_arg->codec.h264;

	/* height */
	if (init_h264_arg->in_interlace_mode)
		write_reg(init_arg->cmn.in_height >> 1, MFC_ENC_VSIZE_PX);
	else
		write_reg(init_arg->cmn.in_height, MFC_ENC_VSIZE_PX);

	/* profile & level */
	reg = read_reg(MFC_ENC_PROFILE);
	/** level */
	reg &= ~(0xFF << 8);
	reg |= ((init_h264_arg->in_level & 0xFF) << 8);
	/** profile - 0 ~ 2 */
	reg &= ~(0x3 << 0);
	reg |= ((init_h264_arg->in_profile & 0x3) << 0);
	write_reg(reg, MFC_ENC_PROFILE);

	/* interface - 0 / 1 */
	write_reg(init_h264_arg->in_interlace_mode & 0x1, MFC_ENC_PIC_STRUCT);

	/* loopfilter disable - 0 ~ 2 */
	write_reg((init_h264_arg->in_deblock_dis & 0x3), MFC_ENC_LF_CTRL);

	/* loopfilter alpha & C0 offset - -6 ~ 6 */
	write_reg((init_h264_arg->in_deblock_alpha_c0 & 0x1F) * 2, MFC_ENC_ALPHA_OFF);

	/* loopfilter beta offset - -6 ~ 6 */
	write_reg((init_h264_arg->in_deblock_beta & 0x1F) * 2, MFC_ENC_BETA_OFF);

	/* pictype : number of B, IDR period */
	reg = read_reg(MFC_ENC_PIC_TYPE_CTRL);
	/** enable - 0 / 1*/
	reg |= (1 << 18);
	/** numbframe - 0 ~ 2 */
	reg &= ~(0x3 << 16);
	reg |= ((init_h264_arg->in_bframenum & 0x3) << 16);
	/** idrperiod - 0 ~ */
	reg &= ~(0xFFFF << 0);
	reg |= ((init_arg->cmn.in_gop_num & 0xFFFF) << 0);
	write_reg(reg, MFC_ENC_PIC_TYPE_CTRL);

	/* rate control config. */
	reg = read_reg(MFC_ENC_RC_CONFIG);
	/** frame-level rate control */
	reg &= ~(0x1 << 9);
	reg |= ((init_arg->cmn.in_rc_fr_en & 0x1) << 9);
	/** macroblock-level rate control */
	reg &= ~(0x1 << 8);
	reg |= ((init_h264_arg->in_rc_mb_en & 0x1) << 8);
	/** frame QP */
	if (init_arg->cmn.in_vop_quant < 1)
		init_arg->cmn.in_vop_quant = 1;
	else if (init_arg->cmn.in_vop_quant > 51)
		init_arg->cmn.in_vop_quant = 51;
	reg &= ~(0x3F << 0);
	reg |= ((init_arg->cmn.in_vop_quant & 0x3F) << 0);
	write_reg(reg, MFC_ENC_RC_CONFIG);

	/* frame rate and bit rate */
	if (init_arg->cmn.in_rc_fr_en > 0) {
		if (init_h264_arg->in_rc_framerate != 0)
			write_reg(init_h264_arg->in_rc_framerate * 1000,
				MFC_ENC_RC_FRAME_RATE);

		if (init_arg->cmn.in_rc_bitrate != 0)
			write_reg(init_arg->cmn.in_rc_bitrate,
				MFC_ENC_RC_BIT_RATE);
	} else {
		write_reg(0, MFC_ENC_RC_FRAME_RATE);
		write_reg(0, MFC_ENC_RC_BIT_RATE);
	}

	/* max & min value of QP */
	reg = read_reg(MFC_ENC_RC_QBOUND);
	/** max QP */
	if (init_arg->cmn.in_rc_qbound_max < 1)
		init_arg->cmn.in_rc_qbound_max = 1;
	else if (init_arg->cmn.in_rc_qbound_max > 51)
		init_arg->cmn.in_rc_qbound_max = 51;
	reg &= ~(0x3F << 8);
	reg |= ((init_arg->cmn.in_rc_qbound_max & 0x3F) << 8);
	/** min QP */
	if (init_arg->cmn.in_rc_qbound_min < 1)
		init_arg->cmn.in_rc_qbound_min = 1;
	else if (init_arg->cmn.in_rc_qbound_min > 51)
		init_arg->cmn.in_rc_qbound_min = 51;
	if (init_arg->cmn.in_rc_qbound_min > init_arg->cmn.in_rc_qbound_max)
		init_arg->cmn.in_rc_qbound_min = init_arg->cmn.in_rc_qbound_max;
	reg &= ~(0x3F << 0);
	reg |= ((init_arg->cmn.in_rc_qbound_min & 0x3F) << 0);
	write_reg(reg, MFC_ENC_RC_QBOUND);

	/* macroblock adaptive scaling features */
	if (init_h264_arg->in_rc_mb_en > 0) {
		reg = read_reg(MFC_ENC_RC_MB_CTRL);
		/** dark region */
		reg &= ~(0x1 << 3);
		reg |= ((init_h264_arg->in_rc_mb_dark_dis & 0x1) << 3);
		/** smooth region */
		reg &= ~(0x1 << 2);
		reg |= ((init_h264_arg->in_rc_mb_smooth_dis & 0x1) << 2);
		/** static region */
		reg &= ~(0x1 << 1);
		reg |= ((init_h264_arg->in_rc_mb_static_dis & 0x1) << 1);
		/** high activity region */
		reg &= ~(0x1 << 0);
		reg |= ((init_h264_arg->in_rc_mb_activity_dis & 0x1) << 0);
		write_reg(reg, MFC_ENC_RC_MB_CTRL);
	}

	/* entropy coding mode 0: CAVLC, 1: CABAC */
	write_reg(init_h264_arg->in_symbolmode & 0x1, MFC_ENC_H264_ENTRP_MODE);

	/* number of ref. picture */
	reg = read_reg(MFC_ENC_H264_NUM_OF_REF);
	/** num of ref. pictures of P */
	reg &= ~(0x3 << 5);
	reg |= ((init_h264_arg->in_ref_num_p & 0x3) << 5);
	/** max number of ref. pictures */
	reg &= ~(0x1F << 0);
	reg |= ((init_h264_arg->in_reference_num & 0x1F) << 0);
	write_reg(reg, MFC_ENC_H264_NUM_OF_REF);

	/* 8x8 transform enable */
	write_reg(init_h264_arg->in_transform8x8_mode & 0x1, MFC_ENC_H264_TRANS_FLAG);

	if ((init_arg->cmn.in_rc_fr_en > 0) && (init_h264_arg->in_rc_mb_en > 0)) {
		shm = read_shm(ctx, P_B_FRAME_QP);
		shm &= ~(0xFFF << 0);
		shm |= ((init_h264_arg->in_vop_quant_b & 0x3F) << 6);
		shm |= ((init_arg->cmn.in_vop_quant_p & 0x3F) << 0);
		write_shm(ctx, shm, P_B_FRAME_QP);
	}

	return 0;
}

/*
 * [3] pre_seq_start() implementations
 */
static int pre_seq_start(struct mfc_inst_ctx *ctx)
{
	/*
	struct mfc_enc_ctx *enc_ctx = (struct mfc_enc_ctx *)ctx->c_priv;
	unsigned int reg;
	*/

	return 0;
}

static int h264_pre_seq_start(struct mfc_inst_ctx *ctx)
{
	struct mfc_enc_ctx *enc_ctx = (struct mfc_enc_ctx *)ctx->c_priv;
	/*
	unsigned int reg;
	unsigned int shm;
	*/

	#if 0
	/* MFC fw 9/30, set the QP for P/B */
	if (mfc_ctx->MfcCodecType == H263_ENC)
		init_arg->in_vop_quant_b = 0;
	write_shm((init_arg->
			   in_vop_quant_p) | (init_arg->in_vop_quant_b << 6),
			  mfc_ctx->shared_mem_vir_addr + 0x70);

	/* MFC fw 11/10 */
	if (mfc_ctx->MfcCodecType == H264_ENC) {
		write_shm((mfc_ctx->vui_enable << 15) |
				  (mfc_ctx->hier_p_enable << 4) |
				  (mfc_ctx->frameSkipEnable << 1),
				  mfc_ctx->shared_mem_vir_addr + 0x28);
		if (mfc_ctx->vui_enable)
			write_shm((mfc_ctx->
					   vui_info.aspect_ratio_idc & 0xff),
					  mfc_ctx->shared_mem_vir_addr + 0x74);
		/* MFC fw 2010/04/09 */
		if (mfc_ctx->hier_p_enable)
			write_shm((mfc_ctx->hier_p_qp.t3_frame_qp << 12) |
					  (mfc_ctx->hier_p_qp.t2_frame_qp << 6) |
					  (mfc_ctx->hier_p_qp.t0_frame_qp),
					  mfc_ctx->shared_mem_vir_addr + 0xe0);
	} else
		write_shm((mfc_ctx->frameSkipEnable << 1),
				  mfc_ctx->shared_mem_vir_addr + 0x28);

	/* MFC fw 10/30, set vop_time_resolution, frame_delta */
	if (mfc_ctx->MfcCodecType == MPEG4_ENC)
		write_shm((1 << 31) |
				  (init_arg->in_TimeIncreamentRes << 16) |
				  (init_arg->in_VopTimeIncreament),
				  mfc_ctx->shared_mem_vir_addr + 0x30);

	if ((mfc_ctx->MfcCodecType == H264_ENC)
	    && (mfc_ctx->h264_i_period_enable)) {
		write_shm((1 << 16) | (mfc_ctx->h264_i_period),
				  mfc_ctx->shared_mem_vir_addr + 0x9c);
	}
	#endif

	/* Set stream buffer addr */
	write_reg(mfc_mem_base_ofs(enc_ctx->streamaddr) >> 11, MFC_ENC_SI_CH1_SB_ADR);
	write_reg(enc_ctx->streamsize, MFC_ENC_SI_CH1_SB_SIZE);

	return 0;
}

/*
 * [4] post_seq_start() implementations
 */
static int post_seq_start(struct mfc_inst_ctx *ctx)
{
	/*
	struct mfc_enc_ctx *enc_ctx = (struct mfc_enc_ctx *)ctx->c_priv;
	int i;
	*/

	/*
	unsigned int shm;
	*/

	/*
	mfc_dbg("header size: %d", read_reg(MFC_ENC_SI_STRM_SIZE));

	for (i = 0; i < read_reg(MFC_ENC_SI_STRM_SIZE); i++)
		mfc_dbg("0x%02x", (unsigned char)(*(enc_ctx->kstrmaddr + i)));
	*/

	return 0;
}

/*
 * [5] set_init_arg() implementations
 */
static int set_init_arg(struct mfc_inst_ctx *ctx, void *arg)
{
	struct mfc_enc_ctx *enc_ctx = (struct mfc_enc_ctx *)ctx->c_priv;
	struct mfc_enc_init_arg *init_arg = (struct mfc_enc_init_arg *)arg;

#ifdef CONFIG_VIDEO_MFC_VCM_UMP
	void *ump_handle;
#endif

	init_arg->cmn.out_header_size = read_reg(MFC_ENC_SI_STRM_SIZE);

#if defined(CONFIG_VIDEO_MFC_VCM_UMP)
	init_arg->cmn.out_u_addr.strm_ref_y = 0;
	ump_handle = mfc_get_buf_ump_handle(enc_ctx->streamaddr);

	mfc_dbg("secure id: 0x%08x", mfc_ump_get_id(ump_handle));

	if (ump_handle != NULL)
		init_arg->cmn.out_u_addr.strm_ref_y = mfc_ump_get_id(ump_handle);
	init_arg->cmn.out_u_addr.mv_ref_yc = 0;

#elif defined(CONFIG_S5P_VMEM)
	mfc_dbg("cookie: 0x%08x", s5p_getcookie((void *)(enc_ctx->streamaddr)));

	init_arg->cmn.out_u_addr.strm_ref_y = s5p_getcookie((void *)(enc_ctx->streamaddr));
	init_arg->cmn.out_u_addr.mv_ref_yc = 0;
#else
	init_arg->cmn.out_u_addr.strm_ref_y = mfc_mem_data_ofs(enc_ctx->streamaddr, 1);
	init_arg->cmn.out_u_addr.mv_ref_yc = 0;
#endif

	/*
	init_arg->cmn.out_buf_size.strm_ref_y = 0;
	init_arg->cmn.out_buf_size.mv_ref_yc = 0;

	init_arg->cmn.out_p_addr.strm_ref_y = 0;
	init_arg->cmn.out_p_addr.mv_ref_yc = 0;
	*/

	return 0;
}

/*
 * [6] set_codec_bufs() implementations
 */
static int set_codec_bufs(struct mfc_inst_ctx *ctx)
{
	return 0;
}

static int h264_set_codec_bufs(struct mfc_inst_ctx *ctx)
{
	struct mfc_alloc_buffer *alloc;

	alloc = _mfc_alloc_buf(ctx, MFC_ENC_UPMV_SIZE, ALIGN_2KB, MBT_CODEC | PORT_A);
	if (alloc == NULL) {
		mfc_err("failed alloc codec buffer\n");

		return -1;
	}
	write_reg(mfc_mem_base_ofs(alloc->real) >> 11, MFC_UP_MV_ADR);

	alloc = _mfc_alloc_buf(ctx, MFC_ENC_COLFLG_SIZE, ALIGN_2KB, MBT_CODEC | PORT_A);
	if (alloc == NULL) {
		mfc_err("failed alloc codec buffer\n");

		return -1;
	}
	write_reg(mfc_mem_base_ofs(alloc->real) >> 11, MFC_COLZERO_FLAG_ADR);

	alloc = _mfc_alloc_buf(ctx, MFC_ENC_INTRAMD_SIZE, ALIGN_2KB, MBT_CODEC | PORT_A);
	if (alloc == NULL) {
		mfc_err("failed alloc codec buffer\n");

		return -1;
	}
	write_reg(mfc_mem_base_ofs(alloc->real) >> 11, MFC_UP_INTRA_MD_ADR);

	alloc = _mfc_alloc_buf(ctx, MFC_ENC_INTRAPRED_SIZE, ALIGN_2KB, MBT_CODEC | PORT_B);
	if (alloc == NULL) {
		mfc_err("failed alloc codec buffer\n");

		return -1;
	}
	write_reg(mfc_mem_base_ofs(alloc->real) >> 11, MFC_UP_INTRA_PRED_ADR);

	alloc = _mfc_alloc_buf(ctx, MFC_ENC_NBORINFO_SIZE, ALIGN_2KB, MBT_CODEC | PORT_A);
	if (alloc == NULL) {
		mfc_err("failed alloc codec buffer\n");

		return -1;
	}
	write_reg(mfc_mem_base_ofs(alloc->real) >> 11, MFC_NBOR_INFO_ADR);

	return 0;
}

/*
 * [7] set_dpbs() implementations
 */
#if 0
static int set_dpbs(struct mfc_inst_ctx *ctx)
{
	return 0;
}
#endif

/*
 * [8] pre_frame_start() implementations
 */
static int pre_frame_start(struct mfc_inst_ctx *ctx)
{
	return 0;
}

/*
 * [9] post_frame_start() implementations
 */
static int post_frame_start(struct mfc_inst_ctx *ctx)
{
	return 0;
}

/*
 * [10] multi_frame_start() implementations
 */
static int multi_data_frame(struct mfc_inst_ctx *ctx)
{
	return 0;
}

/*
 * [11] set_exe_arg() implementations
 */
static int set_exe_arg(struct mfc_inst_ctx *ctx, void *arg)
{
	return 0;
}

/*
 * [12] get_codec_cfg() implementations
 */
static int get_codec_cfg(struct mfc_inst_ctx *ctx, unsigned int type, int *value)
{
	//struct mfc_enc_ctx *enc_ctx = (struct mfc_enc_ctx *)ctx->c_priv;
	int ret = 0;

	mfc_dbg("type: 0x%08x", type);

	/*
	MFC_ENC_GETCONF_FRAME_TAG	= ENC_GET,
	...
	*/

	switch (type) {

	default:
		mfc_err("invalid get config type: 0x%08x\n", type);
		ret = 1;

		break;
	}

	return ret;
}

/*
 * [13] set_codec_cfg() implementations
 */
static int set_codec_cfg(struct mfc_inst_ctx *ctx, unsigned int type, int *value)
{
	//struct mfc_enc_ctx *enc_ctx = (struct mfc_enc_ctx *)ctx->c_priv;
	int ret = 0;

	mfc_dbg("type: 0x%08x", type);

	/*
	MFC_ENC_SETCONF_FRAME_TYPE	= ENC_SET,
	MFC_ENC_SETCONF_CHANGE_FRAME_RATE,
	MFC_ENC_SETCONF_CHANGE_BIT_RATE,
	MFC_ENC_SETCONF_FRAME_TAG,
	MFC_ENC_SETCONF_ALLOW_FRAME_SKIP,
	MFC_ENC_SETCONF_VUI_INFO,
	MFC_ENC_SETCONF_I_PERIOD,
	MFC_ENC_SETCONF_HIER_P,
	...
	*/

	switch (type) {

	default:
		mfc_dbg("invalid set cfg type: 0x%08x\n", type);
		ret = 1;

		break;
	}

	return ret;
}

static struct mfc_enc_info unknown_enc = {
	.name		= "UNKNOWN",
	.codectype	= UNKNOWN_TYPE,
	.codecid	= -1,
	.e_priv_size	= 0,
	/*
	 * The unknown codec operations will be not call,
	 * unused default operations raise build warning.
	 */
	.c_ops		= {
		.alloc_ctx_buf		= alloc_ctx_buf,
		.alloc_desc_buf		= NULL,
		.get_init_arg		= get_init_arg,
		.pre_seq_start		= pre_seq_start,
		.post_seq_start		= post_seq_start,
		.set_init_arg		= set_init_arg,
		.set_codec_bufs		= set_codec_bufs,
		.set_dpbs		= NULL,
		.get_exe_arg		= NULL,
		.pre_frame_start	= pre_frame_start,
		.post_frame_start	= post_frame_start,
		.multi_data_frame	= multi_data_frame,
		.set_exe_arg		= set_exe_arg,
		.get_codec_cfg		= get_codec_cfg,
		.set_codec_cfg		= set_codec_cfg,
	},
};

static struct mfc_enc_info h264_enc = {
	.name		= "H264",
	.codectype	= H264_ENC,
	.codecid	= 16,
	.e_priv_size	= sizeof(struct mfc_enc_h264),
	.c_ops		= {
		.alloc_ctx_buf		= alloc_ctx_buf,
		.alloc_desc_buf		= NULL,
		.get_init_arg		= h264_get_init_arg,
		.pre_seq_start		= h264_pre_seq_start,
		.post_seq_start		= post_seq_start,
		.set_init_arg		= set_init_arg,
		.set_codec_bufs		= h264_set_codec_bufs,
		.set_dpbs		= NULL,
		.get_exe_arg		= NULL,
		.pre_frame_start	= pre_frame_start,
		.post_frame_start	= post_frame_start,
		.multi_data_frame	= multi_data_frame,
		.set_exe_arg		= set_exe_arg,
		.get_codec_cfg		= get_codec_cfg,
		.set_codec_cfg		= set_codec_cfg,
	},
};

static struct mfc_enc_info mpeg4_enc = {
	.name		= "MPEG4",
	.codectype	= MPEG4_ENC,
	.codecid	= 17,
	.e_priv_size	= 0,
	.c_ops		= {
		.alloc_ctx_buf		= alloc_ctx_buf,
		.alloc_desc_buf		= NULL,
		.get_init_arg		= mpeg4_get_init_arg,
		.pre_seq_start		= h264_pre_seq_start,
		.post_seq_start		= post_seq_start,
		.set_init_arg		= set_init_arg,
		.set_codec_bufs		= h264_set_codec_bufs,
		.set_dpbs		= NULL,
		.get_exe_arg		= NULL,
		.pre_frame_start	= pre_frame_start,
		.post_frame_start	= post_frame_start,
		.multi_data_frame	= multi_data_frame,
		.set_exe_arg		= set_exe_arg,
		.get_codec_cfg		= get_codec_cfg,
		.set_codec_cfg		= set_codec_cfg,
	},
};

static struct mfc_enc_info h263_enc = {
	.name		= "H263",
	.codectype	= H263_ENC,
	.codecid	= 18,
	.e_priv_size	= 0,
	.c_ops		= {
		.alloc_ctx_buf		= alloc_ctx_buf,
		.alloc_desc_buf		= NULL,
		.get_init_arg		= h263_get_init_arg,
		.pre_seq_start		= h264_pre_seq_start,
		.post_seq_start		= post_seq_start,
		.set_init_arg		= set_init_arg,
		.set_codec_bufs		= h264_set_codec_bufs,
		.set_dpbs		= NULL,
		.get_exe_arg		= NULL,
		.pre_frame_start	= pre_frame_start,
		.post_frame_start	= post_frame_start,
		.multi_data_frame	= multi_data_frame,
		.set_exe_arg		= set_exe_arg,
		.get_codec_cfg		= get_codec_cfg,
		.set_codec_cfg		= set_codec_cfg,
	},
};

void mfc_init_encoders(void)
{
	list_add_tail(&unknown_enc.list, &mfc_encoders);

	list_add_tail(&h264_enc.list, &mfc_encoders);
	list_add_tail(&mpeg4_enc.list, &mfc_encoders);
	list_add_tail(&h263_enc.list, &mfc_encoders);
}

static int mfc_set_encoder(struct mfc_inst_ctx *ctx, SSBSIP_MFC_CODEC_TYPE codectype)
{
	struct list_head *pos;
	struct mfc_enc_info *encoder;
	int codecid = -1;
	struct mfc_enc_ctx *enc_ctx;

	/* find and set codec private */
	list_for_each(pos, &mfc_encoders) {
		encoder = list_entry(pos, struct mfc_enc_info, list);

		if (encoder->codectype == codectype) {
			if (encoder->codecid < 0)
				break;

			ctx->type = ENCODER;
			ctx->c_ops = (struct codec_operations *)&encoder->c_ops;

			/* Allocate Encoder Context memory */
			enc_ctx = kzalloc(sizeof(struct mfc_enc_ctx), GFP_KERNEL);
			if (!enc_ctx) {
				mfc_err("failed to allocate codec private\n");
				return -ENOMEM;
			}
			ctx->c_priv = enc_ctx;

			/* Allocate Encoder context private memory */
			enc_ctx->e_priv = kzalloc(encoder->e_priv_size, GFP_KERNEL);
			if (!enc_ctx->e_priv) {
				mfc_err("failed to allocate encoder private\n");
				kfree(enc_ctx);
				ctx->c_priv = NULL;
				return -ENOMEM;
			}

			codecid = encoder->codecid;

			break;
		}
	}

	return codecid;
}

int set_strm_ref_buf(struct mfc_inst_ctx *ctx)
{
	struct mfc_alloc_buffer *alloc;
	int i;
	/*
	unsigned int reg;
	*/
	struct mfc_enc_ctx *enc_ctx = (struct mfc_enc_ctx *)ctx->c_priv;

	/* width: 128B align, height: 32B align, size: 8KB align */
	enc_ctx->lumasize = ALIGN(ctx->width, ALIGN_W) * ALIGN(ctx->height, ALIGN_H);
	enc_ctx->lumasize = ALIGN(enc_ctx->lumasize, ALIGN_8KB);
	enc_ctx->chromasize = ALIGN(ctx->width + 16, ALIGN_W) * ALIGN((ctx->height >> 1) + 4, ALIGN_H);
	enc_ctx->chromasize = ALIGN(enc_ctx->chromasize, ALIGN_8KB);

	/*
	 * allocate stream buffer
	 */
	alloc = _mfc_alloc_buf(ctx, MFC_STRM_SIZE, ALIGN_2KB, MBT_CPB | PORT_A);
	if (alloc == NULL) {
		mfc_err("failed alloc stream buffer\n");

		return -1;
	}
	enc_ctx->streamaddr = alloc->real;
	enc_ctx->streamsize = MFC_STRM_SIZE;

	/* FIXME: temp. */
	enc_ctx->kstrmaddr = alloc->addr;

	for (i = 0; i < 2; i++) {
		/*
		 * allocate y0, y1 ref buffer
		 */
		alloc = _mfc_alloc_buf(ctx, enc_ctx->lumasize, ALIGN_2KB, MBT_DPB | PORT_A);
		if (alloc == NULL) {
			mfc_err("failed alloc luma ref buffer\n");

			return -1;
		}
		/*
		 * set luma ref buffer address
		 */
		write_reg(mfc_mem_base_ofs(alloc->real) >> 11, MFC_ENC_REF0_LUMA_ADR + (4 * i));

		/*
		 * allocate y2, y3 ref buffer
		 */
		alloc = _mfc_alloc_buf(ctx, enc_ctx->lumasize, ALIGN_2KB, MBT_DPB | PORT_B);
		if (alloc == NULL) {
			mfc_err("failed alloc luma ref buffer\n");

			return -1;
		}
		/*
		 * set luma ref buffer address
		 */
		write_reg(mfc_mem_base_ofs(alloc->real) >> 11, MFC_ENC_REF2_LUMA_ADR + (4 * i));
	}

	/*
	 * allocate c0 ~ c3 ref buffer
	 */
	for (i = 0; i < 4; i++) {
		alloc = _mfc_alloc_buf(ctx, enc_ctx->chromasize, ALIGN_2KB, MBT_DPB | PORT_B);
		if (alloc == NULL) {
			mfc_err("failed alloc chroma ref buffer\n");

			return -1;
		}
		/*
		 * set chroma ref buffer address
		 */
		write_reg(mfc_mem_base_ofs(alloc->real) >> 11, MFC_ENC_REF0_CHROMA_ADR + (4 * i));
	}

	return 0;
}

int mfc_init_encoding(struct mfc_inst_ctx *ctx, union mfc_args *args)
{
	struct mfc_enc_init_arg *init_arg = (struct mfc_enc_init_arg *)args;
	struct mfc_enc_ctx *enc_ctx = NULL;
	struct mfc_pre_cfg *precfg;
	struct list_head *pos, *nxt;
	int ret;
	unsigned char *in_vir;


	ctx->codecid = mfc_set_encoder(ctx, init_arg->cmn.in_codec_type);
	if (ctx->codecid < 0) {
		mfc_err("unsupported encoding codec: %d", init_arg->cmn.in_codec_type);
		ret = MFC_ENC_INIT_FAIL;
		goto err_handling;
	}

	ctx->width = init_arg->cmn.in_width;
	ctx->height = init_arg->cmn.in_height;

	enc_ctx = (struct mfc_enc_ctx *)ctx->c_priv;

	enc_ctx->pixelcache = init_arg->cmn.in_pixelcache;

	/*
	 * assign pre configuration values to instance context
	 */
	list_for_each_safe(pos, nxt, &ctx->presetcfgs) {
		precfg = list_entry(pos, struct mfc_pre_cfg, list);

		if (ctx->c_ops->set_codec_cfg) {
			ret = ctx->c_ops->set_codec_cfg(ctx, precfg->type, precfg->value);
			if (ret < 0)
				mfc_warn("cannot set preset config type: 0x%08x: %d",
					precfg->type, ret);
		}

		list_del(&precfg->list);
		kfree(precfg);
	}

	/*
	 * allocate context buffer
	 */
	if (ctx->c_ops->alloc_ctx_buf) {
		if (ctx->c_ops->alloc_ctx_buf(ctx) < 0) {
			mfc_err("Context buffer allocation Failed");
			ret = MFC_ENC_INIT_FAIL;
			goto err_handling;
		}
	}

	/* [pixelcache] */
	ret = mfc_cmd_inst_open(ctx);
	if (ret < 0) {
		mfc_err("Open Instance Failed");
		goto err_handling;
	}

	mfc_set_inst_state(ctx, INST_STATE_OPEN);

	if (init_shm(ctx) < 0) {
		mfc_err("Shared Memory Initialization Failed");
		ret = MFC_ENC_INIT_FAIL;
		goto err_handling;
	}

	/*
	 * get init. argumnets
	 */
	if (ctx->c_ops->get_init_arg) {
		if (ctx->c_ops->get_init_arg(ctx, (void *)init_arg) < 0) {
			mfc_err("Get Initial Arguments Failed");
			ret = MFC_ENC_INIT_FAIL;
			goto err_handling;
		}
	}

	/*
	 * allocate & set codec buffers
	 */
	if (ctx->c_ops->set_codec_bufs) {
		if (ctx->c_ops->set_codec_bufs(ctx) < 0) {
			mfc_err("Set Codec Buffers Failed");
			ret = MFC_ENC_INIT_FAIL;
			goto err_handling;
		}
	}

	set_strm_ref_buf(ctx);

	/*
	 * execute pre sequence start operation
	 */
	if (ctx->c_ops->pre_seq_start) {
		if (ctx->c_ops->pre_seq_start(ctx) < 0) {
			mfc_err("Pre-Sequence Start Faild");
			ret = MFC_ENC_INIT_FAIL;
			goto err_handling;
		}
	}

	ret = mfc_cmd_seq_start(ctx);
	if (ret < 0) {
		mfc_err("Sequence Start Failed");
		goto err_handling;
	}

	if (ctx->c_ops->post_seq_start) {
		if (ctx->c_ops->post_seq_start(ctx) < 0) {
			mfc_err("Post Sequence Start Failed");
			ret = MFC_ENC_INIT_FAIL;
			goto err_handling;
		}
	}

	if (ctx->c_ops->set_init_arg) {
		if (ctx->c_ops->set_init_arg(ctx, (void *)init_arg) < 0) {
			mfc_err("Setting Initialized Arguments Failed");
			ret = MFC_ENC_INIT_FAIL;
			goto err_handling;
		}
	}

	if(ctx->buf_cache_type == CACHE){
		in_vir = phys_to_virt(enc_ctx->streamaddr);
		mfc_mem_cache_inv(in_vir, init_arg->cmn.out_header_size);
		mfc_dbg("cache invalidate\n");
	}

#ifdef CONFIG_CPU_FREQ
	/* Fix MFC & Bus Frequency for High resolution for better performance */
	if (ctx->width >= 1920 || ctx->height >= 1080){
		if (atomic_read(&ctx->dev->busfreq_lock_cnt) == 0){
			/* For fixed MFC & Bus Freq to 200 & 400 MHz for 1080p Contents */
			s5pv310_busfreq_lock(DVFS_LOCK_ID_MFC,BUS_L0);
			mfc_dbg("[%s] Bus Freq Locked L0  \n",__func__);
		}
		atomic_inc(&ctx->dev->busfreq_lock_cnt);
		ctx->busfreq_flag = true;
	}
#endif

	/*
	 * allocate & set DPBs
	 */
	/*
	if (ctx->c_ops->set_dpbs) {
		if (ctx->c_ops->set_dpbs(ctx) < 0)
			return MFC_ENC_INIT_FAIL;
	}
	*/

	/*
	ret = mfc_cmd_init_buffers(ctx);
	if (ret < 0)
		return ret;
	*/

	mfc_set_inst_state(ctx, INST_STATE_INIT);

	mfc_print_buf();

	return MFC_OK;

err_handling:
	if (ctx->state > INST_STATE_CREATE) {
		mfc_cmd_inst_close(ctx);
		ctx->state = INST_STATE_CREATE;
	}

	mfc_free_buf_inst(ctx->id);

	if (!enc_ctx) {
		kfree(enc_ctx->e_priv);
		enc_ctx->e_priv = NULL;
	}

	if (!ctx->c_priv) {
		kfree(ctx->c_priv);
		ctx->c_priv = NULL;
	}

	return ret;
}

static int mfc_encoding_frame(struct mfc_inst_ctx *ctx, struct mfc_enc_exe_arg *exe_arg)
{
	struct mfc_enc_ctx *enc_ctx = (struct mfc_enc_ctx *)ctx->c_priv;
	int ret;
#ifdef CONFIG_VIDEO_MFC_VCM_UMP
	void *ump_handle;
#endif


	/* Set Frame Tag */
	write_shm(ctx, exe_arg->in_frametag, SET_FRAME_TAG);

	/* Set stream buffer addr */
	write_reg(mfc_mem_base_ofs(enc_ctx->streamaddr) >> 11, MFC_ENC_SI_CH1_SB_ADR);
	write_reg(enc_ctx->streamsize, MFC_ENC_SI_CH1_SB_SIZE);

	#if 0
	/* force I frame or Not-coded frame */
	if (mfc_ctx->forceSetFrameType == I_FRAME)
		write_reg(1, MFC_ENC_SI_CH1_FRAME_INS);
	else if (mfc_ctx->forceSetFrameType == NOT_CODED)
		write_reg(1 << 1, MFC_ENC_SI_CH1_FRAME_INS);

	if (mfc_ctx->dynamic_framerate != 0) {
		write_shm((1 << 1), mfc_ctx->shared_mem_vir_addr + 0x2c);
		/* MFC fw 2010/04/09 */
		write_shm(mfc_ctx->dynamic_framerate*SCALE_NUM,
				  mfc_ctx->shared_mem_vir_addr + 0x94);
		if (mfc_ctx->MfcCodecType == MPEG4_ENC) {
			time_increment_res = mfc_ctx->dynamic_framerate *
						MPEG4_TIME_RES;
			write_shm((1 << 31) |
				  (time_increment_res << 16) |
				  (MPEG4_TIME_RES),
				  mfc_ctx->shared_mem_vir_addr + 0x30);
		}
	}

	if (mfc_ctx->dynamic_bitrate != 0) {
		write_shm((1 << 2), mfc_ctx->shared_mem_vir_addr + 0x2c);
		write_shm(mfc_ctx->dynamic_bitrate,
				  mfc_ctx->shared_mem_vir_addr + 0x90);
	}
	#endif

	/* Set current frame buffer addr */
	#if 1
	/* RainAde : to access FIMC CMA region for zero copy solution */
	write_reg((exe_arg->in_Y_addr - mfc_mem_base(1))>> 11, MFC_ENC_SI_CH1_CUR_Y_ADR);
	write_reg((exe_arg->in_CbCr_addr - mfc_mem_base(1)) >> 11, MFC_ENC_SI_CH1_CUR_C_ADR);
	#else
	write_reg(mfc_mem_base_ofs(exe_arg->in_Y_addr) >> 11, MFC_ENC_SI_CH1_CUR_Y_ADR);
	write_reg(mfc_mem_base_ofs(exe_arg->in_CbCr_addr) >> 11, MFC_ENC_SI_CH1_CUR_C_ADR);
	#endif

	#if 0
	write_reg(1, MFC_ENC_STR_BF_U_EMPTY);
	write_reg(1, MFC_ENC_STR_BF_L_EMPTY);

	/* buf reset command if stream buffer is frame mode */
	write_reg(0x1 << 1, MFC_ENC_SF_BUF_CTRL);
	#endif

	if(ctx->buf_cache_type == CACHE){
		flush_all_cpu_caches();
		outer_flush_all();
	}

	ret = mfc_cmd_frame_start(ctx);
	if (ret < 0)
		return ret;

	exe_arg->out_frame_type = read_reg(MFC_ENC_SI_SLICE_TYPE);
	exe_arg->out_encoded_size = read_reg(MFC_ENC_SI_STRM_SIZE);
	/* Get Frame Tag top and bottom */
	exe_arg->out_frametag_top = read_shm(ctx, GET_FRAME_TAG_TOP);
	exe_arg->out_frametag_bottom = read_shm(ctx, GET_FRAME_TAG_BOT);


	/* MFC fw 9/30 */
	/*
	enc_arg->out_Y_addr =
	    cur_frm_base + (read_reg(MFC_ENCODED_Y_ADDR) << 11);
	enc_arg->out_CbCr_addr =
	    cur_frm_base + (read_reg(MFC_ENCODED_C_ADDR) << 11);
	*/

	/* FIXME: port must be checked */
	exe_arg->out_Y_addr = mfc_mem_addr_ofs(read_reg(MFC_ENCODED_Y_ADDR) << 11, 1);
	exe_arg->out_CbCr_addr = mfc_mem_addr_ofs(read_reg(MFC_ENCODED_C_ADDR) << 11, 1);

	/* FIXME: cookie may be invalide */
#if defined(CONFIG_VIDEO_MFC_VCM_UMP)
	exe_arg->out_y_secure_id = 0;
	exe_arg->out_c_secure_id = 0;

	ump_handle = mfc_get_buf_ump_handle(read_reg(MFC_ENCODED_Y_ADDR) << 11);
	if (ump_handle != NULL)
		exe_arg->out_y_secure_id = mfc_ump_get_id(ump_handle);

	ump_handle = mfc_get_buf_ump_handle(read_reg(MFC_ENCODED_C_ADDR) << 11);
	if (ump_handle != NULL)
		exe_arg->out_c_secure_id = mfc_ump_get_id(ump_handle);

	mfc_dbg("secure IDs Y: 0x%08x, C:0x%08x\n", exe_arg->out_y_secure_id,
		exe_arg->out_c_secure_id);
#elif defined(CONFIG_S5P_VMEM)
	exe_arg->out_y_cookie = s5p_getcookie((void *)(read_reg(MFC_ENCODED_Y_ADDR) << 11));
	exe_arg->out_c_cookie = s5p_getcookie((void *)(read_reg(MFC_ENCODED_C_ADDR) << 11));

	mfc_dbg("cookie Y: 0x%08x, C:0x%08x\n", exe_arg->out_y_cookie,
		exe_arg->out_c_cookie);
#endif

	#if 0
	write_reg(0, MFC_ENC_SI_CH1_FRAME_INS);
	mfc_ctx->forceSetFrameType = 0;

	write_shm(0, mfc_ctx->shared_mem_vir_addr + 0x2c);
	mfc_ctx->dynamic_framerate = 0;
	mfc_ctx->dynamic_bitrate = 0;
	#endif

	mfc_dbg
	    ("- frame type(%d) encoded frame size(%d) encoded Y_addr(0x%08x) / C_addr(0x%08x)\r\n",
	     exe_arg->out_frame_type, exe_arg->out_encoded_size,
	     exe_arg->out_Y_addr, exe_arg->out_CbCr_addr);

	return MFC_OK;
}

int mfc_exec_encoding(struct mfc_inst_ctx *ctx, union mfc_args *args)
{
	struct mfc_enc_exe_arg *exe_arg;
	int ret;
	/*
	struct mfc_enc_ctx *enc_ctx = (struct mfc_enc_ctx *)ctx->c_priv;
	*/

	exe_arg = (struct mfc_enc_exe_arg *)args;

	mfc_set_inst_state(ctx, INST_STATE_EXE);

	ret = mfc_encoding_frame(ctx, exe_arg);

	mfc_set_inst_state(ctx, INST_STATE_EXE_DONE);

	return ret;
}

