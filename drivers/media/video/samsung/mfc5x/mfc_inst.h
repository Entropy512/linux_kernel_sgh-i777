/*
 * linux/drivers/media/video/samsung/mfc5x/mfc_inst.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Instance manager file for Samsung MFC (Multi Function Codec - FIMV) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __MFC_INST_H
#define __MFC_INST_H __FILE__

#include <linux/list.h>

#include "mfc.h"
#include "mfc_interface.h"


/* FIXME: instance state should be more specific */
enum instance_state {
	INST_STATE_NULL		= 0,

	/* open */
	INST_STATE_CREATE	= 0x0001,

	/* ioctl - *_INIT */
	INST_STATE_OPEN		= 0x0010,
	INST_STATE_INIT,

	/* ioctl - *_EXE */
	INST_STATE_EXE		= 0x0020,
	INST_STATE_EXE_DONE,
};

struct mfc_inst_ctx;

struct codec_operations {
	/* initialization routines */
	int (*alloc_ctx_buf) (struct mfc_inst_ctx *ctx);
	int (*alloc_desc_buf) (struct mfc_inst_ctx *ctx);
	int (*get_init_arg) (struct mfc_inst_ctx *ctx, void *arg);
	int (*pre_seq_start) (struct mfc_inst_ctx *ctx);
	int (*post_seq_start) (struct mfc_inst_ctx *ctx);
	int (*set_init_arg) (struct mfc_inst_ctx *ctx, void *arg);
	int (*set_codec_bufs) (struct mfc_inst_ctx *ctx);
	int (*set_dpbs) (struct mfc_inst_ctx *ctx);		/* decoder */
	/* execution routines */
	int (*get_exe_arg) (struct mfc_inst_ctx *ctx, void *arg);
	int (*pre_frame_start) (struct mfc_inst_ctx *ctx);
	int (*post_frame_start) (struct mfc_inst_ctx *ctx);
	int (*multi_data_frame) (struct mfc_inst_ctx *ctx);
	int (*set_exe_arg) (struct mfc_inst_ctx *ctx, void *arg);
	/* configuration routines */
	int (*get_codec_cfg) (struct mfc_inst_ctx *ctx, unsigned int type, int *value);
	int (*set_codec_cfg) (struct mfc_inst_ctx *ctx, unsigned int type, int *value);
};

struct mfc_pre_cfg {
	struct list_head list;
	unsigned int type;
	unsigned int value[4];
};

struct mfc_dec_cfg {
	unsigned int crc;
	unsigned int pixelcache;
	unsigned int slice;
	unsigned int numextradpb;

	unsigned int postfilter;	/* MPEG4 */
	unsigned int dispdelay_en;	/* H.264 */
	unsigned int dispdelay_val;	/* H.264 */
	unsigned int width;		/* FIMV1 */
	unsigned int height;		/* FIMV1 */
};

struct mfc_enc_cfg {
	/*
	type:
	  init
	  runtime
	  init + runtime
	*/

	/* init */
	unsigned int pixelcache;

	unsigned int frameskip;
	unsigned int frammode;
	unsigned int hier_p;

	/* runtime ? */
	#if 0
	unsigned int frametype;
	unsigned int fraemrate;
	unsigned int bitrate;
	unsigned int vui;		/* H.264 */
	unsigned int hec;		/* MPEG4 */
	unsigned int seqhdrctrl;

	unsigned int i_period;
	#endif

	/* Remove before Release */
	#if 0
	// common parameter
	int codecStd;
	int SourceWidth;
	int SourceHeight;
	int IDRPeriod;                      // [IN] GOP number(interval of I-frame)
	int SliceMode;                      // [IN] Multi slice mode[0:disable 1:enable]
	int RandomIntraMBRefresh;           // [IN] cyclic intra refresh[0 ~ MBCnt]
	int EnableFRMRateControl;           // [IN] frame based rate control enable[0:disable 1:enable]
	int Bitrate;                        // [IN] rate control parameter(bit rate) [1 ~ ]
	int FrameQp;                        // [IN] The quantization parameter of the frame[1 ~ 31]
	int FrameQp_P;                      // [IN] The quantization parameter of the P frame
	int QSCodeMax;                      // [IN] Maximum Quantization value[1 ~ 31]
	int QSCodeMin;                      // [IN] Minimum Quantization value[1 ~ 31]
	int CBRPeriodRf;                    // [IN] Reaction coefficient parameter for rate control[CBR:2 ~ 10, VBR:100~1000]
	int PadControlOn;                   // [IN] Enable padding control
	int LumaPadVal;                     // [IN] Luma pel value used to fill padding area [0 ~ 255]
	int CbPadVal;                       // [IN] CB pel value used to fill padding area [0 ~ 255]
	int CrPadVal;                       // [IN] CR pel value used to fill padding area [0 ~ 255]

