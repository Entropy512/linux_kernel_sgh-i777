/* linux/drivers/video/samsung/s3cfb.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Header file for Samsung Display Driver (FIMD) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _S3CFB_H
#define _S3CFB_H __FILE__

#ifdef __KERNEL__
#include <linux/mutex.h>
#include <linux/fb.h>
#ifdef CONFIG_HAS_WAKELOCK
#include <linux/wakelock.h>
#include <linux/earlysuspend.h>
#endif
#include <plat/fb.h>
#ifdef CONFIG_VCM
#include <plat/s5p-vcm.h>
#ifdef CONFIG_SUPPORT_UMP
#include "ump_kernel_interface_ref_drv.h"
#endif
#endif
#endif

#define TTT		"s3cfb"

#define S3CFB_NAME		"s3cfb"
#define S3CFB_AVALUE(r, g, b)	(((r & 0xf) << 8) | \
				((g & 0xf) << 4) | \
				((b & 0xf) << 0))
#define S3CFB_CHROMA(r, g, b)	(((r & 0xff) << 16) | \
				((g & 0xff) << 8) | \
				((b & 0xff) << 0))

#if defined(CONFIG_FB_S3C_DUAL_LCD)
#define FIMD_MAX		2
#else
#define FIMD_MAX		1
#endif
#define MAX_BUFFER_NUM		3

#define POWER_ON		1
#define POWER_OFF		0

enum s3cfb_data_path_t {
	DATA_PATH_FIFO = 0,
	DATA_PATH_DMA = 1,
	DATA_PATH_IPC = 2,
};

enum s3cfb_alpha_t {
	PLANE_BLENDING,
	PIXEL_BLENDING,
};

enum s3cfb_chroma_dir_t {
	CHROMA_FG,
	CHROMA_BG,
};

enum s3cfb_output_t {
	OUTPUT_RGB,
	OUTPUT_ITU,
	OUTPUT_I80LDI0,
	OUTPUT_I80LDI1,
	OUTPUT_WB_RGB,
	OUTPUT_WB_I80LDI0,
	OUTPUT_WB_I80LDI1,
};

enum s3cfb_rgb_mode_t {
	MODE_RGB_P = 0,
	MODE_BGR_P = 1,
	MODE_RGB_S = 2,
	MODE_BGR_S = 3,
};

enum s3cfb_mem_owner_t {
	DMA_MEM_NONE	= 0,
	DMA_MEM_FIMD	= 1,
	DMA_MEM_OTHER	= 2,
};

struct s3cfb_alpha {
	enum		s3cfb_alpha_t mode;
	int		channel;
	unsigned int	value;
};

struct s3cfb_chroma {
	int		enabled;
	int		blended;
	unsigned int	key;
	unsigned int	comp_key;
	unsigned int	alpha;
	enum		s3cfb_chroma_dir_t dir;
};

enum s3cfb_cpu_auto_cmd_rate {
	DISABLE_AUTO_FRM,
	PER_TWO_FRM,
	PER_FOUR_FRM,
	PER_SIX_FRM,
	PER_EIGHT_FRM,
	PER_TEN_FRM,
	PER_TWELVE_FRM,
	PER_FOURTEEN_FRM,
	PER_SIXTEEN_FRM,
	PER_EIGHTEEN_FRM,
	PER_TWENTY_FRM,
	PER_TWENTY_TWO_FRM,
	PER_TWENTY_FOUR_FRM,
	PER_TWENTY_SIX_FRM,
	PER_TWENTY_EIGHT_FRM,
	PER_THIRTY_FRM,
};

/*
 * struct s3cfb_lcd_timing
 * @h_fp:	horizontal front porch
 * @h_bp:	horizontal back porch
 * @h_sw:	horizontal sync width
 * @v_fp:	vertical front porch
 * @v_fpe:	vertical front porch for even field
 * @v_bp:	vertical back porch
 * @v_bpe:	vertical back porch for even field
*/
struct s3cfb_lcd_timing {
	int	h_fp;
	int	h_bp;
	int	h_sw;
	int	v_fp;
	int	v_fpe;
	int	v_bp;
	int	v_bpe;
	int	v_sw;
#ifdef CONFIG_FB_S3C_MIPI_LCD
	int	cmd_allow_len;
	void	(*cfg_gpio)(struct platform_device *dev);
	int	(*backlight_on)(struct platform_device *dev);
	int	(*reset_lcd)(struct platform_device *dev);
#endif
};

