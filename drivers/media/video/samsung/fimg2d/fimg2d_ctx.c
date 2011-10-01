/* linux/drivers/media/video/samsung/fimg2d/fimg2d_ctx.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *	http://www.samsung.com/
 *
 * Samsung Graphics 2D driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/slab.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <plat/fimg2d.h>

#include "fimg2d.h"

/**
 * fimg2d_match_closed - [GENERIC] compare if closed
 * @ctx: context info
 * @data: closed value to be compared
*/
inline int fimg2d_match_closed(struct fimg2d_context *ctx, void *data)
{
	return (atomic_read(&ctx->closed) == (int)data);
}

/**
 * fimg2d_enqueue - [GENERIC] add an item to queue
 * @info: controller info
 * @node: list_head of struct to be added
 * @q: list_head of destination queue
*/
inline void fimg2d_enqueue(struct fimg2d_control *info,
			struct list_head *node, struct list_head *q)
{
	list_add_tail(node, q);
}

/**
 * fimg2d_dequeue - [GENERIC] remove an item from queue
 * @info: controller info
 * @node: list_head of struct to be removed
*/
inline void fimg2d_dequeue(struct fimg2d_control *info, struct list_head *node)
{
	list_del(node);
}

/**
 * fimg2d_queue_is_empty - [GENERIC] check if queue is empty
 * @q: queue to be checked
*/
inline int fimg2d_queue_is_empty(struct list_head *q)
{
	return list_empty(q);
}

/**
 * fimg2d_get_first_region - [GENERIC] return first entry of region queue
 * @ctx: context info
*/
inline struct fimg2d_region *fimg2d_get_first_region(struct fimg2d_context *ctx)
{
	if (list_empty(&ctx->reg_q))
		return NULL;
	else
		return list_first_entry(&ctx->reg_q, struct fimg2d_region, node);
}

/**
 * fimg2d_find_context - [GENERIC] find a context
 * @info: controller info
 * @data: id or status to be used for comparing
 * @match: function pointer to compare
*/
struct fimg2d_context *
fimg2d_find_context(struct fimg2d_control *info, void *data,
			int (*match)(struct fimg2d_context *ctx, void *data))
{
	struct fimg2d_context *ctx;
	int found;

	found = 0;
	list_for_each_entry(ctx, &info->ctx_q, node) {
		if (match(ctx, data)) {
			found = 1;
			break;
		}
	}

	return (found ? ctx : NULL);
}

/**
 * fimg2d_set_context - [GENERIC] configure a context
 * @info: controller info
 * @ctx: context info
 * @c: context provided by user
 *
 * This function performs below:
 *  1) copy user data to kernel space
 *  2) decode rendering type
 *  3) initialize list_head and context queue
*/
int fimg2d_set_context(struct fimg2d_control *info, struct fimg2d_context *ctx,
			struct fimg2d_user_context __user *c)
{
	fimg2d_debug("context: %p\n", ctx);

	/* clear old set */
	atomic_set(&ctx->closed, 0);
	ctx->color = 0;
	ctx->rop = 0;
	memset(&ctx->alpha, 0, sizeof(ctx->alpha));
	memset(&ctx->src, 0, sizeof(ctx->src));
	memset(&ctx->dst, 0, sizeof(ctx->dst));

	/* scale and rotation */
	get_user(ctx->scale, &c->scale);
	get_user(ctx->rot, &c->rot);

	/* alpha */
	if (copy_from_user(&ctx->alpha, &c->alpha, sizeof(ctx->alpha)))
		return -EFAULT;

	/* copy src and dst info: not needed if NULL */
	if (c->src && copy_from_user(&ctx->src, c->src, sizeof(ctx->src)))
		return -EFAULT;

	if (c->dst && copy_from_user(&ctx->dst, c->dst, sizeof(ctx->dst)))
		return -EFAULT;

	/*
	 * decode for:
	 *  1) get ROP value
	 *  2) configure misc values such as 3rd opr and alpha if needed
	*/
	ctx->rop = info->decode(ctx, c);
	if (ctx->rop == 0)
		return -EFAULT;

	/* initialize master queue node and region queue */
	INIT_LIST_HEAD(&ctx->reg_q);

	return 0;
}

/**
 * fimg2d_set_region - [INTERNAL] configure a region
 * @reg: region to be configured
 * @r: region provided by user
*/
static int fimg2d_set_region(struct fimg2d_region *reg,
				struct fimg2d_user_region __user *r)
{
	/* initialize node for region queue */
	INIT_LIST_HEAD(&reg->node);

	/* update region info */
	if (r->src && copy_from_user(&reg->src, r->src, sizeof(reg->src)))
		return -EFAULT;

	if (r->dst && copy_from_user(&reg->dst, r->dst, sizeof(reg->dst)))
		return -EFAULT;

	return 0;
}

/**
 * fimg2d_add_region - [GENERIC] add a new region to existing context
 * @info: controller info
 * @ctx: context info
 * @r: user passed region
*/
int fimg2d_add_region(struct fimg2d_control *info, struct fimg2d_context *ctx,
			struct fimg2d_user_region __user *r)
{
	struct fimg2d_region *reg;
	int ret;

	fimg2d_debug("context: %p\n", ctx);

	if (atomic_read(&ctx->closed)) {
		printk(KERN_ERR "closed: not permitted to add region\n");
		return -EFAULT;
	}

	reg = kzalloc(sizeof(*reg), GFP_KERNEL);
	if (!reg) {
		printk(KERN_ERR "failed to create region header\n");
		return -ENOMEM;
	}

	/* set region info */
	ret = fimg2d_set_region(reg, r);
	if (ret) {
		printk(KERN_ERR "failed to set region info\n");
		kfree(reg);
		return ret;
	}

	/* add to region queue */
	fimg2d_enqueue(info, &reg->node, &ctx->reg_q);

	return 0;
}

/**
 * fimg2d_close_bitblt - [GENERIC] close context
 * @info: controller info
 * @ctx: context info
*/
int fimg2d_close_bitblt(struct fimg2d_control *info, struct fimg2d_context *ctx)
{
	fimg2d_debug("context: %p\n", ctx);

	spin_lock(&info->lock);
	atomic_set(&ctx->closed, 1);
	spin_unlock(&info->lock);

	return 0;
}