	// MPEG4, H.264 common
	int ProfileIDC;                     // [IN] profile [0:SP 1:ASP]
	int LevelIDC;                       // [IN] level[0(level 0) ~ 50(level 5.0)]
	int FrameQp_B;                      // [IN] The quantization parameter of the B frame
	int SliceArgument;                  // [IN] MB number or byte number [MB number: 1 ~ (MBCnt-1), Byte number: 1900 ~ ]
	int NumberBFrames;                  // [IN] The number of consecutive B frame inserted[0:Use boundary pixel, 1:Use the below setting value]

	// H.264, H263 specific
	int FrameRate;                      // [IN] rate control parameter(frame rate)

	// MPEG4 specific
	int TimeIncreamentRes;              // [IN] frame rate
	int VopTimeIncreament;              // [IN] frame rate
	int DisableQpelME;                  // [IN] disable quarter-pixel motion estimation [0:enable 1:disable]

	// H.264 specific
	int NumberReferenceFrames;          // [IN] The number of reference pictures used  [0:enable, 1:disable]
	int NumberRefForPframes;            // [IN] The number of reference pictures used for encoding P pictures [0:enable, 1:disable]
	int LoopFilterDisable;              // [IN] disable the loop filter [0:enable, 1:disable]
	int LoopFilterAlphaC0Offset;        // [IN] Alpha & C0 offset for H.264 loop filter [-6 ~ +6]
	int LoopFilterBetaOffset;           // [IN] Beta offset for H.264 loop filter [-6 ~ +6]
	int SymbolMode;                     // [IN] The mode of entropy coding(CABAC, CAVLC)
	int PictureInterlace;               // [IN] Enables the interlace mode  [0:frame picture, 1:field picture]
	int Transform8x8Mode;               // [IN] Allow 8x8 transform(This is allowed only for high profile) [0:disable, 1:enable]
	int EnableMBRateControl;            // [IN] Enable macroblock-level rate control [0:disable, 1:enable]
	int DarkDisable;                    // [IN] Disable adaptive rate control on dark region [0:enable, 1:disable]
	int SmoothDisable;                  // [IN] Disable adaptive rate control on smooth region [0:enable, 1:disable]
	int StaticDisable;                  // [IN] Disable adaptive rate control on static region [0:enable, 1:disable]
	int ActivityDisable;                // [IN] Disable adaptive rate control on high activity region [0:enable, 1:disable]
	#endif
};

enum mfc_resolution_status {
	RES_INCREASED = 1,
	RES_DECERASED = 2,
};

enum mfc_resolution_change_status {
	RES_NO_CHANGE = 0,
	RES_SET_CHANGE = 1,
	RES_SET_REALLOC = 2,
	RES_WAIT_FRAME_DONE = 3,
};

struct mfc_inst_ctx {
	int id;				/* assigned by driver */
	int cmd_id;			/* assigned by F/W */
	int codecid;
	unsigned int type;
	enum instance_state state;
	unsigned int width;
	unsigned int height;
	volatile unsigned char *shm;
	unsigned int shmofs;
	unsigned int ctxbufofs;
	unsigned int ctxbufsize;
	unsigned int descbufofs;	/* FIXME: move to decoder context */
	unsigned int descbufsize;	/* FIXME: move to decoder context */
	unsigned long userbase;
	SSBIP_MFC_BUFFER_TYPE buf_cache_type;

	int resolution_status;
	/*
	struct mfc_dec_cfg deccfg;
	struct mfc_enc_cfg enccfg;
	*/
	struct list_head presetcfgs;

	void *c_priv;
	struct codec_operations *c_ops;
	struct mfc_dev *dev;
#ifdef SYSMMU_MFC_ON
	unsigned long pgd;
#endif
#ifdef CONFIG_CPU_FREQ
	int busfreq_flag; /* context bus frequency flag*/
#endif
};

struct mfc_inst_ctx *mfc_create_inst(void);
void mfc_destroy_inst(struct mfc_inst_ctx* ctx);
int mfc_set_inst_state(struct mfc_inst_ctx *ctx, enum instance_state state);
int mfc_chk_inst_state(struct mfc_inst_ctx *ctx, enum instance_state state);
int mfc_set_inst_cfg(struct mfc_inst_ctx *ctx, unsigned int type, int *value);

#endif /* __MFC_INST_H */