struct s3cfb_lcd_polarity {
	int rise_vclk;
	int inv_hsync;
	int inv_vsync;
	int inv_vden;
};

#ifdef CONFIG_FB_S3C_MIPI_LCD
/* for CPU Interface */
struct s3cfb_cpu_timing {
	unsigned int	cs_setup;
	unsigned int	wr_setup;
	unsigned int	wr_act;
	unsigned int	wr_hold;
};
#endif

/*
 * struct s3cfb_lcd
 * @name:		lcd panel name
 * @width:		horizontal resolution
 * @height:		vertical resolution
 * @width_mm:		width of picture in mm
 * @height_mm:		height of picture in mm
 * @bpp:		bits per pixel
 * @freq:		vframe frequency
 * @timing:		rgb timing values
 * @cpu_timing:		cpu timing values
 * @polarity:		polarity settings
 * @init_ldi:		pointer to LDI init function
 *
*/
struct s3cfb_lcd {
#ifdef CONFIG_FB_S3C_MIPI_LCD
	char	*name;
#endif
	int	width;
	int	height;
	int	p_width;
	int	p_height;
	int	bpp;
	int	freq;
	struct	s3cfb_lcd_timing timing;
	struct	s3cfb_lcd_polarity polarity;
#ifdef CONFIG_FB_S3C_MIPI_LCD
	struct	s3cfb_cpu_timing cpu_timing;
#endif
	void	(*init_ldi)(void);
	void	(*deinit_ldi)(void);
};

struct s3cfb_fimd_desc {
	int			state;
	int			dual;
	struct s3cfb_global	*fbdev[FIMD_MAX];
};

struct s3cfb_global {
	void __iomem		*regs;
	struct mutex		lock;
	struct device		*dev;
	struct clk		*clock;
	int			irq;
	wait_queue_head_t	wq;
	unsigned int		wq_count;
	struct fb_info		**fb;

	atomic_t		enabled_win;
	enum s3cfb_output_t	output;
	enum s3cfb_rgb_mode_t	rgb_mode;
	struct s3cfb_lcd	*lcd;
#ifdef CONFIG_VCM
	struct vcm		*s5p_vcm;
#endif

	int 			system_state;
#ifdef CONFIG_HAS_WAKELOCK
	struct early_suspend	early_suspend;
	struct wake_lock	idle_lock;
#endif
};

#ifdef CONFIG_VCM
struct s3cfb_vcm {
	struct	vcm	*dev_vcm;
	struct	vcm_res *dev_vcm_res;
};
#endif

struct s3cfb_window {
	int			id;
	int			enabled;
	atomic_t		in_use;
	int			x;
	int			y;
	enum			s3cfb_data_path_t path;
	enum			s3cfb_mem_owner_t owner;
	int			local_channel;
	int			dma_burst;
	unsigned int		pseudo_pal[16];
	struct			s3cfb_alpha alpha;
	struct			s3cfb_chroma chroma;
	int			power_state;

#ifdef CONFIG_VCM
	struct s3cfb_vcm	s3cfb_vcm[MAX_BUFFER_NUM];
	ump_dd_handle		ump_wrapped_buffer[MAX_BUFFER_NUM];
	struct vcm_res		*s5p_vcm_res;
#endif
};

struct s3cfb_user_window {
	int x;
	int y;
};

struct s3cfb_user_plane_alpha {
	int		channel;
	unsigned char	red;
	unsigned char	green;
	unsigned char	blue;
};

struct s3cfb_user_chroma {
	int		enabled;
	unsigned char	red;
	unsigned char	green;
	unsigned char	blue;
};

