/* linux/drivers/media/video/samsung/tvout/s5p_tvif_ctrl.c
 *
 * Copyright (c) 2009 Samsung Electronics
 *		http://www.samsung.com/
 *
 * Tvout ctrl class for Samsung TVOUT driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*****************************************************************************
 * This file includes functions for ctrl classes of TVOUT driver.
 * There are 3 ctrl classes. (tvif, hdmi, sdo)
 *	- tvif ctrl class: controls hdmi and sdo ctrl class.
 *      - hdmi ctrl class: contrls hdmi hardware by using hw_if/hdmi.c
 *      - sdo  ctrl class: contrls sdo hardware by using hw_if/sdo.c
 *
 *                      +-----------------+
 *                      | tvif ctrl class |
 *                      +-----------------+
 *                             |   |
 *                  +----------+   +----------+		    ctrl class layer
 *                  |                         |
 *                  V                         V
 *         +-----------------+       +-----------------+
 *         | sdo ctrl class  |       | hdmi ctrl class |
 *         +-----------------+       +-----------------+
 *                  |                         |
 *   ---------------+-------------------------+------------------------------
 *                  V                         V
 *         +-----------------+       +-----------------+
 *         |   hw_if/sdo.c   |       |   hw_if/hdmi.c  |         hw_if layer
 *         +-----------------+       +-----------------+
 *                  |                         |
 *   ---------------+-------------------------+------------------------------
 *                  V                         V
 *         +-----------------+       +-----------------+
 *         |   sdo hardware  |       |   hdmi hardware |	  Hardware
 *         +-----------------+       +-----------------+
 *
 ****************************************************************************/
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>

#ifdef CONFIG_CPU_S5PV210
#include <mach/pd.h>
#endif

#include "s5p_tvout_common_lib.h"
#include "hw_if/hw_if.h"
#include "s5p_tvout_ctrl.h"

/****************************************
 * Definitions for sdo ctrl class
 ***************************************/
enum {
	SDO_PCLK = 0,
	SDO_MUX,
	SDO_NO_OF_CLK
};

struct s5p_sdo_vscale_cfg {
	enum s5p_sdo_level		composite_level;
	enum s5p_sdo_vsync_ratio	composite_ratio;
};

struct s5p_sdo_vbi {
	bool wss_cvbs;
	enum s5p_sdo_closed_caption_type caption_cvbs;
};

struct s5p_sdo_offset_gain {
	u32 offset;
	u32 gain;
};

struct s5p_sdo_delay {
	u32 delay_y;
	u32 offset_video_start;
	u32 offset_video_end;
};

struct s5p_sdo_component_porch {
	u32 back_525;
	u32 front_525;
	u32 back_625;
	u32 front_625;
};

struct s5p_sdo_ch_xtalk_cancellat_coeff {
	u32 coeff1;
	u32 coeff2;
};

struct s5p_sdo_closed_caption {
	u32 display_cc;
	u32 nondisplay_cc;
};

struct s5p_sdo_ctrl_private_data {
	struct s5p_sdo_vscale_cfg		video_scale_cfg;
	struct s5p_sdo_vbi			vbi;
	struct s5p_sdo_offset_gain		offset_gain;
	struct s5p_sdo_delay			delay;
	struct s5p_sdo_bright_hue_saturation	bri_hue_sat;
	struct s5p_sdo_cvbs_compensation	cvbs_compen;
	struct s5p_sdo_component_porch		compo_porch;
	struct s5p_sdo_ch_xtalk_cancellat_coeff	xtalk_cc;
	struct s5p_sdo_closed_caption		closed_cap;
	struct s5p_sdo_525_data			wss_525;
	struct s5p_sdo_625_data			wss_625;
	struct s5p_sdo_525_data			cgms_525;
	struct s5p_sdo_625_data			cgms_625;

	bool			color_sub_carrier_phase_adj;

	bool			running;

	struct s5p_tvout_clk_info	clk[SDO_NO_OF_CLK];
	char			*pow_name;
	struct reg_mem_info	reg_mem;
};

static struct s5p_sdo_ctrl_private_data s5p_sdo_ctrl_private = {
	.clk[SDO_PCLK] = {
		.name			= "tvenc",
		.ptr			= NULL
	},
	.clk[SDO_MUX] = {
		.name			= "sclk_dac",
		.ptr			= NULL
	},
		.pow_name		= "tv_enc_pd",
	.reg_mem = {
		.name			= "s5p-sdo",
		.res			= NULL,
		.base			= NULL
	},

	.running			= false,

	.color_sub_carrier_phase_adj	= false,

	.vbi = {
		.wss_cvbs		= true,
		.caption_cvbs		= SDO_INS_OTHERS
	},

	.offset_gain = {
		.offset			= 0,
		.gain			= 0x800
	},

	.delay = {
		.delay_y		= 0x00,
		.offset_video_start	= 0xfa,
		.offset_video_end	= 0x00
	},

	.bri_hue_sat = {
		.bright_hue_sat_adj	= false,
		.gain_brightness	= 0x80,
		.offset_brightness	= 0x00,
		.gain0_cb_hue_sat	= 0x00,
		.gain1_cb_hue_sat	= 0x00,
		.gain0_cr_hue_sat	= 0x00,
		.gain1_cr_hue_sat	= 0x00,
		.offset_cb_hue_sat	= 0x00,
		.offset_cr_hue_sat	= 0x00
	},

	.cvbs_compen = {
		.cvbs_color_compen	= false,
		.y_lower_mid		= 0x200,
		.y_bottom		= 0x000,
		.y_top			= 0x3ff,
		.y_upper_mid		= 0x200,
		.radius			= 0x1ff
	},

	.compo_porch = {
		.back_525		= 0x8a,
		.front_525		= 0x359,
		.back_625		= 0x96,
		.front_625		= 0x35c
	},

	.xtalk_cc = {
		.coeff2			= 0,
		.coeff1			= 0
	},

	.closed_cap = {
		.display_cc		= 0,
		.nondisplay_cc		= 0
	},

	.wss_525 = {
		.copy_permit		= SDO_525_COPY_PERMIT,
		.mv_psp			= SDO_525_MV_PSP_OFF,
		.copy_info		= SDO_525_COPY_INFO,
		.analog_on		= false,
		.display_ratio		= SDO_525_4_3_NORMAL
	},

	.wss_625 = {
		.surround_sound		= false,
		.copyright		= false,
		.copy_protection	= false,
		.text_subtitles		= false,
		.open_subtitles		= SDO_625_NO_OPEN_SUBTITLES,
		.camera_film		= SDO_625_CAMERA,
		.color_encoding		= SDO_625_NORMAL_PAL,
		.helper_signal		= false,
		.display_ratio		= SDO_625_4_3_FULL_576
	},

	.cgms_525 = {
		.copy_permit		= SDO_525_COPY_PERMIT,
		.mv_psp			= SDO_525_MV_PSP_OFF,
		.copy_info		= SDO_525_COPY_INFO,
		.analog_on		= false,
		.display_ratio		= SDO_525_4_3_NORMAL
	},

	.cgms_625 = {
		.surround_sound		= false,
		.copyright		= false,
		.copy_protection	= false,
		.text_subtitles		= false,
		.open_subtitles		= SDO_625_NO_OPEN_SUBTITLES,
		.camera_film		= SDO_625_CAMERA,
		.color_encoding		= SDO_625_NORMAL_PAL,
		.helper_signal		= false,
		.display_ratio		= SDO_625_4_3_FULL_576
	},
};





/****************************************
 * Definitions for hdmi ctrl class
 ***************************************/

#define AVI_SAME_WITH_PICTURE_AR	(0x1<<3)

enum {
	HDMI_PCLK = 0,
	HDMI_MUX,
	HDMI_NO_OF_CLK
};

