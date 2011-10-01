/* linux/arch/arm/mach-s5pv310/clock.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S5PV310 - Clock support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/io.h>

#include <plat/cpu-freq.h>
#include <plat/clock.h>
#include <plat/cpu.h>
#include <plat/pll.h>
#include <plat/devs.h>
#include <plat/s5p-clock.h>
#include <plat/clock-clksrc.h>

#include <mach/map.h>
#include <mach/regs-clock.h>
#include <mach/regs-audss.h>

static struct clk clk_sclk_hdmi27m = {
	.name           = "sclk_hdmi27m",
	.id             = -1,
	.rate           = 27000000,
};

static struct clk clk_sclk_hdmiphy = {
	.name		= "sclk_hdmiphy",
	.id		= -1,
};

static struct clk clk_sclk_usbphy0 = {
	.name		= "sclk_usbphy0",
	.id		= -1,
};

static struct clk clk_sclk_usbphy1 = {
	.name		= "sclk_usbphy1",
	.id		= -1,
};

static struct clk clk_sclk_xxti = {
	.name		= "sclk_usbphy1",
	.id		= -1,
};

static struct clk clk_sclk_xusbxti = {
	.name		= "sclk_usbphy1",
	.id		= -1,
};

static struct clk clk_audiocdclk0 = {
	.name		= "audiocdclk",
	.id		= 0,
};

static struct clk clk_audiocdclk1 = {
	.name		= "audiocdclk",
	.id		= 0,
};

static struct clk clk_audiocdclk2 = {
	.name		= "audiocdclk",
	.id		= -1,
};

static struct clk clk_spdifcdclk = {
	.name		= "spdifcdclk",
	.id		= -1,
};

static int s5pv310_clk_ip_leftbus_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_IP_LEFTBUS, clk, enable);
}

static int s5pv310_clk_ip_rightbus_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_IP_RIGHTBUS, clk, enable);
}

static int s5pv310_clk_ip_cam_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_IP_CAM, clk, enable);
}

static int s5pv310_clk_ip_tv_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_IP_TV, clk, enable);
}

static int s5pv310_clk_ip_mfc_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_IP_MFC, clk, enable);
}

static int s5pv310_clk_ip_g3d_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_IP_G3D, clk, enable);
}

static int s5pv310_clk_ip_image_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_IP_IMAGE, clk, enable);
}

static int s5pv310_clk_ip_lcd0_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_IP_LCD0, clk, enable);
}

static int s5pv310_clk_ip_lcd1_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_IP_LCD1, clk, enable);
}

static int s5pv310_clk_ip_fsys_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_IP_FSYS, clk, enable);
}

static int s5pv310_clk_ip_gps_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_IP_GPS, clk, enable);
}

static int s5pv310_clk_ip_peril_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_IP_PERIL, clk, enable);
}

static int s5pv310_clk_ip_perir_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_IP_PERIR, clk, enable);
}

static int s5pv310_clk_ip_dmc_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_IP_DMC, clk, enable);
}

static int s5pv310_clk_audss_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_AUDSS, clk, enable);
}

static int s5pv310_clk_ip_cpu_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_IP_CPU, clk, enable);
}

static int s5pv310_clksrc_mask_top_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKSRC_MASK_TOP, clk, enable);
}

static int s5pv310_clksrc_mask_cam_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKSRC_MASK_CAM, clk, enable);
}

static int s5pv310_clksrc_mask_tv_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKSRC_MASK_TV, clk, enable);
}

static int s5pv310_clksrc_mask_lcd0_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKSRC_MASK_LCD0, clk, enable);
}

static int s5pv310_clksrc_mask_lcd1_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKSRC_MASK_LCD1, clk, enable);
}

static int s5pv310_clksrc_mask_maudio_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKSRC_MASK_MAUDIO, clk, enable);
}

static int s5pv310_clksrc_mask_fsys_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKSRC_MASK_FSYS, clk, enable);
}

static int s5pv310_clksrc_mask_peril0_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKSRC_MASK_PERIL0, clk, enable);
}

static int s5pv310_clksrc_mask_peril1_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKSRC_MASK_PERIL1, clk, enable);
}

static int s5pv310_clk_epll_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_EPLL_CON0, clk, enable);
}

static int s5pv310_clk_vpll_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_VPLL_CON0, clk, enable);
}


/* Core list of CMU_CPU side */

static struct clksrc_clk clk_mout_apll = {
	.clk	= {
		.name		= "mout_apll",
		.id		= -1,
	},
	.sources	= &clk_src_apll,
	.reg_src	= { .reg = S5P_CLKSRC_CPU, .shift = 0, .size = 1 },
};

static struct clksrc_clk clk_sclk_apll = {
	.clk	= {
		.name		= "sclk_apll",
		.id		= -1,
		.parent		= &clk_mout_apll.clk,
	},
	.reg_div	= { .reg = S5P_CLKDIV_CPU, .shift = 24, .size = 3 },
};

static struct clksrc_clk clk_mout_epll = {
	.clk	= {
		.name		= "mout_epll",
		.id		= -1,
		.parent = &clk_fout_epll,
	},
	.sources	= &clk_src_epll,
	.reg_src	= { .reg = S5P_CLKSRC_TOP0, .shift = 4, .size = 1 },
};

static struct clksrc_clk clk_mout_mpll = {
	.clk = {
		.name		= "mout_mpll",
		.id		= -1,
	},
	.sources	= &clk_src_mpll,
	.reg_src	= { .reg = S5P_CLKSRC_CPU, .shift = 8, .size = 1 },
};

static struct clk *clkset_moutcore_list[] = {
	[0] = &clk_mout_apll.clk,
	[1] = &clk_mout_mpll.clk,
};

static struct clksrc_sources clkset_moutcore = {
	.sources	= clkset_moutcore_list,
	.nr_sources	= ARRAY_SIZE(clkset_moutcore_list),
};

static struct clksrc_clk clk_moutcore = {
	.clk	= {
		.name		= "moutcore",
		.id		= -1,
	},
	.sources	= &clkset_moutcore,
	.reg_src	= { .reg = S5P_CLKSRC_CPU, .shift = 16, .size = 1 },
};

static struct clksrc_clk clk_coreclk = {
	.clk	= {
		.name		= "core_clk",
		.id		= -1,
		.parent		= &clk_moutcore.clk,
	},
	.reg_div	= { .reg = S5P_CLKDIV_CPU, .shift = 0, .size = 3 },
};

static struct clksrc_clk clk_armclk = {
	.clk	= {
		.name		= "armclk",
		.id		= -1,
		.parent		= &clk_coreclk.clk,
	},
};

/* Core list of CMU_CORE side */

static struct clk *clkset_corebus_list[] = {
	[0] = &clk_mout_mpll.clk,
	[1] = &clk_sclk_apll.clk,
};

static struct clksrc_sources clkset_mout_corebus = {
	.sources	= clkset_corebus_list,
	.nr_sources	= ARRAY_SIZE(clkset_corebus_list),
};

static struct clksrc_clk clk_mout_corebus = {
	.clk	= {
		.name		= "mout_corebus",
		.id		= -1,
	},
	.sources	= &clkset_mout_corebus,
	.reg_src	= { .reg = S5P_CLKSRC_DMC, .shift = 4, .size = 1 },
};

static struct clksrc_clk clk_sclk_dmc = {
	.clk	= {
		.name		= "sclk_dmc",
		.id		= -1,
		.parent		= &clk_mout_corebus.clk,
	},
	.reg_div	= { .reg = S5P_CLKDIV_DMC0, .shift = 12, .size = 3 },
};

static struct clksrc_clk clk_aclk_cored = {
	.clk	= {
		.name		= "aclk_cored",
		.id		= -1,
		.parent		= &clk_sclk_dmc.clk,
	},
	.reg_div	= { .reg = S5P_CLKDIV_DMC0, .shift = 16, .size = 3 },
};

static struct clksrc_clk clk_aclk_corep = {
	.clk	= {
		.name		= "aclk_corep",
		.id		= -1,
		.parent		= &clk_aclk_cored.clk,
	},
	.reg_div	= { .reg = S5P_CLKDIV_DMC0, .shift = 20, .size = 3 },
};

static struct clksrc_clk clk_aclk_acp = {
	.clk	= {
		.name		= "aclk_acp",
		.id		= -1,
		.parent		= &clk_mout_corebus.clk,
	},
	.reg_div	= { .reg = S5P_CLKDIV_DMC0, .shift = 0, .size = 3 },
};

static struct clksrc_clk clk_pclk_acp = {
	.clk	= {
		.name		= "pclk_acp",
		.id		= -1,
		.parent		= &clk_aclk_acp.clk,
	},
	.reg_div	= { .reg = S5P_CLKDIV_DMC0, .shift = 4, .size = 3 },
};

/* Core list of CMU_TOP side */

static struct clk *clkset_aclk_top_list[] = {
	[0] = &clk_mout_mpll.clk,
	[1] = &clk_sclk_apll.clk,
};

static struct clksrc_sources clkset_aclk = {
	.sources        = clkset_aclk_top_list,
	.nr_sources     = ARRAY_SIZE(clkset_aclk_top_list),
};

static struct clksrc_clk clk_aclk_200 = {
	.clk    = {
		.name           = "aclk_200",
		.id             = -1,
	},
	.sources        = &clkset_aclk,
	.reg_src        = { .reg = S5P_CLKSRC_TOP0, .shift = 12, .size = 1 },
	.reg_div	= { .reg = S5P_CLKDIV_TOP, .shift = 0, .size = 3 },
};