/* IOCTL commands */
#define S3CFB_WIN_POSITION		_IOW('F', 203, \
						struct s3cfb_user_window)
#define S3CFB_WIN_SET_PLANE_ALPHA	_IOW('F', 204, \
						struct s3cfb_user_plane_alpha)
#define S3CFB_WIN_SET_CHROMA		_IOW('F', 205, \
						struct s3cfb_user_chroma)
#define S3CFB_SET_VSYNC_INT		_IOW('F', 206, u32)
#define S3CFB_GET_VSYNC_INT_STATUS	_IOR('F', 207, u32)
#define S3CFB_GET_LCD_WIDTH		_IOR('F', 302, int)
#define S3CFB_GET_LCD_HEIGHT		_IOR('F', 303, int)
#define S3CFB_SET_WRITEBACK		_IOW('F', 304, u32)
#define S3CFB_SET_WIN_ON		_IOW('F', 305, u32)
#define S3CFB_SET_WIN_OFF		_IOW('F', 306, u32)
#define S3CFB_SET_WIN_PATH		_IOW('F', 307, \
						enum s3cfb_data_path_t)
#define S3CFB_SET_WIN_ADDR		_IOW('F', 308, unsigned long)
#define S3CFB_SET_WIN_MEM		_IOW('F', 309, \
						enum s3cfb_mem_owner_t)
#define S3CFB_GET_FB_PHY_ADDR           _IOR('F', 310, unsigned int)

#ifdef MALI_USE_UNIFIED_MEMORY_PROVIDER
#define S3CFB_GET_FB_UMP_SECURE_ID_0      _IOWR('m', 310, unsigned int)
#define S3CFB_GET_FB_UMP_SECURE_ID_1      _IOWR('m', 311, unsigned int)
#define S3CFB_GET_FB_UMP_SECURE_ID_2      _IOWR('m', 312, unsigned int)
#endif /* MALI_USE_UNIFIED_MEMORY_PROVIDER */

extern struct fb_ops			s3cfb_ops;
extern inline struct s3cfb_global	*get_fimd_global(int id);

/* S3CFB */
extern struct s3c_platform_fb *to_fb_plat(struct device *dev);
extern int s3cfb_draw_logo(struct fb_info *fb);
extern int s3cfb_enable_window(struct s3cfb_global *fbdev, int id);
extern int s3cfb_disable_window(struct s3cfb_global *fbdev, int id);
extern int s3cfb_update_power_state(struct s3cfb_global *fbdev, int id,
				int state);
extern int s3cfb_init_global(struct s3cfb_global *fbdev);
extern int s3cfb_map_video_memory(struct s3cfb_global *fbdev,
				struct fb_info *fb);
extern int s3cfb_unmap_video_memory(struct s3cfb_global *fbdev,
				struct fb_info *fb);
extern int s3cfb_map_default_video_memory(struct s3cfb_global *fbdev,
					struct fb_info *fb, int fimd_id);
extern int s3cfb_unmap_default_video_memory(struct s3cfb_global *fbdev,
					struct fb_info *fb);
extern int s3cfb_set_bitfield(struct fb_var_screeninfo *var);
extern int s3cfb_set_alpha_info(struct fb_var_screeninfo *var,
				struct s3cfb_window *win);
extern int s3cfb_check_var(struct fb_var_screeninfo *var, struct fb_info *fb);
extern int s3cfb_check_var_window(struct s3cfb_global *fbdev,
			struct fb_var_screeninfo *var, struct fb_info *fb);
extern void s3cfb_set_win_params(struct s3cfb_global *fbdev, int id);
extern int s3cfb_set_par_window(struct s3cfb_global *fbdev, struct fb_info *fb);
extern int s3cfb_set_par(struct fb_info *fb);
extern int s3cfb_init_fbinfo(struct s3cfb_global *fbdev, int id);
extern int s3cfb_alloc_framebuffer(struct s3cfb_global *fbdev, int fimd_id);
extern int s3cfb_open(struct fb_info *fb, int user);
extern int s3cfb_release_window(struct fb_info *fb);
extern int s3cfb_release(struct fb_info *fb, int user);
extern int s3cfb_pan_display(struct fb_var_screeninfo *var,
				struct fb_info *fb);