enum {
	HDMI = 0,
	HDMI_PHY,
	HDMI_NO_OF_MEM_RES
};

enum s5p_hdmi_pic_aspect {
	HDMI_PIC_RATIO_4_3	= 1,
	HDMI_PIC_RATIO_16_9	= 2
};

enum s5p_hdmi_colorimetry {
	HDMI_CLRIMETRY_NO	= 0x00,
	HDMI_CLRIMETRY_601	= 0x40,
	HDMI_CLRIMETRY_709	= 0x80,
	HDMI_CLRIMETRY_X_VAL	= 0xc0,
};

enum s5p_hdmi_audio_type {
	HDMI_GENERIC_AUDIO,
	HDMI_60958_AUDIO,
	HDMI_DVD_AUDIO,
	HDMI_SUPER_AUDIO,
};

enum s5p_hdmi_v_mode {
	v640x480p_60Hz,
	v720x480p_60Hz,
	v1280x720p_60Hz,
	v1920x1080i_60Hz,
	v720x480i_60Hz,
	v720x240p_60Hz,
	v2880x480i_60Hz,
	v2880x240p_60Hz,
	v1440x480p_60Hz,
	v1920x1080p_60Hz,
	v720x576p_50Hz,
	v1280x720p_50Hz,
	v1920x1080i_50Hz,
	v720x576i_50Hz,
	v720x288p_50Hz,
	v2880x576i_50Hz,
	v2880x288p_50Hz,
	v1440x576p_50Hz,
	v1920x1080p_50Hz,
	v1920x1080p_24Hz,
	v1920x1080p_25Hz,
	v1920x1080p_30Hz,
	v2880x480p_60Hz,
	v2880x576p_50Hz,
	v1920x1080i_50Hz_1250,
	v1920x1080i_100Hz,
	v1280x720p_100Hz,
	v720x576p_100Hz,
	v720x576i_100Hz,
	v1920x1080i_120Hz,
	v1280x720p_120Hz,
	v720x480p_120Hz,
	v720x480i_120Hz,
	v720x576p_200Hz,
	v720x576i_200Hz,
	v720x480p_240Hz,
	v720x480i_240Hz,
	v720x480p_59Hz,
	v1280x720p_59Hz,
	v1920x1080i_59Hz,
	v1920x1080p_59Hz,
};

struct s5p_hdmi_bluescreen {
	bool	enable;
	u8	cb_b;
	u8	y_g;
	u8	cr_r;
};

struct s5p_hdmi_packet {
	u8				acr[7];
	u8				asp[7];
	u8				gcp[7];
	u8				acp[28];
	u8				isrc1[16];
	u8				isrc2[16];
	u8				obas[7];
	u8				dst[28];
	u8				gmp[28];

	u8				spd_vendor[8];
	u8				spd_product[16];

	u8				vsi[27];
	u8				avi[27];
	u8				spd[27];
	u8				aui[27];
	u8				mpg[27];

	struct s5p_hdmi_infoframe	vsi_info;
	struct s5p_hdmi_infoframe	avi_info;
	struct s5p_hdmi_infoframe	spd_info;
	struct s5p_hdmi_infoframe	aui_info;
	struct s5p_hdmi_infoframe	mpg_info;

	u8				h_asp[3];
	u8				h_acp[3];
	u8				h_isrc[3];
};

struct s5p_hdmi_color_range {
	u8	y_min;
	u8	y_max;
	u8	c_min;
	u8	c_max;
};

struct s5p_hdmi_tg {
	bool correction_en;
	bool bt656_en;
};

struct s5p_hdmi_audio {
	enum s5p_hdmi_audio_type	type;
	u32				freq;
	u32				bit;
	u32				channel;

	u8				on;
};

struct s5p_hdmi_video {
	struct s5p_hdmi_color_range	color_r;
	enum s5p_hdmi_pic_aspect	aspect;
	enum s5p_hdmi_colorimetry	colorimetry;
	enum s5p_hdmi_color_depth	depth;
};

struct s5p_hdmi_o_params {
	struct s5p_hdmi_o_trans	trans;
	struct s5p_hdmi_o_reg	reg;
};

struct s5p_hdmi_ctrl_private_data {
	u8				vendor[8];
	u8				product[16];

	enum s5p_tvout_o_mode		out;
	enum s5p_hdmi_v_mode		mode;

	struct s5p_hdmi_bluescreen	blue_screen;
	struct s5p_hdmi_packet		packet;
	struct s5p_hdmi_tg		tg;
	struct s5p_hdmi_audio		audio;
	struct s5p_hdmi_video		video;

	bool				hpd_status;
	bool				hdcp_en;

	bool				av_mute;

	bool				running;
	char				*pow_name;
	struct s5p_tvout_clk_info		clk[HDMI_NO_OF_CLK];
	struct reg_mem_info		reg_mem[HDMI_NO_OF_MEM_RES];
	struct irq_info			irq;
};

static struct s5p_hdmi_v_format s5p_hdmi_v_fmt[] = {
	[v720x480p_60Hz] = {
		.frame = {
			.vic		= 2,
			.vic_16_9	= 3,
			.repetition	= 0,
			.polarity	= 1,
			.i_p		= 0,
			.h_active	= 720,
			.v_active	= 480,
			.h_total	= 858,
			.h_blank	= 138,
			.v_total	= 525,
			.v_blank	= 45,
			.pixel_clock	= ePHY_FREQ_27_027,
		},
		.h_sync = {
			.begin		= 0xe,
			.end		= 0x4c,
		},
		.v_sync_top = {
			.begin		= 0x9,
			.end		= 0xf,
		},
		.v_sync_bottom = {
			.begin		= 0,
			.end		= 0,
		},
		.v_sync_h_pos = {
			.begin		= 0,
			.end		= 0,
		},
		.v_blank_f = {
			.begin		= 0,
			.end		= 0,
		},
		.mhl_hsync = 0xf,
		.mhl_vsync = 0x1,
	},

	[v1280x720p_60Hz] = {
		.frame = {
			.vic		= 4,
			.vic_16_9	= 4,
			.repetition	= 0,
			.polarity	= 0,
			.i_p		= 0,
			.h_active	= 1280,
			.v_active	= 720,
			.h_total	= 1650,
			.h_blank	= 370,
			.v_total	= 750,
			.v_blank	= 30,
			.pixel_clock	= ePHY_FREQ_74_250,
		},
		.h_sync = {
			.begin		= 0x6c,
			.end		= 0x94,
		},
		.v_sync_top = {
			.begin		= 0x5,
			.end		= 0xa,
		},
		.v_sync_bottom = {
			.begin		= 0,
			.end		= 0,
		},
		.v_sync_h_pos = {
			.begin		= 0,
			.end		= 0,
		},
		.v_blank_f = {
			.begin		= 0,
			.end		= 0,
		},
		.mhl_hsync = 0xf,
		.mhl_vsync = 0x1,
	},

	[v1920x1080i_60Hz] = {
		.frame = {
			.vic		= 5,
			.vic_16_9	= 5,
			.repetition	= 0,
			.polarity	= 0,
			.i_p		= 1,
			.h_active	= 1920,
			.v_active	= 540,
			.h_total	= 2200,
			.h_blank	= 280,
			.v_total	= 1125,
			.v_blank	= 22,
			.pixel_clock	= ePHY_FREQ_74_250,
		},
		.h_sync = {
			.begin		= 0x56,
			.end		= 0x82,
		},
		.v_sync_top = {
			.begin		= 0x2,
			.end		= 0x7,
		},
		.v_sync_bottom = {
			.begin		= 0x234,
			.end		= 0x239,
		},
		.v_sync_h_pos = {
			.begin		= 0x4a4,
			.end		= 0x4a4,
		},
		.v_blank_f = {
			.begin		= 0x249,
			.end		= 0x465,
		},
		.mhl_hsync = 0xf,
		.mhl_vsync = 0x1,
	},