static struct clksrc_clk clk_aclk_100 = {
	.clk    = {
		.name           = "aclk_100",
		.id             = -1,
	},
	.sources        = &clkset_aclk,
	.reg_src        = { .reg = S5P_CLKSRC_TOP0, .shift = 16, .size = 1 },
	.reg_div	= { .reg = S5P_CLKDIV_TOP, .shift = 4, .size = 4 },
};

static struct clksrc_clk clk_aclk_160 = {
	.clk    = {
		.name           = "aclk_160",
		.id             = -1,
	},
	.sources        = &clkset_aclk,
	.reg_src        = { .reg = S5P_CLKSRC_TOP0, .shift = 20, .size = 1 },
	.reg_div	= { .reg = S5P_CLKDIV_TOP, .shift = 8, .size = 3 },
};

static struct clksrc_clk clk_aclk_133 = {
	.clk    = {
		.name           = "aclk_133",
		.id             = -1,
	},
	.sources        = &clkset_aclk,
	.reg_src        = { .reg = S5P_CLKSRC_TOP0, .shift = 24, .size = 1 },
	.reg_div	= { .reg = S5P_CLKDIV_TOP, .shift = 12, .size = 3 },
};

/* CMU_LEFT/RIGHTBUS side */
#if 0
static struct clk *clkset_aclk_lrbus_list[] = {
	[0] = &clk_mout_mpll.clk,
	[1] = &clk_sclk_apll.clk,
};

static struct clksrc_sources clkset_aclk_lrbus = {
	.sources	= clkset_aclk_lrbus_list,
	.nr_sources	= ARRAY_SIZE(clkset_aclk_lrbus_list),
};
#endif

static struct clksrc_clk clk_aclk_gdl = {
	.clk	= {
		.name		= "aclk_gdl",
		.id		= -1,
	},
	.sources	= &clkset_aclk,
	.reg_src	= { .reg = S5P_CLKSRC_LEFTBUS, .shift = 0, .size = 1 },
	.reg_div	= { .reg = S5P_CLKDIV_LEFTBUS, .shift = 0, .size = 3 },
};

static struct clksrc_clk clk_aclk_gdr = {
	.clk	= {
		.name		= "aclk_gdr",
		.id		= -1,
	},
	.sources	= &clkset_aclk,
	.reg_src	= { .reg = S5P_CLKSRC_RIGHTBUS, .shift = 0, .size = 1 },
	.reg_div	= { .reg = S5P_CLKDIV_RIGHTBUS, .shift = 0, .size = 3 },
};

static struct clk *clkset_vpllsrc_list[] = {
	[0] = &clk_fin_vpll,
	[1] = &clk_sclk_hdmi27m,
};

static struct clksrc_sources clkset_vpllsrc = {
	.sources	= clkset_vpllsrc_list,
	.nr_sources	= ARRAY_SIZE(clkset_vpllsrc_list),
};

static struct clksrc_clk clk_vpllsrc = {
	.clk    = {
		.name           = "vpll_src",
		.id             = -1,
		.enable		= s5pv310_clksrc_mask_top_ctrl,
		.ctrlbit	= (1 << 0),
	},
	.sources        = &clkset_vpllsrc,
	.reg_src        = { .reg = S5P_CLKSRC_TOP1, .shift = 0, .size = 1 },
};

static struct clk *clkset_sclk_vpll_list[] = {
	[0] = &clk_vpllsrc.clk,
	[1] = &clk_fout_vpll,
};

static struct clksrc_sources clkset_sclk_vpll = {
	.sources        = clkset_sclk_vpll_list,
	.nr_sources     = ARRAY_SIZE(clkset_sclk_vpll_list),
};

static struct clksrc_clk clk_sclk_vpll = {
	.clk    = {
		.name           = "sclk_vpll",
		.id             = -1,
	},
	.sources        = &clkset_sclk_vpll,
	.reg_src        = { .reg = S5P_CLKSRC_TOP0, .shift = 8, .size = 1 },
};

static struct clk *clkset_sclk_dac_list[] = {
	[0] = &clk_sclk_vpll.clk,
	[1] = &clk_sclk_hdmiphy,
};

static struct clksrc_sources clkset_sclk_dac = {
	.sources	= clkset_sclk_dac_list,
	.nr_sources	= ARRAY_SIZE(clkset_sclk_dac_list),
};

static struct clksrc_clk clk_sclk_dac = {
	.clk	= {
		.name		= "sclk_dac",
		.id		= -1,
		.enable		= s5pv310_clksrc_mask_tv_ctrl,
		.ctrlbit	= (1 << 8),
	},
	.sources	= &clkset_sclk_dac,
	.reg_src	= { .reg = S5P_CLKSRC_TV, .shift = 8, .size = 1 },
};

static struct clksrc_clk clk_sclk_pixel = {
	.clk	= {
		.name		= "sclk_pixel",
		.id		= -1,
		.parent		= &clk_sclk_vpll.clk,
	},
	.reg_div	= { .reg = S5P_CLKDIV_TV, .shift = 0, .size = 4 },
};

static struct clk *clkset_sclk_hdmi_list[] = {
	[0] = &clk_sclk_pixel.clk,
	[1] = &clk_sclk_hdmiphy,
};

static struct clksrc_sources clkset_sclk_hdmi = {
	.sources	= clkset_sclk_hdmi_list,
	.nr_sources	= ARRAY_SIZE(clkset_sclk_hdmi_list),
};

static struct clksrc_clk clk_sclk_hdmi = {
	.clk	= {
		.name		= "sclk_hdmi",
		.id		= -1,
		.enable		= s5pv310_clksrc_mask_tv_ctrl,
		.ctrlbit	= (1 << 0),
	},
	.sources	= &clkset_sclk_hdmi,
	.reg_src	= { .reg = S5P_CLKSRC_TV, .shift = 0, .size = 1 },
};

static struct clk *clkset_sclk_mixer_list[] = {
	[0] = &clk_sclk_dac.clk,
	[1] = &clk_sclk_hdmi.clk,
};

static struct clksrc_sources clkset_sclk_mixer = {
	.sources	= clkset_sclk_mixer_list,
	.nr_sources	= ARRAY_SIZE(clkset_sclk_mixer_list),
};

static struct clk *clkset_mout_hpm_list[] = {
	[0] = &clk_mout_apll.clk,
	[1] = &clk_mout_mpll.clk,
};

static struct clksrc_sources clkset_sclk_hpm = {
	.sources	= clkset_mout_hpm_list,
	.nr_sources	= ARRAY_SIZE(clkset_mout_hpm_list),
};

