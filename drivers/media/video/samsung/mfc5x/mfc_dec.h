/*
 * linux/drivers/media/video/samsung/mfc5x/mfc_dec.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Decoder interface for Samsung MFC (Multi Function Codec - FIMV) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __MFC_DEC_H
#define __MFC_DEC_H __FILE__

#include <linux/list.h>

#include "mfc.h"
#include "mfc_interface.h"
#include "mfc_inst.h"

/* display status */
/* resolution change */
#define RC_MASK		0x3
#define RC_SHIFT	4
#define RC_NO		0
#define RC_INC		1
#define RC_DEC		2

/* progressive/interface */
#define PI_MASK		0x1
#define PI_SHIFT	3
#define PI_PROGRESSIVE	0
#define PI_INTERFACE	1

#define DISP_S_MASK	0x3	/* FIXME: 0x7 */
enum disp_status {
	DISP_S_DECODING	= 0,
	DISP_S_DD	= 1,
	DISP_S_DISPLAY	= 2,
	DISP_S_FINISH	= 3,
	/* RES_CHANGE is not the value of register bit */
	DISP_S_RES_CHANGE = 4,
};

/* decoding status */
/* CRC */
#define CRC_G_MASK	0x1
#define CRC_G_SHIFT	5

#define CRC_N_MASK	0x1
#define CRC_N_SHIFT	4
#define CRC_TWO		0
#define CRC_FOUR	1

#define DEC_S_MASK	0x7
enum dec_status {
	DEC_S_DECODING	= 0,
	DEC_S_DD	= 1,
	DEC_S_DISPLAY	= 2,
	DEC_S_FINISH	= 3,
	DEC_S_NO	= 4,
};

#define DEC_FRM_MASK	0x7
enum dec_frame {
	DEC_FRM_N	= 0,
	DEC_FRM_I	= 1,
	DEC_FRM_P	= 2,
	DEC_FRM_B	= 3,
	DEC_FRM_OTHER	= 4,
};

#define DISP_FRM_MASK	0x3
enum disp_frame {
	DISP_FRM_N = 0,
	DISP_FRM_I = 1,
	DISP_FRM_P = 2,
	DISP_FRM_B = 3,
};

enum dec_pc {
	DPC_ONLY_P	= 0,
	DPC_ONLY_B	= 1,
	DPC_BOTH_P_B	= 2,
	DPC_DISABLE	= 3,
};

struct mfc_dec_ctx {
	unsigned int lumasize;		/* C */
	unsigned int chromasize;	/* C */

	/* init */
	unsigned int crc;		/* I */
	enum dec_pc pixelcache;		/* I */
	unsigned int slice;		/* I */

	unsigned int numextradpb;	/* I */
	unsigned int nummindpb;		/* H */
	unsigned int numtotaldpb;	/* C */

	unsigned int level;		/* H */
	unsigned int profile;		/* H */

	/* init | exec */
	unsigned long streamaddr;	/* I */
	unsigned int streamsize;	/* I */
	unsigned int frametag;	/* I */

	/* exec */
	unsigned int consumed;		/* H */
	int predisplay_Yaddr;		/* H */
	int predisplay_Caddr;		/* H */

	enum dec_frame frametype;	/* H */

	enum disp_status dispstatus;	/* H */
	enum dec_status decstatus;	/* H */

	unsigned int lastframe;		/* I */

	unsigned int dpbflush;		/* I */
	/* etc */
	unsigned int immediatelydisplay;

	/* init | exec */
	unsigned int ispackedpb;	/* I */

	void *d_priv;
};

/* decoder private data */
struct mfc_dec_h264 {
	/* init */
	unsigned int mvsize;		/* C */

	unsigned int dispdelay_en;	/* I */
	unsigned int dispdelay_val;	/* I */

	/* init | exec */
	unsigned int crop_r_ofs;	/* H */
	unsigned int crop_l_ofs;	/* H */
	unsigned int crop_b_ofs;	/* H */
	unsigned int crop_t_ofs;	/* H */
};

struct mfc_dec_mpeg4 {
	/* init */
	unsigned int postfilter;	/* I */

	unsigned int aspect_ratio;	/* H */
	unsigned int ext_par_width;	/* H */
	unsigned int ext_par_height;	/* H */

	/* init | exec */
	unsigned int packedpb;		/* I */
};

struct mfc_dec_fimv1 {
	/* init */
	unsigned int postfilter;	/* I */

	unsigned int aspect_ratio;	/* H */
	unsigned int ext_par_width;	/* H */
	unsigned int ext_par_height;	/* H */

	unsigned int width;		/* I */
	unsigned int height;		/* I */

	/* init | exec */
	unsigned int packedpb;		/* I */
};

int mfc_init_decoding(struct mfc_inst_ctx *ctx, union mfc_args *args);
/*
int mfc_init_decoding(struct mfc_inst_ctx *ctx, struct mfc_dec_init_arg *init_arg);
*/
int mfc_exec_decoding(struct mfc_inst_ctx *ctx, union mfc_args *args);
/*
int mfc_exec_decoding(struct mfc_inst_ctx *ctx, struct mfc_dec_exe_arg *exe_arg);
*/

/*---------------------------------------------------------------------------*/

struct mfc_dec_info {
	struct list_head list;
	const char *name;
	SSBSIP_MFC_CODEC_TYPE codectype;
	int codecid;
	unsigned int d_priv_size;

	const struct codec_operations c_ops;
};

void mfc_init_decoders(void);

#endif /* __MFC_CMD_H */