	[v1920x1080p_60Hz] = {
		.frame = {
			.vic		= 16,
			.vic_16_9	= 16,
			.repetition	= 0,
			.polarity	= 0,
			.i_p		= 0,
			.h_active	= 1920,
			.v_active	= 1080,
			.h_total	= 2200,
			.h_blank	= 280,
			.v_total	= 1125,
			.v_blank	= 45,
			.pixel_clock	= ePHY_FREQ_148_500,
		},
		.h_sync = {
			.begin		= 0x56,
			.end		= 0x82,
		},
		.v_sync_top = {
			.begin		= 0x4,
			.end		= 0x9,
		},
		.v_sync_bottom = {
			.begin		= 0,
			.end		= 0,
		},
		.v_sync_h_pos = {
			.begin		= 0,
			.end		= 0,
		},
		.v_blank_f = {
			.begin		= 0,
			.end		= 0,
		},
		.mhl_hsync = 0xf,
		.mhl_vsync = 0x1,
	},

	[v720x576p_50Hz] = {
		.frame = {
			.vic		= 17,
			.vic_16_9	= 18,
			.repetition	= 0,
			.polarity	= 1,
			.i_p		= 0,
			.h_active	= 720,
			.v_active	= 576,
			.h_total	= 864,
			.h_blank	= 144,
			.v_total	= 625,
			.v_blank	= 49,
			.pixel_clock	= ePHY_FREQ_27,
		},
		.h_sync = {
			.begin		= 0xa,
			.end		= 0x4a,
		},
		.v_sync_top = {
			.begin		= 0x5,
			.end		= 0xa,
		},
		.v_sync_bottom = {
			.begin		= 0,
			.end		= 0,
		},
		.v_sync_h_pos = {
			.begin		= 0,
			.end		= 0,
		},
		.v_blank_f = {
			.begin		= 0,
			.end		= 0,
		},
		.mhl_hsync = 0xf,
		.mhl_vsync = 0x1,
	},

	[v1280x720p_50Hz] = {
		.frame = {
			.vic		= 19,
			.vic_16_9	= 19,
			.repetition	= 0,
			.polarity	= 0,
			.i_p		= 0,
			.h_active	= 1280,
			.v_active	= 720,
			.h_total	= 1980,
			.h_blank	= 700,
			.v_total	= 750,
			.v_blank	= 30,
			.pixel_clock	= ePHY_FREQ_74_250,
		},
		.h_sync = {
			.begin		= 0x1b6,
			.end		= 0x1de,
		},
		.v_sync_top = {
			.begin		= 0x5,
			.end		= 0xa,
		},
		.v_sync_bottom = {
			.begin		= 0,
			.end		= 0,
		},
		.v_sync_h_pos = {
			.begin		= 0,
			.end		= 0,
		},
		.v_blank_f = {
			.begin		= 0,
			.end		= 0,
		},
		.mhl_hsync = 0xf,
		.mhl_vsync = 0x1,
	},

	[v1920x1080i_50Hz] = {
		.frame = {
			.vic		= 20,
			.vic_16_9	= 20,
			.repetition	= 0,
			.polarity	= 0,
			.i_p		= 1,
			.h_active	= 1920,
			.v_active	= 540,
			.h_total	= 2640,
			.h_blank	= 720,
			.v_total	= 1125,
			.v_blank	= 22,
			.pixel_clock	= ePHY_FREQ_74_250,
		},
		.h_sync = {
			.begin		= 0x20e,
			.end		= 0x23a,
		},
		.v_sync_top = {
			.begin		= 0x2,
			.end		= 0x7,
		},
		.v_sync_bottom = {
			.begin		= 0x234,
			.end		= 0x239,
		},
		.v_sync_h_pos = {
			.begin		= 0x738,
			.end		= 0x738,
		},
		.v_blank_f = {
			.begin		= 0x249,
			.end		= 0x465,
		},
		.mhl_hsync = 0xf,
		.mhl_vsync = 0x1,
	},

	[v1920x1080p_50Hz] = {
		.frame = {
			.vic		= 31,
			.vic_16_9	= 31,
			.repetition	= 0,
			.polarity	= 0,
			.i_p		= 0,
			.h_active	= 1920,
			.v_active	= 1080,
			.h_total	= 2640,
			.h_blank	= 720,
			.v_total	= 1125,
			.v_blank	= 45,
			.pixel_clock	= ePHY_FREQ_148_500,
		},
		.h_sync = {
			.begin		= 0x20e,
			.end		= 0x23a,
		},
		.v_sync_top = {
			.begin		= 0x4,
			.end		= 0x9,
		},
		.v_sync_bottom = {
			.begin		= 0,
			.end		= 0,
		},
		.v_sync_h_pos = {
			.begin		= 0,
			.end		= 0,
		},
		.v_blank_f = {
			.begin		= 0,
			.end		= 0,
		},
		.mhl_hsync = 0xf,
		.mhl_vsync = 0x1,
	},

	[v1920x1080p_30Hz] = {
		.frame = {
			.vic		= 34,
			.vic_16_9	= 34,
			.repetition	= 0,
			.polarity	= 0,
			.i_p		= 0,
			.h_active	= 1920,
			.v_active	= 1080,
			.h_total	= 2200,
			.h_blank	= 280,
			.v_total	= 1125,
			.v_blank	= 45,
			.pixel_clock	= ePHY_FREQ_74_250,
		},
		.h_sync = {
			.begin		= 0x56,
			.end		= 0x82,
		},
		.v_sync_top = {
			.begin		= 0x4,
			.end		= 0x9,
		},
		.v_sync_bottom = {
			.begin		= 0,
			.end		= 0,
		},
		.v_sync_h_pos = {
			.begin		= 0,
			.end		= 0,
		},
		.v_blank_f = {
			.begin		= 0,
			.end		= 0,
		},
		.mhl_hsync = 0xf,
		.mhl_vsync = 0x1,
	},

	[v720x480p_59Hz] = {
		.frame = {
			.vic		= 2,
			.vic_16_9	= 3,
			.repetition	= 0,
			.polarity	= 1,
			.i_p		= 0,
			.h_active	= 720,
			.v_active	= 480,
			.h_total	= 858,
			.h_blank	= 138,
			.v_total	= 525,
			.v_blank	= 45,
			.pixel_clock	= ePHY_FREQ_27,
		},
		.h_sync = {
			.begin		= 0xe,
			.end		= 0x4c,
		},
		.v_sync_top = {
			.begin		= 0x9,
			.end		= 0xf,
		},
		.v_sync_bottom = {
			.begin		= 0,
			.end		= 0,
		},
		.v_sync_h_pos = {
			.begin		= 0,
			.end		= 0,
		},
		.v_blank_f = {
			.begin		= 0,
			.end		= 0,
		},
		.mhl_hsync = 0xf,
		.mhl_vsync = 0x1,
	},

	[v1280x720p_59Hz] = {
		.frame = {
			.vic		= 4,
			.vic_16_9	= 4,
			.repetition	= 0,
			.polarity	= 0,
			.i_p		= 0,
			.h_active	= 1280,
			.v_active	= 720,
			.h_total	= 1650,
			.h_blank	= 370,
			.v_total	= 750,
			.v_blank	= 30,
			.pixel_clock	= ePHY_FREQ_74_176,
		},
		.h_sync = {
			.begin		= 0x6c,
			.end		= 0x94,
		},
		.v_sync_top = {
			.begin		= 0x5,
			.end		= 0xa,
		},
		.v_sync_bottom = {
			.begin		= 0,
			.end		= 0,
		},
		.v_sync_h_pos = {
			.begin		= 0,
			.end		= 0,
		},
		.v_blank_f = {
			.begin		= 0,
			.end		= 0,
		},
		.mhl_hsync = 0xf,
		.mhl_vsync = 0x1,
	},

