/*
 * linux/drivers/media/video/samsung/mfc5x/mfc_enc.h
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

#ifndef __MFC_ENC_H
#define __MFC_ENC_H __FILE__

#include <linux/list.h>

#include "mfc.h"
#include "mfc_interface.h"
#include "mfc_inst.h"

enum enc_pc {
	EPC_ENABLE	= 0,
	EPC_DISABLE	= 3,
};

struct mfc_enc_ctx {
	unsigned int lumasize;		/* C */
	unsigned int chromasize;	/* C */

	unsigned long streamaddr;	/* K */
	unsigned int streamsize;	/* K */

	/* FIXME: temp. */
	unsigned char *kstrmaddr;

	/* init */
	enum enc_pc pixelcache;

	/* init | exec */
	unsigned int framemap;

	/* exec */
	unsigned int interlace;
	unsigned int forceframe;
	unsigned int frameskip;
	unsigned int framerate;
	unsigned int bitrate;

	void *e_priv;
};

struct mfc_enc_h264 {
	unsigned int vui_enable;
	unsigned int hier_p_enable;

	unsigned int i_period;
};

int mfc_init_encoding(struct mfc_inst_ctx *ctx, union mfc_args *args);
/*
int mfc_init_encoding(struct mfc_inst_ctx *ctx, struct mfc_dec_init_arg *init_arg);
*/
int mfc_exec_encoding(struct mfc_inst_ctx *ctx, union mfc_args *args);
/*
int mfc_exec_encoding(struct mfc_inst_ctx *ctx, struct mfc_dec_exe_arg *exe_arg);
*/

/*---------------------------------------------------------------------------*/

struct mfc_enc_info {
	struct list_head list;
	const char *name;
	SSBSIP_MFC_CODEC_TYPE codectype;
	int codecid;
	unsigned int e_priv_size;

	const struct codec_operations c_ops;
};

void mfc_init_encoders(void);

#endif /* __MFC_ENC_H */
