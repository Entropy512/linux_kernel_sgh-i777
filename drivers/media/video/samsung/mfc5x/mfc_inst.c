/*
 * linux/drivers/media/video/samsung/mfc5x/mfc_inst.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Instance manager for Samsung MFC (Multi Function Codec - FIMV) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/slab.h>
#include <linux/mm.h>

#include "mfc_inst.h"
#include "mfc_log.h"
#include "mfc_buf.h"
#include "mfc_cmd.h"
#include "mfc_pm.h"
#include "mfc_dec.h"
#include "mfc_enc.h"

#ifdef SYSMMU_MFC_ON
#include <linux/interrupt.h>
#endif

/*
 * the sematic both of mfc_create_inst() and mfc_destory_inst()
 * be symmetric, but MFC channel open operation will be execute
 * while init. sequence. (decoding and encoding)
 * create -	just allocate context memory and initialize state
 *
 * destory -	execute channel close operation
 *		free allocated buffer for instance
 *		free allocated context memory
 */

struct mfc_inst_ctx *mfc_create_inst(void)
{
	struct mfc_inst_ctx *ctx;

	ctx = kzalloc(sizeof(struct mfc_inst_ctx), GFP_KERNEL);
	if (!ctx) {
		mfc_err("failed to create instance\n");
		return NULL;
	}

	/* FIXME: set default values */
	ctx->state = INST_STATE_CREATE;

	ctx->resolution_status = RES_NO_CHANGE;
#ifdef CONFIG_CPU_FREQ
	ctx->busfreq_flag = false;
#endif

#ifdef SYSMMU_MFC_ON
	/*
	ctx->pgd = __pa(current->mm->pgd);
	*/
	ctx->pgd = __pa(swapper_pg_dir);
#endif

	INIT_LIST_HEAD(&ctx->presetcfgs);

	return ctx;
}

void mfc_destroy_inst(struct mfc_inst_ctx* ctx)
{
	int ret = 0;
	struct mfc_dec_ctx *dec_ctx;
	struct mfc_enc_ctx *enc_ctx;

	if (ctx) {
		if (ctx->state > INST_STATE_CREATE) {
			mfc_clock_on();
			/* FIXME: meaningless return value */
			ret = mfc_cmd_inst_close(ctx);
			mfc_clock_off();
		}

		mfc_free_buf_inst(ctx->id);

		/* Free Decoder/Encoder context private memory */
		if (ctx->type == DECODER) {
			dec_ctx = ctx->c_priv;
			if (!dec_ctx)
				kfree(dec_ctx->d_priv);
		} else if (ctx->type == ENCODER) {
			enc_ctx = ctx->c_priv;
			if (!enc_ctx)
				kfree(enc_ctx->e_priv);
		} else {
			mfc_err("mfc_destroy_inst(): wrong codec type");
		}

		/* Free Decoder/Encoder Context */
		if (!ctx->c_priv)
			kfree(ctx->c_priv);

		/* Free instance context memory */
		kfree(ctx);
	}
}

int mfc_set_inst_state(struct mfc_inst_ctx *ctx, enum instance_state state)
{
	mfc_dbg("state: 0x%08x", state);

	if (ctx->state > state)
		return -1;

	ctx->state = state;

	return 0;
}

int mfc_chk_inst_state(struct mfc_inst_ctx *ctx, enum instance_state state)
{
	if (ctx->state != state)
		return -1;
	else
		return 0;
}