static struct clksrc_clk clk_dout_copy = {
	.clk	= {
		.name		= "dout_copy",
		.id		= -1,
	},
	.sources	= &clkset_sclk_hpm,
	.reg_src	= { .reg = S5P_CLKSRC_CPU, .shift = 20, .size = 1 },
	.reg_div	= { .reg = S5P_CLKDIV_CPU1, .shift = 0, .size = 3 },
};
static struct clk init_clocks_disable[] = {
        {
                .name           = "timers",
                .id             = -1,
                .parent         = &clk_aclk_100.clk,
                .enable         = s5pv310_clk_ip_peril_ctrl,
                .ctrlbit        = (1 << 24),
        }, {
		.name		= "csis",
		.id		= 0,
		.enable		= s5pv310_clk_ip_cam_ctrl,
		.ctrlbit	= (1 << 4),
	}, {
		.name		= "csis",
		.id		= 1,
		.enable		= s5pv310_clk_ip_cam_ctrl,
		.ctrlbit	= (1 << 5),
	}, {
		.name		= "fimc",
		.id		= 0,
		.parent		= &clk_aclk_160.clk,
		.enable		= s5pv310_clk_ip_cam_ctrl,
		.ctrlbit	= ((0x1 << 12) | (0x1 << 7) | (0x1 << 0)),
	}, {
		.name		= "fimc",
		.id		= 1,
		.parent		= &clk_aclk_160.clk,
		.enable		= s5pv310_clk_ip_cam_ctrl,
		.ctrlbit	= ((0x1 << 13) | (0x1 << 8) | (0x1 << 1)),
	}, {
		.name		= "fimc",
		.id		= 2,
		.parent		= &clk_aclk_160.clk,
		.enable		= s5pv310_clk_ip_cam_ctrl,
		.ctrlbit	= ((0x1 << 14) | (0x1 << 9) | (0x1 << 2)),
	}, {
		.name		= "fimc",
		.id		= 3,
		.parent		= &clk_aclk_160.clk,
		.enable		= s5pv310_clk_ip_cam_ctrl,
		.ctrlbit	= ((0x1 << 15) | (0x1 << 10) | (0x1 << 3)),
	}, {
		.name		= "jpeg",
		.id		= -1,
		.enable		= s5pv310_clk_ip_cam_ctrl,
		.ctrlbit	= ((0x1 << 11) | (0x1 << 6)),
	}, {
		.name		= "mfc",
		.id		= -1,
		.parent		= &clk_aclk_100.clk,
		.enable		= s5pv310_clk_ip_mfc_ctrl,
		.ctrlbit	= ((0x1 << 4) | (0x1 << 3) | (0x1 << 2) | (0x1 << 1) | (0x1 << 0)),
	}, {
		.name		= "hsmmc",
		.id		= 0,
		.enable		= s5pv310_clk_ip_fsys_ctrl,
		.ctrlbit	= (1 << 5),
	}, {
		.name		= "hsmmc",
		.id		= 1,
		.enable		= s5pv310_clk_ip_fsys_ctrl,
		.ctrlbit	= (1 << 6),
	}, {
		.name		= "hsmmc",
		.id		= 2,
		.enable		= s5pv310_clk_ip_fsys_ctrl,
		.ctrlbit	= (1 << 7),
	}, {
		.name		= "hsmmc",
		.id		= 3,
		.enable		= s5pv310_clk_ip_fsys_ctrl,
		.ctrlbit	= (1 << 8),
	}, {
		.name		= "mshc",
		.id		= -1,
		.enable		= s5pv310_clk_ip_fsys_ctrl,
		.ctrlbit	= (1 << 9),
	}, {
		.name		= "iis",
		.id		= 0,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 20),
	}, {
		.name		= "iis",
		.id		= 1,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 21),
	}, {
		.name		= "pcm",
		.id		= 1,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 22),
	}, {
		.name		= "pcm",
		.id		= 2,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 23),
	}, {
		.name		= "spdif",
		.id		= -1,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 26),
	}, {
		.name		= "sata",
		.id		= -1,
		.enable		= s5pv310_clk_ip_fsys_ctrl,
		.ctrlbit	= (1 << 10),
	}, {
		.name		= "adc",
		.id		= -1,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 15),
	}, {
		.name		= "watchdog",
		.id		= -1,
		.enable		= s5pv310_clk_ip_perir_ctrl,
		.ctrlbit	= (1 << 14),
	}, {
		.name		= "rtc",
		.id		= -1,
		.enable		= s5pv310_clk_ip_perir_ctrl,
		.ctrlbit	= (1 << 15),
	}, {
		.name		= "fimg2d",
		.id		= -1,
		.enable		= s5pv310_clk_ip_image_ctrl,
		.ctrlbit	= ((0x1 << 6) | (0x1 << 3) | (0x1 << 0)),
	}, {
		.name		= "usbhost",
		.id		= -1,
		.enable		= s5pv310_clk_ip_fsys_ctrl ,
		.ctrlbit	= (1 << 12),
	}, {
		.name		= "usbotg",
		.id		= -1,
		.enable		= s5pv310_clk_ip_fsys_ctrl,
		.ctrlbit	= (1 << 13),
	}, {
		.name		= "spi",
		.id		= 1,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 17),
	}, {
		.name		= "spi",
		.id		= 2,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 18),
	}, {
		.name		= "i2c",
		.id		= 0,
		.parent		= &clk_aclk_100.clk,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 6),
	}, {
		.name		= "i2c",
		.id		= 1,
		.parent		= &clk_aclk_100.clk,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 7),
	}, {
		.name		= "i2c",
		.id		= 2,
		.parent		= &clk_aclk_100.clk,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 8),
	}, {
		.name		= "i2c",
		.id		= 3,
		.parent		= &clk_aclk_100.clk,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 9),
	}, {
		.name		= "i2c",
		.id		= 4,
		.parent		= &clk_aclk_100.clk,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 10),
	}, {
		.name		= "i2c",
		.id		= 5,
		.parent		= &clk_aclk_100.clk,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 11),
	}, {
		.name		= "i2c",
		.id		= 6,
		.parent		= &clk_aclk_100.clk,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 12),
	}, {
		.name		= "i2c",
		.id		= 7,
		.parent		= &clk_aclk_100.clk,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 13),
	}, {
		.name		= "ac97",
		.id		= -1,
		.parent		= &clk_aclk_100.clk,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 27),
	}, {
		.name		= "keypad",
		.id		= -1,
		.enable		= s5pv310_clk_ip_perir_ctrl,
		.ctrlbit	= (1 << 16),
	}, {
		.name           = "i2c-hdmiphy",
		.id             = -1,
		.parent         = &clk_aclk_100.clk,
		.enable         = s5pv310_clk_ip_peril_ctrl,
		.ctrlbit        = (1 << 14),
	}, {
		.name		= "hdmi",
		.id		= -1,
		.parent		= &clk_aclk_160.clk,
		.enable		= s5pv310_clk_ip_tv_ctrl,
		.ctrlbit	= ((0x1 << 5) | (0x1 << 4) | (0x1 << 3)),
	}, {
		.name		= "tvenc",
		.id		= -1,
		.parent		= &clk_aclk_160.clk,
		.enable		= s5pv310_clk_ip_tv_ctrl,
		.ctrlbit	= (1 << 2),
	}, {
		.name		= "mixer",
		.id		= -1,
		.parent		= &clk_aclk_160.clk,
		.enable		= s5pv310_clk_ip_tv_ctrl,
		.ctrlbit	= (1 << 1),
	}, {
		.name		= "vp",
		.id		= -1,
		.parent		= &clk_aclk_160.clk,
		.enable		= s5pv310_clk_ip_tv_ctrl,
		.ctrlbit	= (1 << 0),
	}, {
		.name		= "secss",
		.id		= -1,
		.parent		= &clk_aclk_acp.clk,
		.enable		= s5pv310_clk_ip_dmc_ctrl,
		.ctrlbit	= ((1 << 15) | (1 << 13) | (1 << 12) | (1 << 4)),
	}, {
		.name		= "mie1",
		.id		= -1,
		.enable		= s5pv310_clk_ip_lcd1_ctrl,
		.ctrlbit	= (1 << 1),
	}, {
		.name		= "mdnie1",
		.id		= -1,
		.enable		= s5pv310_clk_ip_lcd1_ctrl,
		.ctrlbit	= (1 << 2),
	}, {
		.name		= "gps",
		.id		= -1,
		.enable		= s5pv310_clk_ip_gps_ctrl,
		.ctrlbit	= ((1 << 1) | (1 << 0)),
	}, {
		.name           = "iis",
		.id             = -1,
		.enable         = s5pv310_clk_audss_ctrl,
		.ctrlbit        = (1 << 3) | (1 << 2),
	}, {
		.name           = "pcm",
		.id             = 0,
		.enable         = s5pv310_clk_audss_ctrl,
		.ctrlbit        = (1 << 5) | (1 << 4),
	}, {
		.name           = "srp",
		.id             = -1,
		.enable         = s5pv310_clk_audss_ctrl,
		.ctrlbit        = (1 << 8) | (1 << 7) | (1 << 6) | (1 << 0),
	},

	/* Add the IP clocks in addition excluding SYSMMU and PMMU */
	{
		.name		= "rot",
		.id		= -1,
		.enable		= s5pv310_clk_ip_image_ctrl,
		.ctrlbit	= ((0x1 << 7) | (0x1 << 4) | (0x1 << 1)),
	}, {
		.name		= "nfcon",
		.id		= -1,
		.enable		= s5pv310_clk_ip_fsys_ctrl,
		.ctrlbit	= (1 << 16),
	}, {
		.name		= "pcie",
		.id		= 1,
		.enable		= s5pv310_clk_ip_fsys_ctrl,
		.ctrlbit	= ((1 << 18) | (1 << 14)),
#if !defined(CONFIG_VIDEO_TSI)
	}, {
		.name		= "tsi",
		.id		= 1,
		.enable		= s5pv310_clk_ip_fsys_ctrl,
		.ctrlbit	= (1 << 4),
#endif
	}, {
		.name		= "sataphy",
		.id		= 1,
		.enable		= s5pv310_clk_ip_fsys_ctrl,
		.ctrlbit	= (1 << 3),
	}, {
		.name		= "pciephy",
		.id		= 1,
		.enable		= s5pv310_clk_ip_fsys_ctrl,
		.ctrlbit	= (1 << 2),
	}, {
		.name		= "modem",
		.id		= -1,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 28),
	}, {
		.name		= "seckey",
		.id		= -1,
		.enable		= s5pv310_clk_ip_perir_ctrl,
		.ctrlbit	= (1 << 12),
	}, {
		.name		= "hdmicec",
		.id		= -1,
		.enable		= s5pv310_clk_ip_perir_ctrl,
		.ctrlbit	= (1 << 11),
	}, {
		.name		= "tzpc",
		.id		=  0,
		.enable		= s5pv310_clk_ip_perir_ctrl,
		.ctrlbit	= (1 << 5),
	}, {
		.name		= "tzpc",
		.id		=  1,
		.enable		= s5pv310_clk_ip_perir_ctrl,
		.ctrlbit	= (1 << 6),
	}, {
		.name		= "tzpc",
		.id		=  2,
		.enable		= s5pv310_clk_ip_perir_ctrl,
		.ctrlbit	= (1 << 7),
	}, {
		.name		= "tzpc",
		.id		=  3,
		.enable		= s5pv310_clk_ip_perir_ctrl,
		.ctrlbit	= (1 << 8),
	}, {
		.name		= "tzpc",
		.id		=  4,
		.enable		= s5pv310_clk_ip_perir_ctrl,
		.ctrlbit	= (1 << 9),
	}, {
		.name		= "tzpc",
		.id		=  5,
		.enable		= s5pv310_clk_ip_perir_ctrl,
		.ctrlbit	= (1 << 10),
	}, {
		.name		= "slimbus",
		.id		= -1,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 25),
	}, {
		.name		= "dsim",
		.id		= 0,
		.enable		= s5pv310_clk_ip_lcd0_ctrl,
		.ctrlbit	= (1 << 3),
	}, {
		.name		= "dsim",
		.id		= 1,
		.enable		= s5pv310_clk_ip_lcd1_ctrl,
		.ctrlbit	= (1 << 3),
	}, {
	/* Not used IPs are clock gated */
		.name		= "iem-iec",
		.id		= -1,
		.enable		= s5pv310_clk_ip_dmc_ctrl,
		.ctrlbit	= (1 << 18),
	}, {
		.name		= "iem-apc",
		.id		= -1,
		.enable		= s5pv310_clk_ip_dmc_ctrl,
		.ctrlbit	= (1 << 17),
	}, {
		.name		= "ppmuacp",
		.id		= -1,
		.enable		= s5pv310_clk_ip_dmc_ctrl,
		.ctrlbit	= (1 << 16),
	}, {
		.name		= "id_remap",
		.id		= -1,
		.enable		= s5pv310_clk_ip_dmc_ctrl,
		.ctrlbit	= (1 << 13),
	}, {
		.name		= "sromc",
		.id		= 1,
		.enable		= s5pv310_clk_ip_fsys_ctrl,
		.ctrlbit	= (1 << 11),
	}, {
		.name		= "secjtag",
		.id		= -1,
		.enable		= s5pv310_clk_ip_dmc_ctrl,
		.ctrlbit	= (1 << 6),
	}, {
		.name		= "ppmufile",
		.id		= -1,
		.enable		= s5pv310_clk_ip_fsys_ctrl,
		.ctrlbit	= (1 << 17),
	}, {
		.name		= "ppmuimage",
		.id		= -1,
		.enable		= s5pv310_clk_ip_image_ctrl,
		.ctrlbit	= (1 << 9),
	}, {
		.name		= "pxl_async1",
		.id		= -1,
		.enable		= s5pv310_clk_ip_cam_ctrl,
		.ctrlbit	= (0x1 << 18),
	}, {
		.name		= "pxl_async0",
		.id		= -1,
		.enable		= s5pv310_clk_ip_cam_ctrl,
		.ctrlbit	= (0x1 << 17),
	}, {
		.name		= "ppmucamif",
		.id		= 1,
		.enable		= s5pv310_clk_ip_cam_ctrl,
		.ctrlbit	= (0x1 << 16),
	}, {
		.name		= "ppmug3d",
		.id		= -1,
		.enable		= s5pv310_clk_ip_g3d_ctrl,
		.ctrlbit	= (0x1 << 1),
	}, {
		.name		= "chipid",
		.id		= -1,
		.enable		= s5pv310_clk_ip_perir_ctrl,
		.ctrlbit	= (1 << 0),
	}, {
		.name		= "onenand",
		.id		= -1,
		.enable		= s5pv310_clk_ip_fsys_ctrl,
		.ctrlbit	= (1 << 15),
	}, {
		.name		= "hpm",
		.id		= -1,
		.enable		= s5pv310_clk_ip_cpu_ctrl,
		.ctrlbit	= (1 << 0),
	}, {
		.name		= "ppmuleft",
		.id		= -1,
		.enable		= s5pv310_clk_ip_leftbus_ctrl,
		.ctrlbit	= (1 << 1),
	}, {
		.name		= "ppmuright",
		.id		= -1,
		.enable		= s5pv310_clk_ip_rightbus_ctrl,
		.ctrlbit	= (1 << 1),
	}, {
		.name		= "mdma1",
		.id		= -1,
		.enable		= s5pv310_clk_ip_image_ctrl,
		.ctrlbit	= ((1 << 8) | (1 << 5) | (1 << 2)),
	}, {
		.name		= "fimd1",
		.id		= -1,
		.enable		= s5pv310_clk_ip_lcd1_ctrl,
		.ctrlbit	= ((0x1 << 5) | (0x1 << 4) | (0x1 << 0)),
	},
};