	[v1920x1080i_59Hz] = {
		.frame = {
			.vic		= 5,
			.vic_16_9	= 5,
			.repetition	= 0,
			.polarity	= 0,
			.i_p		= 1,
			.h_active	= 1920,
			.v_active	= 540,
			.h_total	= 2200,
			.h_blank	= 280,
			.v_total	= 1125,
			.v_blank	= 22,
			.pixel_clock	= ePHY_FREQ_74_176,
		},
		.h_sync = {
			.begin		= 0x56,
			.end		= 0x82,
		},
		.v_sync_top = {
			.begin		= 0x2,
			.end		= 0x7,
		},
		.v_sync_bottom = {
			.begin		= 0x234,
			.end		= 0x239,
		},
		.v_sync_h_pos = {
			.begin		= 0x4a4,
			.end		= 0x4a4,
		},
		.v_blank_f = {
			.begin		= 0x249,
			.end		= 0x465,
		},
		.mhl_hsync = 0xf,
		.mhl_vsync = 0x1,
	},

	[v1920x1080p_59Hz] = {
		.frame = {
			.vic		= 16,
			.vic_16_9	= 16,
			.repetition	= 0,
			.polarity	= 0,
			.i_p		= 0,
			.h_active	= 1920,
			.v_active	= 1080,
			.h_total	= 2200,
			.h_blank	= 280,
			.v_total	= 1125,
			.v_blank	= 45,
			.pixel_clock	= ePHY_FREQ_148_352,
		},
		.h_sync = {
			.begin		= 0x56,
			.end		= 0x82,
		},
		.v_sync_top = {
			.begin		= 0x4,
			.end		= 0x9,
		},
		.v_sync_bottom = {
			.begin		= 0,
			.end		= 0,
		},
		.v_sync_h_pos = {
			.begin		= 0,
			.end		= 0,
		},
		.v_blank_f = {
			.begin		= 0,
			.end		= 0,
		},
		.mhl_hsync = 0xf,
		.mhl_vsync = 0x1,
	},
};

static struct s5p_hdmi_o_params s5p_hdmi_output[] = {
	{
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00},
	}, {
		{0x02, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x02, 0x04},
		{0x40, 0x00, 0x02, 0x00, 0x00},
	}, {
		{0x02, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x02, 0x04},
		{0x00, 0x00, 0x02, 0x20, 0x00},
	}, {
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x22, 0x01, 0x20, 0x01},
	},
};

static struct s5p_hdmi_ctrl_private_data s5p_hdmi_ctrl_private = {
	.vendor		= "SAMSUNG",
	.product	= "S5PC210",

	.blue_screen = {
		.enable = false,
		.cb_b	= 0,
		.y_g	= 0,
		.cr_r	= 0,
	},

	.video = {
		.color_r = {
			.y_min = 1,
			.y_max = 254,
			.c_min = 1,
			.c_max = 254,
		},
		.depth	= HDMI_CD_24,
	},

	.packet = {
		.vsi_info = {0x81, 0x1, 27},
		.avi_info = {0x82, 0x2, 13},
		.spd_info = {0x83, 0x1, 27},
		.aui_info = {0x84, 0x1, 0x0a},
		.mpg_info = {0x85, 0x1, 5},
	},

	.tg = {
		.correction_en	= false,
		.bt656_en	= false,
	},

	.hdcp_en = false,

	.audio = {
		.type	= HDMI_60958_AUDIO,
		.bit	= 16,
		.freq	= 44100,
	},

	.av_mute	= false,
	.running	= false,

	.pow_name = "hdmi_pd",

	.clk[HDMI_PCLK] = {
		.name = "hdmi",
		.ptr = NULL
	},

	.clk[HDMI_MUX] = {
		.name = "sclk_hdmi",
		.ptr = NULL
	},

	.reg_mem[HDMI] = {
		.name = "s5p-hdmi",
		.res = NULL,
		.base = NULL
	},

	.reg_mem[HDMI_PHY] = {
		.name = "s5p-i2c-hdmi-phy",
		.res = NULL,
		.base = NULL
	},

	.irq = {
		.name = "s5p-hdmi",
		.handler = s5p_hdmi_irq,
		.no = -1
	}

};









/****************************************
 * Definitions for tvif ctrl class
 ***************************************/
struct s5p_tvif_ctrl_private_data {
	enum s5p_tvout_disp_mode	curr_std;
	enum s5p_tvout_o_mode		curr_if;

	bool				running;
};

static struct s5p_tvif_ctrl_private_data s5p_tvif_ctrl_private = {
	.curr_std = TVOUT_INIT_DISP_VALUE,
	.curr_if = TVOUT_INIT_O_VALUE,

	.running = false
};







/****************************************
 * Functions for sdo ctrl class
 ***************************************/

static void s5p_sdo_ctrl_init_private(void)
{
}

static int s5p_sdo_ctrl_set_reg(enum s5p_tvout_disp_mode disp_mode)
{
	struct s5p_sdo_ctrl_private_data *private = &s5p_sdo_ctrl_private;

	s5p_sdo_sw_reset(1);

	if (s5p_sdo_set_display_mode(disp_mode, SDO_O_ORDER_COMPOSITE_Y_C_CVBS))
		return -1;

	if (s5p_sdo_set_video_scale_cfg(
		private->video_scale_cfg.composite_level,
		private->video_scale_cfg.composite_ratio))
		return -1;

	if (s5p_sdo_set_vbi(
		private->vbi.wss_cvbs, private->vbi.caption_cvbs))
		return -1;

	s5p_sdo_set_offset_gain(
		private->offset_gain.offset, private->offset_gain.gain);

	s5p_sdo_set_delay(
		private->delay.delay_y,
		private->delay.offset_video_start,
		private->delay.offset_video_end);

	s5p_sdo_set_schlock(private->color_sub_carrier_phase_adj);

	s5p_sdo_set_brightness_hue_saturation(private->bri_hue_sat);

	s5p_sdo_set_cvbs_color_compensation(private->cvbs_compen);

	s5p_sdo_set_component_porch(
		private->compo_porch.back_525,
		private->compo_porch.front_525,
		private->compo_porch.back_625,
		private->compo_porch.front_625);

	s5p_sdo_set_ch_xtalk_cancel_coef(
		private->xtalk_cc.coeff2, private->xtalk_cc.coeff1);

	s5p_sdo_set_closed_caption(
		private->closed_cap.display_cc,
		private->closed_cap.nondisplay_cc);

	if (s5p_sdo_set_wss525_data(private->wss_525))
		return -1;

	if (s5p_sdo_set_wss625_data(private->wss_625))
		return -1;

	if (s5p_sdo_set_cgmsa525_data(private->cgms_525))
		return -1;

	if (s5p_sdo_set_cgmsa625_data(private->cgms_625))
		return -1;

	s5p_sdo_set_interrupt_enable(0);

	s5p_sdo_clear_interrupt_pending();

	s5p_sdo_clock_on(1);
	s5p_sdo_dac_on(1);

	return 0;
}

static void s5p_sdo_ctrl_internal_stop(void)
{
	s5p_sdo_clock_on(0);
	s5p_sdo_dac_on(0);
}

static void s5p_sdo_ctrl_clock(bool on)
{
	if (on) {
		clk_enable(s5p_sdo_ctrl_private.clk[SDO_MUX].ptr);

#ifdef CONFIG_CPU_S5PV210
		s5pv210_pd_enable(s5p_sdo_ctrl_private.pow_name);
#endif

#ifdef CONFIG_CPU_S5PV310
		s5p_tvout_pm_runtime_get();
#endif

		clk_enable(s5p_sdo_ctrl_private.clk[SDO_PCLK].ptr);
	} else {
		clk_disable(s5p_sdo_ctrl_private.clk[SDO_PCLK].ptr);

#ifdef CONFIG_CPU_S5PV210
		s5pv210_pd_disable(s5p_sdo_ctrl_private.pow_name);
#endif

#ifdef CONFIG_CPU_S5PV310
		s5p_tvout_pm_runtime_put();
#endif

		clk_disable(s5p_sdo_ctrl_private.clk[SDO_MUX].ptr);
	}

	mdelay(50);
}

