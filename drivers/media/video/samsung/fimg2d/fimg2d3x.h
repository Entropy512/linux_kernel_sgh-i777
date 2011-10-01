/* linux/drivers/media/video/samsung/g2d/fimg2d3x.h
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

#ifndef _FIMG2D3X_H_
#define _FIMG2D3X_H_ __FILE__

/* externs */
extern void fimg2d3x_reset(struct fimg2d_control *info);
extern void fimg2d3x_enable_irq(struct fimg2d_control *info);
extern void fimg2d3x_disable_irq(struct fimg2d_control *info);
extern void fimg2d3x_clear_irq(struct fimg2d_control *info);
extern void fimg2d3x_clear_cache(struct fimg2d_control *info);
extern int fimg2d3x_is_finish(struct fimg2d_control *info);
extern void fimg2d3x_start_bitblt(struct fimg2d_control *info);
extern void fimg2d3x_enable_alpha(struct fimg2d_control *info, struct fimg2d_alpha *a);
extern void fimg2d3x_enable_rot90(struct fimg2d_control *info);
extern void fimg2d3x_disable_rot90(struct fimg2d_control *info);
extern void fimg2d3x_set_src_type(struct fimg2d_control *info, FIMG2D_IMG_T type);
extern void fimg2d3x_set_src_param(struct fimg2d_control *info, struct fimg2d_param *p);
extern void fimg2d3x_set_src_coordinate(struct fimg2d_control *info, struct fimg2d_rect *r);
extern void fimg2d3x_set_dst_type(struct fimg2d_control *info, FIMG2D_IMG_T type);
extern void fimg2d3x_set_dst_param(struct fimg2d_control *info, struct fimg2d_param *p);
extern void fimg2d3x_set_dst_coordinate(struct fimg2d_control *info, struct fimg2d_rect *r);
extern void fimg2d3x_set_pat_param(struct fimg2d_control *info, struct fimg2d_param *p);
extern void fimg2d3x_set_pat_offset(struct fimg2d_control *info,
		struct fimg2d_context *ctx, struct fimg2d_region *r);
extern void fimg2d3x_set_direction(struct fimg2d_control *info, struct fimg2d_context *ctx);
extern void fimg2d3x_set_opr3_type(struct fimg2d_control *info, FIMG2D_IMG_T type);
extern void fimg2d3x_set_rop_value(struct fimg2d_control *info, unsigned long value);
extern void fimg2d3x_set_fgcolor(struct fimg2d_control *info, unsigned long value);
extern void fimg2d3x_set_bgcolor(struct fimg2d_control *info, unsigned long value);
extern void fimg2d3x_enable_stretch(struct fimg2d_control *info);
extern void fimg2d3x_dump_regs(struct fimg2d_control *info);

#endif /* _FIMG2D3X_H_ */