static struct clk init_clocks[] = {
	{
		.name		= "uart",
		.id		= 0,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 0),
	}, {
		.name		= "uart",
		.id		= 1,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 1),
	}, {
		.name		= "uart",
		.id		= 2,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 2),
	}, {
		.name		= "uart",
		.id		= 3,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 3),
	}, {
		.name		= "uart",
		.id		= 4,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 4),
	}, {
		.name		= "uart",
		.id		= 5,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 5),
	}, {
		.name		= "gic",
		.id		= -1,
		.enable		= s5pv310_clk_ip_dmc_ctrl,
		.ctrlbit	= (1 << 20),
	}, {
		.name		= "coretimers",
		.id		= -1,
		.enable		= s5pv310_clk_ip_dmc_ctrl,
		.ctrlbit	= (1 << 19),
	}, {
		.name		= "cpu",
		.id		= -1,
		.enable		= s5pv310_clk_ip_dmc_ctrl,
		.ctrlbit	= ((1 << 10) | (1 << 7)),
	}, {
		.name		= "int_combiner",
		.id		= -1,
		.enable		= s5pv310_clk_ip_dmc_ctrl,
		.ctrlbit	= (1 << 2),
	}, {
		.name		= "dmc1",
		.id		= -1,
		.enable		= s5pv310_clk_ip_dmc_ctrl,
		.ctrlbit	= ((1 << 9) | (1 << 1)),
	}, {
		.name		= "dmc0",
		.id		= -1,
		.enable		= s5pv310_clk_ip_dmc_ctrl,
		.ctrlbit	= ((1 << 8) | (1 << 0)),
	},

	/* Add the IP clocks in addition excluding SYSMMU and PMMU */
	{
		.name		= "slimbus",
		.id		= -1,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 25),
	}, {
		.name		= "sysreg",
		.id		= -1,
		.enable		= s5pv310_clk_ip_perir_ctrl,
		.ctrlbit	= (1 << 1),
	}, {
		.name		= "tmu_apbif",
		.id		= -1,
		.enable		= s5pv310_clk_ip_perir_ctrl,
		.ctrlbit	= (1 << 17),
	}, {
		.name		= "spi",
		.id		= 0,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 16),
	}, {
		.name		= "cmu_dmcpart",
		.id		= -1,
		.enable		= s5pv310_clk_ip_perir_ctrl,
		.ctrlbit	= (1 << 4),
	}, {
		.name		= "pmu_apbif",
		.id		= -1,
		.enable		= s5pv310_clk_ip_perir_ctrl,
		.ctrlbit	= (1 << 3),
	}, {
		.name		= "cssys",
		.id		= -1,
		.enable		= s5pv310_clk_ip_cpu_ctrl,
		.ctrlbit	= (1 << 1),
	}, {
		.name		= "fimd0",
		.id		= -1,
		.enable		= s5pv310_clk_ip_lcd0_ctrl,
		.ctrlbit	= ((0x1 << 5) | (0x1 << 4) | (0x1 << 0)),
	}, {
		.name		= "mie0",
		.id		= -1,	/* changed to -1 to get by NULL id */
		.enable		= s5pv310_clk_ip_lcd0_ctrl,
		.ctrlbit	= (1 << 1),
	}, {
		.name		= "mdnie0",
		.id		= -1,	/* changed to -1 to get by NULL id */
		.enable		= s5pv310_clk_ip_lcd0_ctrl,
		.ctrlbit	= (1 << 2),
#if defined(CONFIG_VIDEO_TSI)
	}, {

			.name		= "tsi",
			.id	= 1,
			.enable		= s5pv310_clk_ip_fsys_ctrl,
			.ctrlbit	= (1 << 4),
#endif
#ifdef CONFIG_FB_S3C_MIPI_LCD
	}, {
		.name		= "dsim0",
		.id		= -1,
		.enable		= s5pv310_clk_ip_lcd0_ctrl,
		.ctrlbit	= (1 << 3),
	}, {
		.name		= "dsim1",
		.id		= -1,
		.enable		= s5pv310_clk_ip_lcd1_ctrl,
		.ctrlbit	= (1 << 3),
#endif
	},
};

static struct clk init_dmaclocks[] = {
	{
		.name		= "dma",
		.id		= 0,
#if defined(CONFIG_CPU_S5PV310_EVT1)
		.enable		= s5pv310_clk_ip_image_ctrl,
		.ctrlbit	= ((1 << 8) | (1 << 5) | (1 << 2)),
#else
		.enable		= s5pv310_clk_ip_dmc_ctrl,
		.ctrlbit	= ((1 << 14) | (1 << 11) | (1 << 3)),
#endif
		.dev            = &s5pv310_device_mdma.dev,
	}, {
		.name		= "dma",
		.id		= 1,
		.enable		= s5pv310_clk_ip_fsys_ctrl,
		.ctrlbit	= (1 << 0),
		.dev            = &s5pv310_device_pdma0.dev,
	}, {
		.name		= "dma",
		.id		= 2,
		.parent		= &init_dmaclocks[1],
		.enable		= s5pv310_clk_ip_fsys_ctrl,
		.ctrlbit	= (1 << 1),
		.dev            = &s5pv310_device_pdma1.dev,
	},
};

static struct clk *clkset_sclk_audio0_list[] = {
	[0] = &clk_audiocdclk0,
	[1] = NULL,
	[2] = &clk_sclk_hdmi27m,
	[3] = &clk_sclk_usbphy0,
	[4] = &clk_sclk_xxti,
	[5] = &clk_sclk_xusbxti,
	[6] = &clk_mout_mpll.clk,
	[7] = &clk_mout_epll.clk,
	[8] = &clk_sclk_vpll.clk,
};

static struct clksrc_sources clkset_sclk_audio0 = {
	.sources	= clkset_sclk_audio0_list,
	.nr_sources	= ARRAY_SIZE(clkset_sclk_audio0_list),
};

static struct clksrc_clk clk_sclk_audio0 = {
	.clk		= {
		.name		= "audio-bus",
		.id		= 0,
		.enable		= s5pv310_clksrc_mask_maudio_ctrl,
		.ctrlbit	= (1 << 0),
	},
	.sources = &clkset_sclk_audio0,
	.reg_src = { .reg = S5P_CLKSRC_MAUDIO, .shift = 0, .size = 4 },
	.reg_div = { .reg = S5P_CLKDIV_MAUDIO, .shift = 0, .size = 4 },
};

static struct clk *clkset_mout_audss_list[] = {
	NULL,
	&clk_fout_epll,
};

static struct clksrc_sources clkset_mout_audss = {
	.sources	= clkset_mout_audss_list,
	.nr_sources	= ARRAY_SIZE(clkset_mout_audss_list),
};

static struct clksrc_clk clk_mout_audss = {
	.clk		= {
		.name		= "mout_audss",
		.id		= -1,
	},
	.sources	= &clkset_mout_audss,
	.reg_src	= { .reg = S5P_CLKSRC_AUDSS, .shift = 0, .size = 1 },
};