extern int s3cfb_blank(int blank_mode, struct fb_info *fb);
extern inline unsigned int __chan_to_field(unsigned int chan,
					struct fb_bitfield bf);
extern int s3cfb_setcolreg(unsigned int regno, unsigned int red,
			unsigned int green, unsigned int blue,
			unsigned int transp, struct fb_info *fb);
extern int s3cfb_cursor(struct fb_info *fb, struct fb_cursor *cursor);
extern int s3cfb_ioctl(struct fb_info *fb, unsigned int cmd, unsigned long arg);
extern int s3cfb_enable_localpath(struct s3cfb_global *fbdev, int id);
extern int s3cfb_disable_localpath(struct s3cfb_global *fbdev, int id);

/* FIMD */
extern int s3cfb_clear_interrupt(struct s3cfb_global *ctrl);
extern int s3cfb_display_on(struct s3cfb_global *ctrl);
extern int s3cfb_set_output(struct s3cfb_global *ctrl);
extern int s3cfb_set_display_mode(struct s3cfb_global *ctrl);
extern int s3cfb_display_on(struct s3cfb_global *ctrl);
extern int s3cfb_display_off(struct s3cfb_global *ctrl);
extern int s3cfb_set_clock(struct s3cfb_global *ctrl);
extern int s3cfb_set_polarity(struct s3cfb_global *ctrl);
extern int s3cfb_set_timing(struct s3cfb_global *ctrl);
extern int s3cfb_set_lcd_size(struct s3cfb_global *ctrl);
extern int s3cfb_set_global_interrupt(struct s3cfb_global *ctrl, int enable);
extern int s3cfb_set_vsync_interrupt(struct s3cfb_global *ctrl, int enable);
extern int s3cfb_get_vsync_interrupt(struct s3cfb_global *ctrl);
extern int s3cfb_set_fifo_interrupt(struct s3cfb_global *ctrl, int enable);
extern int s3cfb_window_on(struct s3cfb_global *ctrl, int id);
extern int s3cfb_window_off(struct s3cfb_global *ctrl, int id);
extern int s3cfb_win_map_on(struct s3cfb_global *ctrl, int id, int color);
extern int s3cfb_win_map_off(struct s3cfb_global *ctrl, int id);
extern int s3cfb_set_window_control(struct s3cfb_global *ctrl, int id);
extern int s3cfb_set_alpha_blending(struct s3cfb_global *ctrl, int id);
extern int s3cfb_set_alpha_value(struct s3cfb_global *ctrl, int value);
extern int s3cfb_set_window_position(struct s3cfb_global *ctrl, int id);
extern int s3cfb_set_window_size(struct s3cfb_global *ctrl, int id);
extern int s3cfb_set_buffer_address(struct s3cfb_global *ctrl, int id);
extern int s3cfb_set_buffer_size(struct s3cfb_global *ctrl, int id);
extern int s3cfb_set_chroma_key(struct s3cfb_global *ctrl, int id);
extern int s3cfb_channel_localpath_on(struct s3cfb_global *ctrl, int id);
extern int s3cfb_channel_localpath_off(struct s3cfb_global *ctrl, int id);
#ifdef CONFIG_FB_S3C_MIPI_LCD
extern void s3cfb_set_trigger(struct s3cfb_global *ctrl);
#endif

#ifdef CONFIG_HAS_WAKELOCK
#ifdef CONFIG_HAS_EARLYSUSPEND
extern void s3cfb_early_suspend(struct early_suspend *h);
extern void s3cfb_late_resume(struct early_suspend *h);
#endif
#endif

/* LCD */
extern void s3cfb_set_lcd_info(struct s3cfb_global *ctrl);

#endif /* _S3CFB_H */