void s5p_sdo_ctrl_stop(void)
{
	if (s5p_sdo_ctrl_private.running) {
		s5p_sdo_ctrl_internal_stop();
		s5p_sdo_ctrl_clock(0);

		s5p_sdo_ctrl_private.running = false;
	}
}

int s5p_sdo_ctrl_start(enum s5p_tvout_disp_mode disp_mode)
{
	struct s5p_sdo_ctrl_private_data *sdo_private = &s5p_sdo_ctrl_private;

	switch (disp_mode) {
	case TVOUT_NTSC_M:
	case TVOUT_NTSC_443:
		sdo_private->video_scale_cfg.composite_level =
			SDO_LEVEL_75IRE;
		sdo_private->video_scale_cfg.composite_ratio =
			SDO_VTOS_RATIO_10_4;
		break;

	case TVOUT_PAL_BDGHI:
	case TVOUT_PAL_M:
	case TVOUT_PAL_N:
	case TVOUT_PAL_NC:
	case TVOUT_PAL_60:
		sdo_private->video_scale_cfg.composite_level =
			SDO_LEVEL_0IRE;
		sdo_private->video_scale_cfg.composite_ratio =
			SDO_VTOS_RATIO_7_3;
		break;

	default:
		tvout_err("invalid disp_mode(%d) for SDO\n",
			disp_mode);
		goto err_on_s5p_sdo_start;
	}

	if (sdo_private->running)
		s5p_sdo_ctrl_internal_stop();
	else {
		s5p_sdo_ctrl_clock(1);

		sdo_private->running = true;
	}

	if (s5p_sdo_ctrl_set_reg(disp_mode))
		goto err_on_s5p_sdo_start;

	return 0;

err_on_s5p_sdo_start:
	return -1;
}

int s5p_sdo_ctrl_constructor(struct platform_device *pdev)
{
	int ret;
	int i, j;

	ret = s5p_tvout_map_resource_mem(
		pdev,
		s5p_sdo_ctrl_private.reg_mem.name,
		&(s5p_sdo_ctrl_private.reg_mem.base),
		&(s5p_sdo_ctrl_private.reg_mem.res));

	if (ret)
		goto err_on_res;

	for (i = SDO_PCLK; i < SDO_NO_OF_CLK; i++) {
		s5p_sdo_ctrl_private.clk[i].ptr =
			clk_get(&pdev->dev, s5p_sdo_ctrl_private.clk[i].name);

		if (IS_ERR(s5p_sdo_ctrl_private.clk[i].ptr)) {
			tvout_err("Failed to find clock %s\n",
					s5p_sdo_ctrl_private.clk[i].name);
			ret = -ENOENT;
			goto err_on_clk;
		}
	}

	s5p_sdo_ctrl_init_private();
	s5p_sdo_init(s5p_sdo_ctrl_private.reg_mem.base);

	return 0;

err_on_clk:
	for (j = 0; j < i; j++)
		clk_put(s5p_sdo_ctrl_private.clk[j].ptr);

	s5p_tvout_unmap_resource_mem(
		s5p_sdo_ctrl_private.reg_mem.base,
		s5p_sdo_ctrl_private.reg_mem.res);

err_on_res:
	return ret;
}

void s5p_sdo_ctrl_destructor(void)
{
	int i;

	s5p_tvout_unmap_resource_mem(
		s5p_sdo_ctrl_private.reg_mem.base,
		s5p_sdo_ctrl_private.reg_mem.res);

	for (i = SDO_PCLK; i < SDO_NO_OF_CLK; i++)
		if (s5p_sdo_ctrl_private.clk[i].ptr) {
			if (s5p_sdo_ctrl_private.running)
				clk_disable(s5p_sdo_ctrl_private.clk[i].ptr);
			clk_put(s5p_sdo_ctrl_private.clk[i].ptr);
	}
}






/****************************************
 * Functions for hdmi ctrl class
 ***************************************/

static enum s5p_hdmi_v_mode s5p_hdmi_check_v_fmt(enum s5p_tvout_disp_mode disp)
{
	struct s5p_hdmi_ctrl_private_data	*ctrl = &s5p_hdmi_ctrl_private;
	struct s5p_hdmi_video			*video = &ctrl->video;
	enum s5p_hdmi_v_mode			mode;

	video->aspect		= HDMI_PIC_RATIO_16_9;
	video->colorimetry	= HDMI_CLRIMETRY_601;

	switch (disp) {
	case TVOUT_480P_60_16_9:
		mode = v720x480p_60Hz;
		break;

	case TVOUT_480P_60_4_3:
		mode = v720x480p_60Hz;
		video->aspect = HDMI_PIC_RATIO_4_3;
		break;

	case TVOUT_480P_59:
		mode = v720x480p_59Hz;
		break;

	case TVOUT_576P_50_16_9:
		mode = v720x576p_50Hz;
		break;

	case TVOUT_576P_50_4_3:
		mode = v720x576p_50Hz;
		video->aspect = HDMI_PIC_RATIO_4_3;
		break;

	case TVOUT_720P_60:
		mode = v1280x720p_60Hz;
		video->colorimetry = HDMI_CLRIMETRY_709;
		break;

	case TVOUT_720P_59:
		mode = v1280x720p_59Hz;
		video->colorimetry = HDMI_CLRIMETRY_709;
		break;

	case TVOUT_720P_50:
		mode = v1280x720p_50Hz;
		video->colorimetry = HDMI_CLRIMETRY_709;
		break;

	case TVOUT_1080P_30:
		mode = v1920x1080p_30Hz;
		video->colorimetry = HDMI_CLRIMETRY_709;
		break;

	case TVOUT_1080P_60:
		mode = v1920x1080p_60Hz;
		video->colorimetry = HDMI_CLRIMETRY_709;
		break;

	case TVOUT_1080P_59:
		mode = v1920x1080p_59Hz;
		video->colorimetry = HDMI_CLRIMETRY_709;
		break;

	case TVOUT_1080P_50:
		mode = v1920x1080p_50Hz;
		video->colorimetry = HDMI_CLRIMETRY_709;
		break;

	case TVOUT_1080I_60:
		mode = v1920x1080i_60Hz;
		video->colorimetry = HDMI_CLRIMETRY_709;
		break;

	case TVOUT_1080I_59:
		mode = v1920x1080i_59Hz;
		video->colorimetry = HDMI_CLRIMETRY_709;
		break;

	case TVOUT_1080I_50:
		mode = v1920x1080i_50Hz;
		video->colorimetry = HDMI_CLRIMETRY_709;
		break;

	default:
		mode = v720x480p_60Hz;
		tvout_err("Not supported mode : %d\n", mode);
	}

	return mode;
}

static void s5p_hdmi_set_acr(struct s5p_hdmi_audio *audio, u8 *acr)
{
	u32 n = (audio->freq == 32000) ? 4096 :
		(audio->freq == 44100) ? 6272 :
		(audio->freq == 88200) ? 12544 :
		(audio->freq == 176400) ? 25088 :
		(audio->freq == 48000) ? 6144 :
		(audio->freq == 96000) ? 12288 :
		(audio->freq == 192000) ? 24576 : 0;

	u32 cts = (audio->freq == 32000) ? 27000 :
		(audio->freq == 44100) ? 30000 :
		(audio->freq == 88200) ? 30000 :
		(audio->freq == 176400) ? 30000 :
		(audio->freq == 48000) ? 27000 :
		(audio->freq == 96000) ? 27000 :
		(audio->freq == 192000) ? 27000 : 0;

	acr[1] = cts >> 16;
	acr[2] = cts >> 8 & 0xff;
	acr[3] = cts & 0xff;

	acr[4] = n >> 16;
	acr[5] = n >> 8 & 0xff;
	acr[6] = n & 0xff;

	tvout_dbg("n value = %d\n", n);
	tvout_dbg("cts   = %d\n", cts);
}