static struct clk *clkset_sclk_audss_list[] = {
	&clk_mout_audss.clk,
	&clk_audiocdclk0,
	&clk_sclk_audio0.clk,
};

static struct clksrc_sources clkset_sclk_audss = {
	.sources	= clkset_sclk_audss_list,
	.nr_sources	= ARRAY_SIZE(clkset_sclk_audss_list),
};

static struct clksrc_clk clk_sclk_audss = {
	.clk		= {
		.name		= "audio-bus",
		.id		= -1,
		.enable		= s5pv310_clk_audss_ctrl,
		.ctrlbit	= S5P_AUDSS_CLKGATE_I2SBUS,
	},
	.sources	= &clkset_sclk_audss,
	.reg_src	= { .reg = S5P_CLKSRC_AUDSS, .shift = 2, .size = 2 },
	.reg_div	= { .reg = S5P_CLKDIV_AUDSS, .shift = 4, .size = 8 },
};

static struct clk *clkset_sclk_audio1_list[] = {
	[0] = &clk_audiocdclk1,
	[1] = NULL,
	[2] = &clk_sclk_hdmi27m,
	[3] = &clk_sclk_usbphy0,
	[4] = &clk_sclk_xxti,
	[5] = &clk_sclk_xusbxti,
	[6] = &clk_mout_mpll.clk,
	[7] = &clk_mout_epll.clk,
	[8] = &clk_sclk_vpll.clk,
};

static struct clksrc_sources clkset_sclk_audio1 = {
	.sources	= clkset_sclk_audio1_list,
	.nr_sources	= ARRAY_SIZE(clkset_sclk_audio1_list),
};

static struct clksrc_clk clk_sclk_audio1 = {
	.clk		= {
		.name		= "audio-bus",
		.id		= 1,
		.enable		= s5pv310_clksrc_mask_peril1_ctrl,
		.ctrlbit	= (1 << 0),
	},
	.sources = &clkset_sclk_audio1,
	.reg_src = { .reg = S5P_CLKSRC_PERIL1, .shift = 0, .size = 4 },
	.reg_div = { .reg = S5P_CLKDIV_PERIL4, .shift = 4, .size = 8 },
};

static struct clk *clkset_sclk_audio2_list[] = {
	[0] = &clk_audiocdclk2,
	[1] = NULL,
	[2] = &clk_sclk_hdmi27m,
	[3] = &clk_sclk_usbphy0,
	[4] = &clk_sclk_xxti,
	[5] = &clk_sclk_xusbxti,
	[6] = &clk_mout_mpll.clk,
	[7] = &clk_mout_epll.clk,
	[8] = &clk_sclk_vpll.clk,
};

static struct clksrc_sources clkset_sclk_audio2 = {
	.sources	= clkset_sclk_audio2_list,
	.nr_sources	= ARRAY_SIZE(clkset_sclk_audio2_list),
};

static struct clksrc_clk clk_sclk_audio2 = {
	.clk	= {
		.name		= "audio-bus",
		.id		= 2,
		.enable		= s5pv310_clksrc_mask_peril1_ctrl,
		.ctrlbit	= (1 << 4),
	},
	.sources = &clkset_sclk_audio2,
	.reg_src = { .reg = S5P_CLKSRC_PERIL1, .shift = 4, .size = 4 },
	.reg_div = { .reg = S5P_CLKDIV_PERIL4, .shift = 16, .size = 4 },
};

static struct clk *clkset_sclk_spdif_list[] = {
	[0] = &clk_sclk_audio0.clk,
	[1] = &clk_sclk_audio1.clk,
	[2] = &clk_sclk_audio2.clk,
	[3] = &clk_spdifcdclk,
};

static struct clksrc_sources clkset_sclk_spdif = {
	.sources	= clkset_sclk_spdif_list,
	.nr_sources	= ARRAY_SIZE(clkset_sclk_spdif_list),
};

static struct clksrc_clk clk_sclk_spdif = {
	.clk	= {
		.name		= "sclk_spdif",
		.id		= -1,
		.enable		= s5pv310_clksrc_mask_peril1_ctrl,
		.ctrlbit	= (1 << 8),
	},
	.sources = &clkset_sclk_spdif,
	.reg_src = { .reg = S5P_CLKSRC_PERIL1, .shift = 8, .size = 2 },
};

static struct clk *clkset_group_list[] = {
	[0] = &clk_ext_xtal_mux,
	[1] = &clk_xusbxti,
	[2] = &clk_sclk_hdmi27m,
	[3] = &clk_sclk_usbphy0,
	[4] = &clk_sclk_usbphy1,
	[5] = &clk_sclk_hdmiphy,
	[6] = &clk_mout_mpll.clk,
	[7] = &clk_mout_epll.clk,
	[8] = &clk_sclk_vpll.clk,
};

static struct clksrc_sources clkset_group = {
	.sources	= clkset_group_list,
	.nr_sources	= ARRAY_SIZE(clkset_group_list),
};

static struct clksrc_clk clk_sclk_mipidphy4l = {
	.clk    = {
		.name           = "sclk_mipidphy4l",
		.id             = -1,
		.enable		= s5pv310_clk_ip_lcd0_ctrl,
		.ctrlbit	= (1 << 4),
	},
	.sources        = &clkset_group,
	.reg_src        = { .reg = S5P_CLKSRC_LCD0, .shift = 12, .size = 4 },
	.reg_div	= { .reg = S5P_CLKDIV_LCD0, .shift = 16, .size = 4 },
};

static struct clksrc_clk clk_sclk_mipidphy2l = {
	.clk    = {
		.name           = "sclk_mipidphy2l",
		.id             = -1,
		.enable		= s5pv310_clk_ip_lcd1_ctrl,
		.ctrlbit	= (1 << 4),
	},
	.sources        = &clkset_group,
	.reg_src        = { .reg = S5P_CLKSRC_LCD1, .shift = 12, .size = 4 },
	.reg_div	= { .reg = S5P_CLKDIV_LCD1, .shift = 16, .size = 4 },
};

static struct clk *clkset_mout_g2d0_list[] = {
	[0] = &clk_mout_mpll.clk,
	[1] = &clk_sclk_apll.clk,
};

static struct clksrc_sources clkset_mout_g2d0 = {
	.sources	= clkset_mout_g2d0_list,
	.nr_sources	= ARRAY_SIZE(clkset_mout_g2d0_list),
};

static struct clksrc_clk clk_mout_g2d0 = {
	.clk	= {
		.name		= "mout_g2d0",
		.id		= -1,
	},
	.sources	= &clkset_mout_g2d0,
	.reg_src	= { .reg = S5P_CLKSRC_IMAGE, .shift = 0, .size = 1 },
};

static struct clk *clkset_mout_g2d1_list[] = {
	[0] = &clk_mout_epll.clk,
	[1] = &clk_sclk_vpll.clk,
};

static struct clksrc_sources clkset_mout_g2d1 = {
	.sources	= clkset_mout_g2d1_list,
	.nr_sources	= ARRAY_SIZE(clkset_mout_g2d1_list),
};

static struct clksrc_clk clk_mout_g2d1 = {
	.clk	= {
		.name		= "mout_g2d1",
		.id		= -1,
	},
	.sources	= &clkset_mout_g2d1,
	.reg_src	= { .reg = S5P_CLKSRC_IMAGE, .shift = 4, .size = 1 },
};

static struct clk *clkset_mout_g2d_list[] = {
	[0] = &clk_mout_g2d0.clk,
	[1] = &clk_mout_g2d1.clk,
};

static struct clksrc_sources clkset_mout_g2d = {
	.sources	= clkset_mout_g2d_list,
	.nr_sources	= ARRAY_SIZE(clkset_mout_g2d_list),
};

static struct clk *clkset_mout_mfc0_list[] = {
	[0] = &clk_mout_mpll.clk,
	[1] = &clk_sclk_apll.clk,
};

static struct clksrc_sources clkset_mout_mfc0 = {
	.sources	= clkset_mout_mfc0_list,
	.nr_sources	= ARRAY_SIZE(clkset_mout_mfc0_list),
};

static struct clksrc_clk clk_mout_mfc0 = {
	.clk	= {
		.name		= "mout_mfc0",
		.id		= -1,
	},
	.sources	= &clkset_mout_mfc0,
	.reg_src	= { .reg = S5P_CLKSRC_MFC, .shift = 0, .size = 1 },
};

static struct clk *clkset_mout_mfc1_list[] = {
	[0] = &clk_mout_epll.clk,
	[1] = &clk_sclk_vpll.clk,
};

static struct clksrc_sources clkset_mout_mfc1 = {
	.sources	= clkset_mout_mfc1_list,
	.nr_sources	= ARRAY_SIZE(clkset_mout_mfc1_list),
};

static struct clksrc_clk clk_mout_mfc1 = {
	.clk	= {
		.name		= "mout_mfc1",
		.id		= -1,
	},
	.sources	= &clkset_mout_mfc1,
	.reg_src	= { .reg = S5P_CLKSRC_MFC, .shift = 4, .size = 1 },
};

static struct clk *clkset_mout_mfc_list[] = {
	[0] = &clk_mout_mfc0.clk,
	[1] = &clk_mout_mfc1.clk,
};

static struct clksrc_sources clkset_mout_mfc = {
	.sources	= clkset_mout_mfc_list,
	.nr_sources	= ARRAY_SIZE(clkset_mout_mfc_list),
};

static struct clk *clkset_mout_g3d0_list[] = {
	[0] = &clk_mout_mpll.clk,
	[1] = &clk_sclk_apll.clk,
};

static struct clksrc_sources clkset_mout_g3d0 = {
	.sources	= clkset_mout_g3d0_list,
	.nr_sources	= ARRAY_SIZE(clkset_mout_g3d0_list),
};