int mfc_set_inst_cfg(struct mfc_inst_ctx *ctx, unsigned int type, int *value)
{
	int ret = MFC_OK;
	struct mfc_pre_cfg *precfg;

	/*
	struct list_head *pos, *nxt;
	*/

	mfc_dbg("type: 0x%08x, ctx->type: 0x%08x", type, ctx->type);

	if (ctx->state <= INST_STATE_CREATE) {
		precfg = (struct mfc_pre_cfg *)
			kzalloc(sizeof(struct mfc_pre_cfg), GFP_KERNEL);

		if (unlikely(precfg == NULL)) {
			mfc_err("no more kernel memory");

			return MFC_GET_CONF_FAIL;
		}

		memset(precfg, 0x00, sizeof(precfg));

		precfg->type = type;
		memcpy(precfg->value, value, sizeof(precfg->value));

		/*
		mfc_dbg("type: 0x%08x, precfg->type: 0x%08x", type, precfg->type);
		mfc_dbg("value1: %d, precfg->value1: %d", value[0], precfg->value[0]);
		mfc_dbg("value2: %d, precfg->value2: %d", value[1], precfg->value[1]);
		mfc_dbg("value3: %d, precfg->value3: %d", value[2], precfg->value[2]);
		mfc_dbg("value4: %d, precfg->value4: %d", value[3], precfg->value[3]);
		*/

		list_add_tail(&precfg->list, &ctx->presetcfgs);

		/*
		mfc_dbg("precfg enties...");
		precfg = NULL;

		list_for_each_safe(pos, nxt, &ctx->presetcfgs) {
			precfg = list_entry(pos, struct mfc_pre_cfg, list);

			mfc_dbg("type: 0x%08x, precfg->type: 0x%08x", type, precfg->type);
			mfc_dbg("value1: %d, precfg->value1: %d", value[0], precfg->value[0]);
			mfc_dbg("value2: %d, precfg->value2: %d", value[1], precfg->value[1]);
			mfc_dbg("value3: %d, precfg->value3: %d", value[2], precfg->value[2]);
			mfc_dbg("value4: %d, precfg->value4: %d", value[3], precfg->value[3]);
		}
		*/

		return MFC_OK;
	}

	switch(type){
                case MFC_DEC_SETCONF_POST_ENABLE:
                case MFC_DEC_SETCONF_EXTRA_BUFFER_NUM:
                case MFC_DEC_SETCONF_DISPLAY_DELAY:
                case MFC_DEC_SETCONF_IS_LAST_FRAME:
                case MFC_DEC_SETCONF_SLICE_ENABLE:
                case MFC_DEC_SETCONF_CRC_ENABLE:
                case MFC_DEC_SETCONF_FIMV1_WIDTH_HEIGHT:
                case MFC_DEC_SETCONF_FRAME_TAG:
                case MFC_DEC_SETCONF_IMMEDIATELY_DISPLAY:
                case MFC_DEC_SETCONF_DPB_FLUSH:
                case MFC_DEC_SETCONF_PIXEL_CACHE:
                case MFC_ENC_SETCONF_FRAME_TYPE:
                case MFC_ENC_SETCONF_CHANGE_FRAME_RATE:
                case MFC_ENC_SETCONF_CHANGE_BIT_RATE:
                case MFC_ENC_SETCONF_FRAME_TAG:
                case MFC_ENC_SETCONF_ALLOW_FRAME_SKIP:
                case MFC_ENC_SETCONF_VUI_INFO:
                case MFC_ENC_SETCONF_I_PERIOD:
                case MFC_ENC_SETCONF_HIER_P:
			mfc_dbg("[%s] SetConf %d\n",__func__,type);
			if (ctx->c_ops->set_codec_cfg) {
				if ((ctx->c_ops->set_codec_cfg(ctx, type, value)) < 0)
					return MFC_SET_CONF_FAIL;
			}
			break;

                case MFC_DEC_GETCONF_CRC_DATA:
                case MFC_DEC_GETCONF_BUF_WIDTH_HEIGHT:
                case MFC_DEC_GETCONF_CROP_INFO:
                case MFC_DEC_GETCONF_FRAME_TAG:
                case MFC_DEC_GETCONF_WIDTH_HEIGHT:
                case MFC_ENC_GETCONF_FRAME_TAG:
			if (ctx->c_ops->get_codec_cfg) {
				if ((ctx->c_ops->get_codec_cfg(ctx, type, value)) < 0)
					return MFC_GET_CONF_FAIL;
			}
			break;
		default : 
		     printk(KERN_ERR "Invalid config paramter\n");
		     return MFC_FAIL;
	}

	return ret;
}