static void s5p_hdmi_set_asp(u8 *header)
{
	header[1] = 0;
	header[2] = 0;
}

static void s5p_hdmi_set_acp(struct s5p_hdmi_audio *audio, u8 *header)
{
	header[1] = audio->type;
}

static void s5p_hdmi_set_isrc(u8 *header)
{
}

static void s5p_hdmi_set_gmp(u8 *gmp)
{
}

static void s5p_hdmi_set_avi(
	enum s5p_hdmi_v_mode mode, enum s5p_tvout_o_mode out,
	struct s5p_hdmi_video *video, u8 *avi)
{
	struct s5p_hdmi_o_params		param = s5p_hdmi_output[out];
	struct s5p_hdmi_v_frame			frame;

	frame = s5p_hdmi_v_fmt[mode].frame;

	avi[0] = param.reg.pxl_fmt;
	avi[0] |= (0x1 << 4);

	avi[1] = video->colorimetry;
	avi[1] |= video->aspect << 4;
	avi[1] |= AVI_SAME_WITH_PICTURE_AR;

	avi[3] = (video->aspect == HDMI_PIC_RATIO_16_9) ?
				frame.vic_16_9 : frame.vic;

	avi[4] = frame.repetition;
}

static void s5p_hdmi_set_aui(struct s5p_hdmi_audio *audio, u8 *aui)
{
	aui[0] = (0 << 4) | audio->channel;
	aui[1] = ((audio->type == HDMI_60958_AUDIO) ? 0 : audio->freq << 2) | 0;
	aui[2] = 0;
}

static void s5p_hdmi_set_spd(u8 *spd)
{
	struct s5p_hdmi_ctrl_private_data *ctrl = &s5p_hdmi_ctrl_private;

	memcpy(spd, ctrl->vendor, 8);
	memcpy(&spd[8], ctrl->product, 16);

	spd[24] = 0x1; /* Digital STB */
}

static void s5p_hdmi_set_mpg(u8 *mpg)
{
}

static int s5p_hdmi_ctrl_audio_enable(bool en)
{
	if (!s5p_hdmi_output[s5p_hdmi_ctrl_private.out].reg.dvi)
		s5p_hdmi_reg_audio_enable(en);

	return 0;
}

#if 0 /* This function will be used in the future */
static void s5p_hdmi_ctrl_bluescreen_clr(u8 cb_b, u8 y_g, u8 cr_r)
{
	struct s5p_hdmi_ctrl_private_data *ctrl = &s5p_hdmi_ctrl_private;

	ctrl->blue_screen.cb_b	= cb_b;
	ctrl->blue_screen.y_g	= y_g;
	ctrl->blue_screen.cr_r	= cr_r;

	s5p_hdmi_reg_bluescreen_clr(cb_b, y_g, cr_r);
}
#endif

static void s5p_hdmi_ctrl_set_bluescreen(bool en)
{
	struct s5p_hdmi_ctrl_private_data	*ctrl = &s5p_hdmi_ctrl_private;

	ctrl->blue_screen.enable = en ? true : false;

	s5p_hdmi_reg_bluescreen(en);
}

static void s5p_hdmi_ctrl_set_audio(bool en)
{
	struct s5p_hdmi_ctrl_private_data       *ctrl = &s5p_hdmi_ctrl_private;

	s5p_hdmi_ctrl_private.audio.on = en ? 1 : 0;

	if (ctrl->running)
		s5p_hdmi_ctrl_audio_enable(en);
}

static void s5p_hdmi_ctrl_set_av_mute(bool en)
{
	struct s5p_hdmi_ctrl_private_data       *ctrl = &s5p_hdmi_ctrl_private;

	ctrl->av_mute = en ? 1 : 0;

	if (ctrl->running) {
		if (en) {
			s5p_hdmi_ctrl_audio_enable(false);
			s5p_hdmi_ctrl_set_bluescreen(true);
		} else {
			s5p_hdmi_ctrl_audio_enable(true);
			s5p_hdmi_ctrl_set_bluescreen(false);
		}
	}

}

u8 s5p_hdmi_ctrl_get_mute(void)
{
	return s5p_hdmi_ctrl_private.av_mute ? 1 : 0;
}

#if 0 /* This function will be used in the future */
static void s5p_hdmi_ctrl_mute(bool en)
{
	struct s5p_hdmi_ctrl_private_data	*ctrl = &s5p_hdmi_ctrl_private;

	if (en) {
		s5p_hdmi_reg_bluescreen(true);
		s5p_hdmi_ctrl_audio_enable(false);
	} else {
		s5p_hdmi_reg_bluescreen(false);
		if (ctrl->audio.on)
			s5p_hdmi_ctrl_audio_enable(true);
	}
}
#endif

void s5p_hdmi_ctrl_set_hdcp(bool en)
{
	struct s5p_hdmi_ctrl_private_data	*ctrl = &s5p_hdmi_ctrl_private;

	ctrl->hdcp_en = en ? 1 : 0;
}

static void s5p_hdmi_ctrl_init_private(void)
{
}

static bool s5p_hdmi_ctrl_set_reg(
		enum s5p_hdmi_v_mode mode, enum s5p_tvout_o_mode out)
{
	struct s5p_hdmi_ctrl_private_data	*ctrl = &s5p_hdmi_ctrl_private;
	struct s5p_hdmi_packet			*packet = &ctrl->packet;

	struct s5p_hdmi_bluescreen		*bl = &ctrl->blue_screen;
	struct s5p_hdmi_color_range		*cr = &ctrl->video.color_r;
	struct s5p_hdmi_tg			*tg = &ctrl->tg;

	s5p_hdmi_reg_bluescreen_clr(bl->cb_b, bl->y_g, bl->cr_r);
	s5p_hdmi_reg_bluescreen(bl->enable);

	s5p_hdmi_reg_clr_range(cr->y_min, cr->y_max, cr->c_min, cr->c_max);

	s5p_hdmi_reg_acr(packet->acr);
	s5p_hdmi_reg_asp(packet->h_asp);
	s5p_hdmi_reg_gcp(s5p_hdmi_v_fmt[mode].frame.i_p, packet->gcp);

	s5p_hdmi_reg_acp(packet->h_acp, packet->acp);
	s5p_hdmi_reg_isrc(packet->isrc1, packet->isrc2);
	s5p_hdmi_reg_gmp(packet->gmp);

	s5p_hdmi_reg_infoframe(&packet->avi_info, packet->avi);
	s5p_hdmi_reg_infoframe(&packet->aui_info, packet->aui);
	s5p_hdmi_reg_infoframe(&packet->spd_info, packet->spd);
	s5p_hdmi_reg_infoframe(&packet->mpg_info, packet->mpg);

	s5p_hdmi_reg_packet_trans(&s5p_hdmi_output[out].trans);
	s5p_hdmi_reg_output(&s5p_hdmi_output[out].reg);

	s5p_hdmi_reg_tg(&s5p_hdmi_v_fmt[mode].frame);
	s5p_hdmi_reg_v_timing(&s5p_hdmi_v_fmt[mode]);
	s5p_hdmi_reg_tg_cmd(tg->correction_en, tg->bt656_en, true);

	switch (ctrl->audio.type) {
	case HDMI_GENERIC_AUDIO:
		break;

	case HDMI_60958_AUDIO:
		s5p_hdmi_audio_init(PCM, 44100, 16, 0);
		break;

	case HDMI_DVD_AUDIO:
	case HDMI_SUPER_AUDIO:
		break;

	default:
		tvout_err("Invalid audio type %d\n", ctrl->audio.type);
		return -1;
	}