static struct clksrc_clk clk_mout_g3d0 = {
	.clk	= {
		.name		= "mout_g3d0",
		.id		= -1,
	},
	.sources	= &clkset_mout_g3d0,
	.reg_src	= { .reg = S5P_CLKSRC_G3D, .shift = 0, .size = 1 },
};

static struct clk *clkset_mout_g3d1_list[] = {
	[0] = &clk_mout_epll.clk,
	[1] = &clk_sclk_vpll.clk,
};

static struct clksrc_sources clkset_mout_g3d1 = {
	.sources	= clkset_mout_g3d1_list,
	.nr_sources	= ARRAY_SIZE(clkset_mout_g3d1_list),
};

static struct clksrc_clk clk_mout_g3d1 = {
	.clk	= {
		.name		= "mout_g3d1",
		.id		= -1,
	},
	.sources	= &clkset_mout_g3d1,
	.reg_src	= { .reg = S5P_CLKSRC_G3D, .shift = 4, .size = 1 },
};

static struct clk *clkset_mout_g3d_list[] = {
	[0] = &clk_mout_g3d0.clk,
	[1] = &clk_mout_g3d1.clk,
};

static struct clksrc_sources clkset_mout_g3d = {
	.sources	= clkset_mout_g3d_list,
	.nr_sources	= ARRAY_SIZE(clkset_mout_g3d_list),
};

static struct clksrc_clk clk_dout_mmc0 = {
	.clk		= {
		.name		= "dout_mmc0",
		.id		= -1,
	},
	.sources = &clkset_group,
	.reg_src = { .reg = S5P_CLKSRC_FSYS, .shift = 0, .size = 4 },
	.reg_div = { .reg = S5P_CLKDIV_FSYS1, .shift = 0, .size = 4 },
};

static struct clksrc_clk clk_dout_mmc1 = {
	.clk		= {
		.name		= "dout_mmc1",
		.id		= -1,
	},
	.sources = &clkset_group,
	.reg_src = { .reg = S5P_CLKSRC_FSYS, .shift = 4, .size = 4 },
	.reg_div = { .reg = S5P_CLKDIV_FSYS1, .shift = 16, .size = 4 },
};

static struct clksrc_clk clk_dout_mmc2 = {
	.clk		= {
		.name		= "dout_mmc2",
		.id		= -1,
	},
	.sources = &clkset_group,
	.reg_src = { .reg = S5P_CLKSRC_FSYS, .shift = 8, .size = 4 },
	.reg_div = { .reg = S5P_CLKDIV_FSYS2, .shift = 0, .size = 4 },
};

static struct clksrc_clk clk_dout_mmc3 = {
	.clk		= {
		.name		= "dout_mmc3",
		.id		= -1,
	},
	.sources = &clkset_group,
	.reg_src = { .reg = S5P_CLKSRC_FSYS, .shift = 12, .size = 4 },
	.reg_div = { .reg = S5P_CLKDIV_FSYS2, .shift = 16, .size = 4 },
};

static struct clksrc_clk clk_dout_mmc4 = {
	.clk		= {
		.name		= "dout_mmc4",
		.id		= -1,
	},
	.sources = &clkset_group,
	.reg_src = { .reg = S5P_CLKSRC_FSYS, .shift = 16, .size = 4 },
	.reg_div = { .reg = S5P_CLKDIV_FSYS3, .shift = 0, .size = 4 },
};


static struct clksrc_clk clksrcs[] = {
	{
		.clk	= {
			.name		= "uclk1",
			.id		= 0,
			.ctrlbit	= (1 << 0),
			.enable		= s5pv310_clksrc_mask_peril0_ctrl,
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_PERIL0, .shift = 0, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_PERIL0, .shift = 0, .size = 4 },
	}, {
		.clk		= {
			.name		= "uclk1",
			.id		= 1,
			.enable		= s5pv310_clksrc_mask_peril0_ctrl,
			.ctrlbit	= (1 << 4),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_PERIL0, .shift = 4, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_PERIL0, .shift = 4, .size = 4 },
	}, {
		.clk		= {
			.name		= "uclk1",
			.id		= 2,
			.enable		= s5pv310_clksrc_mask_peril0_ctrl,
			.ctrlbit	= (1 << 8),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_PERIL0, .shift = 8, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_PERIL0, .shift = 8, .size = 4 },
	}, {
		.clk		= {
			.name		= "uclk1",
			.id		= 3,
			.enable		= s5pv310_clksrc_mask_peril0_ctrl,
			.ctrlbit	= (1 << 12),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_PERIL0, .shift = 12, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_PERIL0, .shift = 12, .size = 4 },
	}, {
		.clk		= {
			.name		= "uclk1",
			.id		= 4,
			.enable		= s5pv310_clksrc_mask_peril0_ctrl,
			.ctrlbit	= (1 << 16),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_PERIL0, .shift = 16, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_PERIL0, .shift = 16, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_pwm",
			.id		= -1,
			.enable		= s5pv310_clk_ip_peril_ctrl,
			.ctrlbit	= (1 << 24),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_PERIL0, .shift = 24, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_PERIL3, .shift = 0, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_csis",
			.id		= 0,
			.enable		= s5pv310_clksrc_mask_cam_ctrl,
			.ctrlbit	= (1 << 24),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_CAM, .shift = 24, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_CAM, .shift = 24, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_csis",
			.id		= 1,
			.enable		= s5pv310_clksrc_mask_cam_ctrl,
			.ctrlbit	= (1 << 28),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_CAM, .shift = 28, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_CAM, .shift = 28, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_cam0",
			.id		= -1,
			.enable		= s5pv310_clksrc_mask_cam_ctrl,
			.ctrlbit	= (1 << 16),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_CAM, .shift = 16, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_CAM, .shift = 16, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_cam1",
			.id		= -1,
			.enable		= s5pv310_clksrc_mask_cam_ctrl,
			.ctrlbit	= (1 << 20),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_CAM, .shift = 20, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_CAM, .shift = 20, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_fimc",
			.id		= 0,
			.enable		= s5pv310_clksrc_mask_cam_ctrl,
			.ctrlbit	= (1 << 0),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_CAM, .shift = 0, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_CAM, .shift = 0, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_fimc",
			.id		= 1,
			.enable		= s5pv310_clksrc_mask_cam_ctrl,
			.ctrlbit	= (1 << 4),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_CAM, .shift = 4, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_CAM, .shift = 4, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_fimc",
			.id		= 2,
			.enable		= s5pv310_clksrc_mask_cam_ctrl,
			.ctrlbit	= (1 << 8),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_CAM, .shift = 8, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_CAM, .shift = 8, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_fimc",
			.id		= 3,
			.enable		= s5pv310_clksrc_mask_cam_ctrl,
			.ctrlbit	= (1 << 12),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_CAM, .shift = 12, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_CAM, .shift = 12, .size = 4 },
	}, {
		.clk	= {
			.name		= "sclk_mixer",
			.id		= -1,
			.enable		= s5pv310_clksrc_mask_tv_ctrl,
			.ctrlbit	= (1 << 4),
		},
		.sources = &clkset_sclk_mixer,
		.reg_src = { .reg = S5P_CLKSRC_TV, .shift = 4, .size = 1 },
	}, {
		.clk		= {
			.name		= "sclk_fimd",
			.id		= 0,
			.enable		= s5pv310_clksrc_mask_lcd0_ctrl,
			.ctrlbit	= (1 << 0),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_LCD0, .shift = 0, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_LCD0, .shift = 0, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_fimd",
			.id		= 1,
			.enable		= s5pv310_clksrc_mask_lcd1_ctrl,
			.ctrlbit	= (1 << 0),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_LCD1, .shift = 0, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_LCD1, .shift = 0, .size = 4 },
#ifdef CONFIG_FB_S3C_MDNIE
	}, {
		.clk		= {
			.name		= "sclk_mdnie",
			.id		= -1,
			.enable		= s5pv310_clksrc_mask_lcd0_ctrl,
			.ctrlbit	= (1 << 4),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_LCD0, .shift = 4, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_LCD0, .shift = 4, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_mdnie",
			.id		= 1,
			.enable		= s5pv310_clksrc_mask_lcd1_ctrl,
			.ctrlbit	= (1 << 4),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_LCD1, .shift = 4, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_LCD1, .shift = 4, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_mdnie_pwm",
			.id		= -1,
			.enable		= s5pv310_clksrc_mask_lcd0_ctrl,
			.ctrlbit	= (1 << 8),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_LCD0, .shift = 8, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_LCD0, .shift = 8, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_mdnie_pwm",
			.id		= 1,
			.enable		= s5pv310_clksrc_mask_lcd1_ctrl,
			.ctrlbit	= (1 << 8),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_LCD1, .shift = 8, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_LCD1, .shift = 8, .size = 4 },
#endif
	}, {
		.clk		= {
			.name		= "sclk_mipi",
#ifdef CONFIG_FB_S3C_MIPI_LCD
			.id		= -1,
#else
			.id		= 0,
#endif
			.parent		= &clk_sclk_mipidphy4l.clk,
			.enable		= s5pv310_clksrc_mask_lcd0_ctrl,
			.ctrlbit	= (1 << 12),
		},
		.reg_div = { .reg = S5P_CLKDIV_LCD0, .shift = 20, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_mipi",
			.id		= 1,
			.parent		= &clk_sclk_mipidphy2l.clk,
			.enable		= s5pv310_clksrc_mask_lcd1_ctrl,
			.ctrlbit	= (1 << 12),
		},
		.reg_div = { .reg = S5P_CLKDIV_LCD1, .shift = 20, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_mmc",
			.id		= 0,
			.parent		= &clk_dout_mmc0.clk,
			.enable		= s5pv310_clksrc_mask_fsys_ctrl,
			.ctrlbit	= (1 << 0),
		},
		.reg_div = { .reg = S5P_CLKDIV_FSYS1, .shift = 8, .size = 8 },
	}, {
		.clk		= {
			.name		= "sclk_mmc",
			.id		= 1,
			.parent         = &clk_dout_mmc1.clk,
			.enable		= s5pv310_clksrc_mask_fsys_ctrl,
			.ctrlbit	= (1 << 4),
		},
		.reg_div = { .reg = S5P_CLKDIV_FSYS1, .shift = 24, .size = 8 },
	}, {
		.clk		= {
			.name		= "sclk_mmc",
			.id		= 2,
			.parent         = &clk_dout_mmc2.clk,
			.enable		= s5pv310_clksrc_mask_fsys_ctrl,
			.ctrlbit	= (1 << 8),
		},
		.reg_div = { .reg = S5P_CLKDIV_FSYS2, .shift = 8, .size = 8 },
	}, {
		.clk		= {
			.name		= "sclk_mmc",
			.id		= 3,
			.parent         = &clk_dout_mmc3.clk,
			.enable		= s5pv310_clksrc_mask_fsys_ctrl,
			.ctrlbit	= (1 << 12),
		},
		.reg_div = { .reg = S5P_CLKDIV_FSYS2, .shift = 24, .size = 8 },
	}, {
		.clk		= {
			.name		= "sclk_mshc",
			.id		= -1,
			.parent         = &clk_dout_mmc4.clk,
			.enable		= s5pv310_clksrc_mask_fsys_ctrl,
			.ctrlbit	= (1 << 16),
		},
		.reg_div = { .reg = S5P_CLKDIV_FSYS3, .shift = 8, .size = 8 },
	}, {
		.clk		= {
			.name		= "sclk_sata",
			.id		= -1,
			.enable		= s5pv310_clksrc_mask_fsys_ctrl,
			.ctrlbit	= (1 << 24),
		},
		.sources = &clkset_mout_corebus,
		.reg_src = { .reg = S5P_CLKSRC_FSYS, .shift = 24, .size = 1 },
		.reg_div = { .reg = S5P_CLKDIV_FSYS0, .shift = 20, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_spi",
			.id		= 0,
			.enable		= s5pv310_clksrc_mask_peril1_ctrl,
			.ctrlbit	= (1 << 16),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_PERIL1, .shift = 16, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_PERIL1, .shift = 8, .size = 8 },
	}, {
		.clk		= {
			.name		= "sclk_spi",
			.id		= 1,
			.enable		= s5pv310_clksrc_mask_peril1_ctrl,
			.ctrlbit	= (1 << 20),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_PERIL1, .shift = 20, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_PERIL1, .shift = 24, .size = 8 },
	}, {
		.clk		= {
			.name		= "sclk_spi",
			.id		= 2,
			.enable		= s5pv310_clksrc_mask_peril1_ctrl,
			.ctrlbit	= (1 << 24),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_PERIL1, .shift = 24, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_PERIL2, .shift = 8, .size = 8 },
	}, {
		.clk		= {
			.name		= "sclk_fimg2d",
			.id		= -1,
		},
		.sources = &clkset_mout_g2d,
		.reg_src = { .reg = S5P_CLKSRC_IMAGE, .shift = 8, .size = 1 },
		.reg_div = { .reg = S5P_CLKDIV_IMAGE, .shift = 0, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_mfc",
			.id		= -1,
		},
		.sources = &clkset_mout_mfc,
		.reg_src = { .reg = S5P_CLKSRC_MFC, .shift = 8, .size = 1 },
		.reg_div = { .reg = S5P_CLKDIV_MFC, .shift = 0, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_g3d",
			.id		= -1,
			.enable		= s5pv310_clk_ip_g3d_ctrl,
			.ctrlbit	= (1 << 0),
		},
		.sources = &clkset_mout_g3d,
		.reg_src = { .reg = S5P_CLKSRC_G3D, .shift = 8, .size = 1 },
		.reg_div = { .reg = S5P_CLKDIV_G3D, .shift = 0, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_pcm",
			.id		= 0,
			.parent		= &clk_sclk_audio0.clk,
		},
		.reg_div = { .reg = S5P_CLKDIV_MAUDIO, .shift = 4, .size = 8 },
	}, {
		.clk		= {
			.name		= "sclk_pcm",
			.id		= 1,
			.parent		= &clk_sclk_audio1.clk,
		},
		.reg_div = { .reg = S5P_CLKDIV_PERIL4, .shift = 4, .size = 8 },
	}, {
		.clk		= {
			.name		= "sclk_pcm",
			.id		= 2,
			.parent		= &clk_sclk_audio2.clk,
		},
		.reg_div = { .reg = S5P_CLKDIV_PERIL4, .shift = 20, .size = 8 },
	}, {
		.clk		= {
			.name		= "sclk_i2s",
			.id		= 1,
			.parent		= &clk_sclk_audio1.clk,
		},
		.reg_div = { .reg = S5P_CLKDIV_PERIL5, .shift = 0, .size = 6 },
	}, {
		.clk		= {
			.name		= "sclk_i2s",
			.id		= 2,
			.parent		= &clk_sclk_audio2.clk,
		},
		.reg_div = { .reg = S5P_CLKDIV_PERIL5, .shift = 8, .size = 6 },
	}, {
		.clk		= {
			.name		= "sclk_hpm",
			.id		= -1,
			.parent		= &clk_dout_copy.clk,
		},
		.reg_div = { .reg = S5P_CLKDIV_CPU1, .shift = 4, .size = 3 },
	}, {
		.clk		= {
			.name		= "sclk_pwi",
			.id		= -1,
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_DMC, .shift = 16, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_DMC1, .shift = 8, .size = 4 },
	},
};

/* Clock initialisation code */
static struct clksrc_clk *sysclks[] = {
	&clk_mout_apll,
	&clk_sclk_apll,
	&clk_mout_epll,
	&clk_mout_mpll,
	&clk_moutcore,
	&clk_coreclk,
	&clk_armclk,
	&clk_mout_corebus,
	&clk_sclk_dmc,
	&clk_aclk_cored,
	&clk_aclk_corep,
	&clk_aclk_acp,
	&clk_pclk_acp,
	&clk_vpllsrc,
	&clk_sclk_vpll,
	&clk_aclk_200,
	&clk_aclk_100,
	&clk_aclk_160,
	&clk_aclk_133,
	&clk_sclk_mipidphy4l,
	&clk_sclk_mipidphy2l,
	&clk_aclk_gdl,
	&clk_aclk_gdr,
	&clk_dout_mmc0,
	&clk_dout_mmc1,
	&clk_dout_mmc2,
	&clk_dout_mmc3,
	&clk_dout_mmc4,
	&clk_mout_audss,
	&clk_sclk_audss,
	&clk_mout_mfc0,
	&clk_mout_mfc1,
	&clk_sclk_audio0,
	&clk_sclk_audio1,
	&clk_sclk_audio2,
	&clk_sclk_spdif,
	&clk_mout_g2d0,
	&clk_mout_g2d1,
	&clk_mout_g3d0,
	&clk_mout_g3d1,
	&clk_sclk_dac,
	&clk_sclk_pixel,
	&clk_sclk_hdmi,
	&clk_dout_copy,
};

static unsigned long s5pv310_epll_get_rate(struct clk *clk)
{
	return clk->rate;
}

static u32 epll_div[][6] = {
	{  48000000, 0, 48, 3, 3, 0 },
	{  96000000, 0, 48, 3, 2, 0 },
	{ 144000000, 1, 72, 3, 2, 0 },
	{ 192000000, 0, 48, 3, 1, 0 },
	{ 288000000, 1, 72, 3, 1, 0 },
	{  84000000, 0, 42, 3, 2, 0 },
	{  50000000, 0, 50, 3, 3, 0 },
	{  80000000, 1, 80, 3, 3, 0 },
	{  32750000, 1, 65, 3, 4, 35127 },
	{  32768000, 1, 65, 3, 4, 35127 },
	{  49152000, 0, 49, 3, 3, 9961 },
	{  67737600, 1, 67, 3, 3, 48366 },
	{  73728000, 1, 73, 3, 3, 47710 },
	{  45158400, 0, 45, 3, 3, 10381 },
	{  45000000, 0, 45, 3, 3, 10355 },
	{  45158000, 0, 45, 3, 3, 10355 },
	{  49125000, 0, 49, 3, 3, 9961 },
	{  67738000, 1, 67, 3, 3, 48366 },
	{  73800000, 1, 73, 3, 3, 47710 },
	{  36000000, 1, 32, 3, 4, 0 },
	{  60000000, 1, 60, 3, 3, 0 },
	{  72000000, 1, 72, 3, 3, 0 },
	{ 191923200, 0, 47, 3, 1, 64278 },
	{ 180633600, 0, 45, 3, 1, 10381 },
};