	s5p_hdmi_reg_audio_enable(true);

	return 0;
}

static void s5p_hdmi_ctrl_internal_stop(void)
{
	struct s5p_hdmi_ctrl_private_data	*ctrl = &s5p_hdmi_ctrl_private;
	struct s5p_hdmi_tg			*tg = &ctrl->tg;

	tvout_dbg("\n");
#ifdef CONFIG_HDMI_HPD
	s5p_hpd_set_eint();
#endif
	if (ctrl->hdcp_en)
		s5p_hdcp_stop();

	mdelay(10);

	s5p_hdmi_reg_enable(false);

	s5p_hdmi_reg_tg_cmd(tg->correction_en, tg->bt656_en, false);
}

int s5p_hdmi_ctrl_phy_power(bool on)
{
	tvout_dbg("on(%d)\n", on);
	if (on) {
		/* on */
		clk_enable(s5ptv_status.i2c_phy_clk);

		s5p_hdmi_phy_power(true);


	} else {
		/*
		 * for preventing hdmi hang up when restart
		 * switch to internal clk - SCLK_DAC, SCLK_PIXEL
		 */
		s5p_mixer_ctrl_mux_clk(s5ptv_status.sclk_dac);
		clk_set_parent(s5ptv_status.sclk_hdmi,
					s5ptv_status.sclk_pixel);

		s5p_hdmi_phy_power(false);

		clk_disable(s5ptv_status.i2c_phy_clk);
	}

	return 0;
}

void s5p_hdmi_ctrl_clock(bool on)
{
	struct s5p_hdmi_ctrl_private_data	*ctrl = &s5p_hdmi_ctrl_private;
	struct s5p_tvout_clk_info		*clk = ctrl->clk;

	tvout_dbg("on(%d)\n", on);
	if (on) {
		clk_enable(clk[HDMI_MUX].ptr);

#ifdef CONFIG_CPU_S5PV210
		s5pv210_pd_enable(ctrl->pow_name);
#endif

#ifdef CONFIG_CPU_S5PV310
		s5p_tvout_pm_runtime_get();
#endif

		clk_enable(clk[HDMI_PCLK].ptr);
	} else {
		clk_disable(clk[HDMI_PCLK].ptr);

#ifdef CONFIG_CPU_S5PV210
		s5pv210_pd_disable(ctrl->pow_name);
#endif

#ifdef CONFIG_CPU_S5PV310
		s5p_tvout_pm_runtime_put();
#endif

		clk_disable(clk[HDMI_MUX].ptr);
	}

	mdelay(50);
}

bool s5p_hdmi_ctrl_status(void)
{
	return s5p_hdmi_ctrl_private.running;
}

void s5p_hdmi_ctrl_stop(void)
{
	struct s5p_hdmi_ctrl_private_data	*ctrl = &s5p_hdmi_ctrl_private;

	tvout_dbg("running(%d)\n", ctrl->running);
	if (ctrl->running) {
		ctrl->running = false;
#ifdef CONFIG_HAS_EARLYSUSPEND
		if (suspend_status) {
			tvout_dbg("driver is suspend_status\n");
		} else
#endif
		{
			s5p_hdmi_ctrl_internal_stop();
			s5p_hdmi_ctrl_clock(0);
		}
	}
}

int s5p_hdmi_ctrl_start(
	enum s5p_tvout_disp_mode disp, enum s5p_tvout_o_mode out)
{
	struct s5p_hdmi_ctrl_private_data	*ctrl = &s5p_hdmi_ctrl_private;
	struct s5p_hdmi_packet			*packet = &ctrl->packet;
	struct s5p_hdmi_v_frame			frame;

	enum s5p_hdmi_v_mode			mode;

	ctrl->out = out;
	mode = s5p_hdmi_check_v_fmt(disp);
	ctrl->mode = mode;

	tvout_dbg("\n");
	if (ctrl->running)
		s5p_hdmi_ctrl_internal_stop();
	else {
		s5p_hdmi_ctrl_clock(1);
		ctrl->running = true;
	}
	on_start_process = false;
	tvout_dbg("on_start_process(%d)\n", on_start_process);

	frame = s5p_hdmi_v_fmt[mode].frame;

	if (s5p_hdmi_phy_config(frame.pixel_clock, ctrl->video.depth) < 0) {
		tvout_err("hdmi phy configuration failed.\n");
		goto err_on_s5p_hdmi_start;
	}


	s5p_hdmi_set_acr(&ctrl->audio, packet->acr);
	s5p_hdmi_set_asp(packet->h_asp);
	s5p_hdmi_set_gcp(ctrl->video.depth, packet->gcp);

	s5p_hdmi_set_acp(&ctrl->audio, packet->h_acp);
	s5p_hdmi_set_isrc(packet->h_isrc);
	s5p_hdmi_set_gmp(packet->gmp);

	s5p_hdmi_set_avi(mode, out, &ctrl->video, packet->avi);
	s5p_hdmi_set_spd(packet->spd);
	s5p_hdmi_set_aui(&ctrl->audio, packet->aui);
	s5p_hdmi_set_mpg(packet->mpg);

	s5p_hdmi_ctrl_set_reg(mode, out);

	s5p_hdmi_reg_enable(true);
//JJW  HDMI  Rotate  Issue 
#ifdef CONFIG_HDMI_HPD
		s5p_hpd_set_hdmiint();
#endif
//JJW End. 

	if (ctrl->hdcp_en)
		s5p_hdcp_start();

	return 0;

err_on_s5p_hdmi_start:
	return -1;
}

int s5p_hdmi_ctrl_constructor(struct platform_device *pdev)
{
	struct s5p_hdmi_ctrl_private_data	*ctrl = &s5p_hdmi_ctrl_private;
	struct reg_mem_info		*reg_mem = ctrl->reg_mem;
	struct s5p_tvout_clk_info		*clk = ctrl->clk;
	struct irq_info			*irq = &ctrl->irq;
	int ret, i, k, j;

	for (i = 0; i < HDMI_NO_OF_MEM_RES; i++) {
		ret = s5p_tvout_map_resource_mem(pdev, reg_mem[i].name,
			&(reg_mem[i].base), &(reg_mem[i].res));

		if (ret)
			goto err_on_res;
	}

	for (k = HDMI_PCLK; k < HDMI_NO_OF_CLK; k++) {
		clk[k].ptr = clk_get(&pdev->dev, clk[k].name);

		if (IS_ERR(clk[k].ptr)) {
			printk(KERN_ERR	"%s clk is not found\n", clk[k].name);
			ret = -ENOENT;
			goto err_on_clk;
		}
	}

	irq->no = platform_get_irq_byname(pdev, irq->name);

	if (irq->no < 0) {
		printk(KERN_ERR "can not get platform irq by name : %s\n",
					irq->name);
		ret = irq->no;
		goto err_on_irq;
	}

	ret = request_irq(irq->no, irq->handler, IRQF_DISABLED,
			irq->name, NULL);
	if (ret) {
		printk(KERN_ERR "can not request irq : %s\n", irq->name);
		goto err_on_irq;
	}

	s5p_hdmi_ctrl_init_private();
	s5p_hdmi_init(reg_mem[HDMI].base, reg_mem[HDMI_PHY].base);

#if defined(CONFIG_ARCH_S5PV310)
	/* set initial state of HDMI PHY power to off */
	s5p_hdmi_ctrl_phy_power(1);
	s5p_hdmi_ctrl_phy_power(0);
#endif

	s5p_hdcp_init();

	return 0;

err_on_irq:
err_on_clk:
	for (j = 0; j < k; j++)
		clk_put(clk[j].ptr);

err_on_res:
	for (j = 0; j < i; j++)
		s5p_tvout_unmap_resource_mem(reg_mem[j].base, reg_mem[j].res);

	return ret;
}