static int s5pv310_epll_set_rate(struct clk *clk, unsigned long rate)
{
	unsigned int epll_con, epll_con_k;
	unsigned int i;

	/* Return if nothing changed */
	if (clk->rate == rate)
		return 0;

	epll_con = __raw_readl(S5P_EPLL_CON0);
	epll_con &= ~(0x1 << 27 | \
			PLL46XX_MDIV_MASK << PLL46XX_MDIV_SHIFT |   \
			PLL46XX_PDIV_MASK << PLL46XX_PDIV_SHIFT | \
			PLL46XX_SDIV_MASK << PLL46XX_SDIV_SHIFT);

	for (i = 0; i < ARRAY_SIZE(epll_div); i++) {
		if (epll_div[i][0] == rate) {
			epll_con_k = epll_div[i][5] << 0;
			epll_con |= epll_div[i][1] << 27;
			epll_con |= epll_div[i][2] << PLL46XX_MDIV_SHIFT;
			epll_con |= epll_div[i][3] << PLL46XX_PDIV_SHIFT;
			epll_con |= epll_div[i][4] << PLL46XX_SDIV_SHIFT;
			break;
		}
	}

	if (i == ARRAY_SIZE(epll_div)) {
		printk(KERN_ERR "%s: Invalid Clock EPLL Frequency\n",
				__func__);
		return -EINVAL;
	}

	__raw_writel(epll_con, S5P_EPLL_CON0);
	__raw_writel(epll_con_k, S5P_EPLL_CON1);

	clk->rate = rate;

	return 0;
}

static struct clk_ops s5pv310_epll_ops = {
	.get_rate = s5pv310_epll_get_rate,
	.set_rate = s5pv310_epll_set_rate,
};


struct vpll_div_data {
	u32 rate;
	u32 pdiv;
	u32 mdiv;
	u32 sdiv;
	u32 k;
	u32 mfr;
	u32 mrr;
	u32 vsel;

};

static struct vpll_div_data vpll_div[] = {
	{  54000000, 3, 53, 3, 1024, 0, 17, 0 },
	{ 108000000, 3, 53, 2, 1024, 0, 17, 0 },
#ifdef CONFIG_S5PV310_MSHC_VPLL_46MHZ
	{ 370882812, 3, 44, 0, 2417, 0, 14, 0 },
#endif
};

static unsigned long s5pv310_vpll_get_rate(struct clk *clk)
{
	return clk->rate;
}

static int s5pv310_vpll_set_rate(struct clk *clk, unsigned long rate)
{
	unsigned int vpll_con0, vpll_con1;
	unsigned int i;

	/* Return if nothing changed */
	if (clk->rate == rate)
		return 0;

	vpll_con0 = __raw_readl(S5P_VPLL_CON0);
	vpll_con0 &= ~(0x1 << 27 |					\
			PLL90XX_MDIV_MASK << PLL90XX_MDIV_SHIFT |	\
			PLL90XX_PDIV_MASK << PLL90XX_PDIV_SHIFT |	\
			PLL90XX_SDIV_MASK << PLL90XX_SDIV_SHIFT);

	vpll_con1 = __raw_readl(S5P_VPLL_CON1);
	vpll_con1 &= ~(0x1f << 24 |	\
			0x3f << 16 |	\
			0xfff << 0);

	for (i = 0; i < ARRAY_SIZE(vpll_div); i++) {
		if (vpll_div[i].rate == rate) {
			vpll_con0 |= vpll_div[i].vsel << 27;
			vpll_con0 |= vpll_div[i].pdiv << PLL90XX_PDIV_SHIFT;
			vpll_con0 |= vpll_div[i].mdiv << PLL90XX_MDIV_SHIFT;
			vpll_con0 |= vpll_div[i].sdiv << PLL90XX_SDIV_SHIFT;
			vpll_con1 |= vpll_div[i].mrr << 24;
			vpll_con1 |= vpll_div[i].mfr << 16;
			vpll_con1 |= vpll_div[i].k << 0;
			break;
		}
	}

	if (i == ARRAY_SIZE(vpll_div)) {
		printk(KERN_ERR "%s: Invalid Clock VPLL Frequency\n",
				__func__);
		return -EINVAL;
	}

	__raw_writel(vpll_con0, S5P_VPLL_CON0);
	__raw_writel(vpll_con1, S5P_VPLL_CON1);

	clk->rate = rate;

	return 0;
}

static struct clk_ops s5pv310_vpll_ops = {
	.get_rate = s5pv310_vpll_get_rate,
	.set_rate = s5pv310_vpll_set_rate,
};

static int xtal_rate;

static unsigned long s5pv310_fout_apll_get_rate(struct clk *clk)
{
	return s5p_get_pll45xx(xtal_rate, __raw_readl(S5P_APLL_CON0), pll_4508);
}

static struct clk_ops s5pv310_fout_apll_ops = {
	.get_rate = s5pv310_fout_apll_get_rate,
};

void __init_or_cpufreq s5pv310_setup_clocks(void)
{
	struct clk *xtal_clk;
	unsigned long apll;
	unsigned long mpll;
	unsigned long epll;
	unsigned long vpll;
	unsigned long xtal;
	unsigned long armclk;
	unsigned long sclk_dmc;
	unsigned long aclk_200;
	unsigned long aclk_100;
	unsigned long aclk_160;
	unsigned long aclk_133;
	unsigned int ptr;
	unsigned long vpllsrc;

	printk(KERN_DEBUG "%s: registering clocks\n", __func__);

	xtal_clk = clk_get(NULL, "xtal");
	BUG_ON(IS_ERR(xtal_clk));

	xtal = clk_get_rate(xtal_clk);

	xtal_rate = xtal;

	clk_put(xtal_clk);

	printk(KERN_DEBUG "%s: xtal is %ld\n", __func__, xtal);

#ifndef CONFIG_S5PV310_FPGA
	apll = s5p_get_pll45xx(xtal, __raw_readl(S5P_APLL_CON0), pll_4508);
	mpll = s5p_get_pll45xx(xtal, __raw_readl(S5P_MPLL_CON0), pll_4508);
	epll = s5p_get_pll46xx(xtal, __raw_readl(S5P_EPLL_CON0),
				__raw_readl(S5P_EPLL_CON1), pll_4600);

	vpllsrc = clk_get_rate(&clk_vpllsrc.clk);
	vpll = s5p_get_pll46xx(vpllsrc, __raw_readl(S5P_VPLL_CON0),
				__raw_readl(S5P_VPLL_CON1), pll_4650);
#else
	apll = xtal;
	mpll = xtal;
	epll = xtal;
	vpll = xtal;
#endif

	clk_fout_apll.ops = &s5pv310_fout_apll_ops;
	clk_fout_mpll.rate = mpll;
	clk_fout_epll.rate = epll;
	clk_fout_vpll.rate = vpll;

	armclk = clk_get_rate(&clk_armclk.clk);
	sclk_dmc = clk_get_rate(&clk_sclk_dmc.clk);
	aclk_200 = clk_get_rate(&clk_aclk_200.clk);
	aclk_100 = clk_get_rate(&clk_aclk_100.clk);
	aclk_160 = clk_get_rate(&clk_aclk_160.clk);
	aclk_133 = clk_get_rate(&clk_aclk_133.clk);

	printk(KERN_INFO "S5PV310: ARMCLK=%ld, DMC=%ld, ACLK200=%ld\n"
			 "ACLK100=%ld, ACLK160=%ld, ACLK133=%ld\n",
			  armclk, sclk_dmc, aclk_200,
			  aclk_100, aclk_160, aclk_133);

	clk_f.rate = armclk;
	clk_h.rate = sclk_dmc;
	clk_p.rate = aclk_100;

	for (ptr = 0; ptr < ARRAY_SIZE(clksrcs); ptr++)
		s3c_set_clksrc(&clksrcs[ptr], true);

	clk_fout_epll.enable = s5pv310_clk_epll_ctrl;
	clk_fout_epll.ops = &s5pv310_epll_ops;

#ifdef CONFIG_S5PV310_MSHC_EPLL_45MHZ
	clk_set_parent(&clk_dout_mmc4.clk, &clk_mout_epll.clk);
#endif
#ifdef CONFIG_S5PV310_MSHC_VPLL_46MHZ
	clk_set_parent(&clk_dout_mmc4.clk, &clk_sclk_vpll.clk);
	clk_set_parent(&clk_sclk_vpll.clk, &clk_fout_vpll);
#endif

	clk_set_parent(&clk_sclk_audss.clk, &clk_mout_audss.clk);
	clk_set_parent(&clk_mout_audss.clk, &clk_fout_epll);
	clk_set_parent(&clk_sclk_audio0.clk, &clk_mout_epll.clk);
	clk_set_parent(&clk_sclk_audio1.clk, &clk_mout_epll.clk);
	clk_set_parent(&clk_sclk_audio2.clk, &clk_mout_epll.clk);
	clk_set_parent(&clk_mout_epll.clk, &clk_fout_epll);

	clk_fout_vpll.enable = s5pv310_clk_vpll_ctrl;
	clk_fout_vpll.ops = &s5pv310_vpll_ops;

	clk_set_rate(&clk_sclk_apll.clk, 100000000);
}

static struct clk *clks[] __initdata = {
	&clk_sclk_hdmi27m,
	&clk_sclk_hdmiphy,
};

void __init s5pv310_register_clocks(void)
{
	struct clk *clkp;
	int ret;
	int ptr;

	ret = s3c24xx_register_clocks(clks, ARRAY_SIZE(clks));
	if (ret > 0)
		printk(KERN_ERR "Failed to register %u clocks\n", ret);

	for (ptr = 0; ptr < ARRAY_SIZE(sysclks); ptr++)
		s3c_register_clksrc(sysclks[ptr], 1);

	s3c_register_clksrc(clksrcs, ARRAY_SIZE(clksrcs));
	s3c_register_clocks(init_clocks, ARRAY_SIZE(init_clocks));
	s3c_register_clocks(init_dmaclocks, ARRAY_SIZE(init_dmaclocks));

	clkp = init_clocks_disable;
	for (ptr = 0; ptr < ARRAY_SIZE(init_clocks_disable); ptr++, clkp++) {
		ret = s3c24xx_register_clock(clkp);
		if (ret < 0) {
			printk(KERN_ERR "Failed to register clock %s (%d)\n",
			       clkp->name, ret);
		}
		(clkp->enable)(clkp, 0);
	}

	s3c_pwmclk_init();
}