void s5p_hdmi_ctrl_destructor(void)
{
	struct s5p_hdmi_ctrl_private_data	*ctrl = &s5p_hdmi_ctrl_private;
	struct reg_mem_info		*reg_mem = ctrl->reg_mem;
	struct s5p_tvout_clk_info		*clk = ctrl->clk;
	struct irq_info			*irq = &ctrl->irq;

	int i;

	if (irq->no >= 0)
		free_irq(irq->no, NULL);

	for (i = 0; i < HDMI_NO_OF_MEM_RES; i++)
		s5p_tvout_unmap_resource_mem(reg_mem[i].base, reg_mem[i].res);

	for (i = HDMI_PCLK; i < HDMI_NO_OF_CLK; i++)
		if (clk[i].ptr) {
			if (ctrl->running)
				clk_disable(clk[i].ptr);
			clk_put(clk[i].ptr);
		}
}

void s5p_hdmi_ctrl_suspend(void)
{
}

void s5p_hdmi_ctrl_resume(void)
{
}









/****************************************
 * Functions for tvif ctrl class
 ***************************************/
static void s5p_tvif_ctrl_init_private(void)
{
}

/*
 * TV cut off sequence
 * VP stop -> Mixer stop -> HDMI stop -> HDMI TG stop
 * Above sequence should be satisfied.
 */
static int s5p_tvif_ctrl_internal_stop(void)
{
	tvout_dbg("\n");
	s5p_mixer_ctrl_stop();

	switch (s5p_tvif_ctrl_private.curr_if) {
	case TVOUT_COMPOSITE:
		s5p_sdo_ctrl_stop();
		break;

	case TVOUT_DVI:
	case TVOUT_HDMI:
	case TVOUT_HDMI_RGB:
		s5p_hdmi_ctrl_stop();
#ifdef CONFIG_HAS_EARLYSUSPEND
		if (suspend_status) {
			tvout_dbg("driver is suspend_status\n");
		} else
#endif
		{
			s5p_hdmi_ctrl_phy_power(0);
		}
		break;

	default:
		tvout_err("invalid out parameter(%d)\n",
			s5p_tvif_ctrl_private.curr_if);
		return -1;
	}

	return 0;
}

static void s5p_tvif_ctrl_internal_start(
		enum s5p_tvout_disp_mode std,
		enum s5p_tvout_o_mode inf)
{
	tvout_dbg("\n");
	s5p_mixer_ctrl_set_int_enable(false);

	/* Clear All Interrupt Pending */
	s5p_mixer_ctrl_clear_pend_all();

	switch (inf) {
	case TVOUT_COMPOSITE:
		if (s5p_mixer_ctrl_start(std, inf) < 0)
			goto ret_on_err;

		if (0 != s5p_sdo_ctrl_start(std))
			goto ret_on_err;

		break;

	case TVOUT_HDMI:
	case TVOUT_HDMI_RGB:
	case TVOUT_DVI:
		s5p_hdmi_ctrl_phy_power(1);

		if (s5p_mixer_ctrl_start(std, inf) < 0)
			goto ret_on_err;

		if (0 != s5p_hdmi_ctrl_start(std, inf))
			goto ret_on_err;
		break;
	default:
		break;
	}

ret_on_err:
	s5p_mixer_ctrl_set_int_enable(true);

	/* Clear All Interrupt Pending */
	s5p_mixer_ctrl_clear_pend_all();
}

int s5p_tvif_ctrl_set_audio(bool en)
{
	switch (s5p_tvif_ctrl_private.curr_if) {
	case TVOUT_HDMI:
	case TVOUT_HDMI_RGB:
	case TVOUT_DVI:
		s5p_hdmi_ctrl_set_audio(en);

		break;
	default:
		return -EINVAL;
	}

	return 0;
}

int s5p_tvif_ctrl_set_av_mute(bool en)
{
	switch (s5p_tvif_ctrl_private.curr_if) {
	case TVOUT_HDMI:
	case TVOUT_HDMI_RGB:
	case TVOUT_DVI:
		s5p_hdmi_ctrl_set_av_mute(en);

		break;
	default:
		return -EINVAL;
	}

	return 0;
}

int s5p_tvif_ctrl_get_std_if(
		enum s5p_tvout_disp_mode *std, enum s5p_tvout_o_mode *inf)
{
	*std = s5p_tvif_ctrl_private.curr_std;
	*inf = s5p_tvif_ctrl_private.curr_if;

	return 0;
}

bool s5p_tvif_ctrl_get_run_state()
{
	return s5p_tvif_ctrl_private.running;
}

int s5p_tvif_ctrl_start(
		enum s5p_tvout_disp_mode std, enum s5p_tvout_o_mode inf)
{
	tvout_dbg("\n");
	if (s5p_tvif_ctrl_private.running &&
		(std == s5p_tvif_ctrl_private.curr_std) &&
		(inf == s5p_tvif_ctrl_private.curr_if))
	{
		on_start_process = false;
		tvout_dbg("on_start_process(%d)\n", on_start_process);
		goto cannot_change;
	}

	switch (inf) {
	case TVOUT_COMPOSITE:
	case TVOUT_HDMI:
	case TVOUT_HDMI_RGB:
	case TVOUT_DVI:
		break;
	default:
		tvout_err("invalid out parameter(%d)\n", inf);
		goto cannot_change;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	if (suspend_status) {
		tvout_dbg("driver is suspend_status\n");
	} else
#endif
	{
		/* how to control the clock path on stop time ??? */
		if (s5p_tvif_ctrl_private.running)
			s5p_tvif_ctrl_internal_stop();

		s5p_tvif_ctrl_internal_start(std, inf);
	}

	s5p_tvif_ctrl_private.running = true;
	s5p_tvif_ctrl_private.curr_std = std;
	s5p_tvif_ctrl_private.curr_if = inf;

	return 0;

cannot_change:
	return -1;
}

void s5p_tvif_ctrl_stop(void)
{
	if (s5p_tvif_ctrl_private.running) {
		s5p_tvif_ctrl_internal_stop();

		s5p_tvif_ctrl_private.running = false;
	}
}

int s5p_tvif_ctrl_constructor(struct platform_device *pdev)
{
	if (s5p_sdo_ctrl_constructor(pdev))
		goto err;

	if (s5p_hdmi_ctrl_constructor(pdev))
		goto err;

	s5p_tvif_ctrl_init_private();

	return 0;

err:
	return -1;
}

void s5p_tvif_ctrl_destructor(void)
{
	s5p_sdo_ctrl_destructor();
	s5p_hdmi_ctrl_destructor();
}

void s5p_tvif_ctrl_suspend(void)
{
	tvout_dbg("\n");
	if (s5p_tvif_ctrl_private.running) {
		s5p_tvif_ctrl_internal_stop();
#ifdef CONFIG_VCM
		s5p_tvout_vcm_deactivate();
#endif
	}

}

void s5p_tvif_ctrl_resume(void)
{
	pr_info("%s: running:%d\n", __func__, s5p_tvif_ctrl_private.running);

	if (s5p_tvif_ctrl_private.running) {
#ifdef CONFIG_VCM
		s5p_tvout_vcm_activate();
#endif
		s5p_tvif_ctrl_internal_start(
			s5p_tvif_ctrl_private.curr_std,
			s5p_tvif_ctrl_private.curr_if);
	}
}

#ifdef CONFIG_PM
void s5p_hdmi_ctrl_phy_power_resume(void)
{
	tvout_dbg("running(%d)\n", s5p_tvif_ctrl_private.running);
	if (s5p_tvif_ctrl_private.running)
		return;

	s5p_hdmi_ctrl_phy_power(1);
	s5p_hdmi_ctrl_phy_power(0);

	return;
}
#endif
