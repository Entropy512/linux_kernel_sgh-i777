/* linux/drivers/video/samsung/s3cfb_fimd6x.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Register interface file for Samsung Display Controller (FIMD) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fb.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <mach/map.h>
#include <plat/clock.h>
#include <plat/fb.h>
#include <plat/regs-fb.h>
#include <mach/mipi_ddi.h>

#include "s3cfb.h"

void s3cfb_check_line_count(struct s3cfb_global *ctrl)
{
	int timeout = 30 * 5300;
	int i = 0;

	do {
		if (!(readl(ctrl->regs + S3C_VIDCON1) & 0x7ff0000))
			break;
		i++;
	} while (i < timeout);

	if (i == timeout) {
		dev_err(ctrl->dev, "line count mismatch\n");
		s3cfb_display_on(ctrl);
	}
}

int s3cfb_set_output(struct s3cfb_global *ctrl)
{
	u32 cfg;

	cfg = readl(ctrl->regs + S3C_VIDCON0);
	cfg &= ~S3C_VIDCON0_VIDOUT_MASK;

	if (ctrl->output == OUTPUT_RGB)
		cfg |= S3C_VIDCON0_VIDOUT_RGB;
	else if (ctrl->output == OUTPUT_ITU)
		cfg |= S3C_VIDCON0_VIDOUT_ITU;
	else if (ctrl->output == OUTPUT_I80LDI0)
		cfg |= S3C_VIDCON0_VIDOUT_I80LDI0;
	else if (ctrl->output == OUTPUT_I80LDI1)
		cfg |= S3C_VIDCON0_VIDOUT_I80LDI1;
	else if (ctrl->output == OUTPUT_WB_RGB)
		cfg |= S3C_VIDCON0_VIDOUT_WB_RGB;
	else if (ctrl->output == OUTPUT_WB_I80LDI0)
		cfg |= S3C_VIDCON0_VIDOUT_WB_I80LDI0;
	else if (ctrl->output == OUTPUT_WB_I80LDI1)
		cfg |= S3C_VIDCON0_VIDOUT_WB_I80LDI1;
	else {
		dev_err(ctrl->dev, "invalid output type: %d\n", ctrl->output);
		return -EINVAL;
	}

	writel(cfg, ctrl->regs + S3C_VIDCON0);

	cfg = readl(ctrl->regs + S3C_VIDCON2);
	cfg &= ~(S3C_VIDCON2_WB_MASK | S3C_VIDCON2_TVFORMATSEL_MASK | \
					S3C_VIDCON2_TVFORMATSEL_YUV_MASK);

	if (ctrl->output == OUTPUT_RGB)
		cfg |= S3C_VIDCON2_WB_DISABLE;
	else if (ctrl->output == OUTPUT_ITU)
		cfg |= S3C_VIDCON2_WB_DISABLE;
	else if (ctrl->output == OUTPUT_I80LDI0)
		cfg |= S3C_VIDCON2_WB_DISABLE;
	else if (ctrl->output == OUTPUT_I80LDI1)
		cfg |= S3C_VIDCON2_WB_DISABLE;
	else if (ctrl->output == OUTPUT_WB_RGB)
		cfg |= (S3C_VIDCON2_WB_ENABLE | S3C_VIDCON2_TVFORMATSEL_SW | \
					S3C_VIDCON2_TVFORMATSEL_YUV444);
	else if (ctrl->output == OUTPUT_WB_I80LDI0)
		cfg |= (S3C_VIDCON2_WB_ENABLE | S3C_VIDCON2_TVFORMATSEL_SW | \
					S3C_VIDCON2_TVFORMATSEL_YUV444);
	else if (ctrl->output == OUTPUT_WB_I80LDI1)
		cfg |= (S3C_VIDCON2_WB_ENABLE | S3C_VIDCON2_TVFORMATSEL_SW | \
					S3C_VIDCON2_TVFORMATSEL_YUV444);
	else {
		dev_err(ctrl->dev, "invalid output type: %d\n", ctrl->output);
		return -EINVAL;
	}

	writel(cfg, ctrl->regs + S3C_VIDCON2);

	return 0;
}

#ifdef CONFIG_FB_S3C_MIPI_LCD
int s3cfb_set_free_run(struct s3cfb_global *ctrl, int onoff)
{
	__u32 cfg;

	cfg = readl(ctrl->regs + S3C_VIDCON0);
	cfg &= ~S3C_VIDCON0_VCLKEN_MASK;

	if (onoff)
		cfg |= S3C_VIDCON0_VCLKEN_FREERUN;
	else
		cfg |= S3C_VIDCON0_VCLKEN_NORMAL;

	writel(cfg, ctrl->regs + S3C_VIDCON0);

	dev_dbg(ctrl->dev, "free_run = 0x%x\n", readl(ctrl->regs + S3C_VIDCON0));

	return 0;
}

int s3cfb_enable_mipi_dsi_mode(struct s3cfb_global *ctrl, unsigned int enable)
{
	unsigned int cfg;

	cfg = readl(ctrl->regs + S3C_VIDCON0);

	cfg &= ~(S3C_VIDCON0_DSI_ENABLE);

	if (enable)
		cfg |= S3C_VIDCON0_DSI_ENABLE;
	else
		cfg |= S3C_VIDCON0_DSI_DISABLE;

	writel(cfg, ctrl->regs + S3C_VIDCON0);

	dev_dbg(ctrl->dev, "0x%x\n", readl(ctrl->regs + S3C_VIDCON0));

	return 0;
}

int s3cfb_set_cpu_interface_timing(struct s3cfb_global *ctrl, unsigned char ldi)
{
	unsigned int cpu_if_time_reg, cpu_if_time_val;
	struct s3cfb_lcd *lcd = NULL;
	struct s3cfb_cpu_timing *cpu_timing = NULL;

	lcd = ctrl->lcd;
	if (lcd == NULL) {
		dev_err(ctrl->dev, "s3cfb_lcd is NULL.\n");
		return -1;
	}

	cpu_timing = &lcd->cpu_timing;
	if (cpu_timing == NULL) {
		dev_err(ctrl->dev, "cpu_timing is NULL.\n");
		return -1;
	}

	/* get offset of I80IFCON register. */
	cpu_if_time_reg = (ldi == DDI_MAIN_LCD) ? S3C_I80IFCONA0 : S3C_I80IFCONA1;

	cpu_if_time_val = readl(ctrl->regs + cpu_if_time_reg);

	cpu_if_time_val = S3C_LCD_CS_SETUP(cpu_timing->cs_setup) |
		S3C_LCD_WR_SETUP(cpu_timing->wr_setup) |
		S3C_LCD_WR_ACT(cpu_timing->wr_act) |
		S3C_LCD_WR_HOLD(cpu_timing->wr_hold) |
		S3C_RSPOL_LOW | /* in case of LCD MIPI module */
		/* S3C_RSPOL_HIGH | */
		S3C_I80IFEN_ENABLE;

	writel(cpu_if_time_val, ctrl->regs + cpu_if_time_reg);

	return 0;
}

void s3cfb_set_trigger(struct s3cfb_global *ctrl)
{
	u32 reg = 0;

	reg = readl(ctrl->regs + S3C_TRIGCON);

	reg |= 1 << 0 | 1 << 1;

	writel(reg, ctrl->regs + S3C_TRIGCON);
}

u8 s3cfb_is_frame_done(struct s3cfb_global *ctrl)
{
	u32 reg = 0;

	reg = readl(ctrl->regs + S3C_TRIGCON);

	/* frame done func is valid only when TRIMODE[0] is set to 1. */

	return (((reg & (0x1 << 2)) == (0x1 << 2)) ? 1 : 0);
}

int s3cfb_set_auto_cmd_rate(struct s3cfb_global *ctrl,
	unsigned char cmd_rate, unsigned char ldi)
{
	unsigned int cmd_rate_val;
	unsigned int i80_if_con_reg, i80_if_con_reg_val;

	i80_if_con_reg = (ldi == DDI_MAIN_LCD) ? S3C_I80IFCONB0 : S3C_I80IFCONB1;

	cmd_rate_val = (cmd_rate == DISABLE_AUTO_FRM) ? (0x0 << 0) :
		(cmd_rate == PER_TWO_FRM) ? (0x1 << 0) :
		(cmd_rate == PER_FOUR_FRM) ? (0x2 << 0) :
		(cmd_rate == PER_SIX_FRM) ? (0x3 << 0) :
		(cmd_rate == PER_EIGHT_FRM) ? (0x4 << 0) :
		(cmd_rate == PER_TEN_FRM) ? (0x5 << 0) :
		(cmd_rate == PER_TWELVE_FRM) ? (0x6 << 0) :
		(cmd_rate == PER_FOURTEEN_FRM) ? (0x7 << 0) :
		(cmd_rate == PER_SIXTEEN_FRM) ? (0x8 << 0) :
		(cmd_rate == PER_EIGHTEEN_FRM) ? (0x9 << 0) :
		(cmd_rate == PER_TWENTY_FRM) ? (0xa << 0) :
		(cmd_rate == PER_TWENTY_TWO_FRM) ? (0xb << 0) :
		(cmd_rate == PER_TWENTY_FOUR_FRM) ? (0xc << 0) :
		(cmd_rate == PER_TWENTY_SIX_FRM) ? (0xd << 0) :
		(cmd_rate == PER_TWENTY_EIGHT_FRM) ? (0xe << 0) : (0xf << 0);

	i80_if_con_reg_val = readl(ctrl->regs + i80_if_con_reg);
	i80_if_con_reg_val &= ~(0xf << 0);
	i80_if_con_reg_val |= cmd_rate_val;
	writel(i80_if_con_reg_val, ctrl->regs + i80_if_con_reg);

	dev_dbg(ctrl->dev, "0x%x\n", ((u32) ctrl->regs) + i80_if_con_reg);

	return 0;
}
#endif

int s3cfb_set_display_mode(struct s3cfb_global *ctrl)
{
	u32 cfg;

	cfg = readl(ctrl->regs + S3C_VIDCON0);
	cfg &= ~S3C_VIDCON0_PNRMODE_MASK;
	cfg |= (ctrl->rgb_mode << S3C_VIDCON0_PNRMODE_SHIFT);
	writel(cfg, ctrl->regs + S3C_VIDCON0);

	return 0;
}

int s3cfb_display_on(struct s3cfb_global *ctrl)
{
	u32 cfg;

	cfg = readl(ctrl->regs + S3C_VIDCON0);
	cfg |= (S3C_VIDCON0_ENVID_ENABLE | S3C_VIDCON0_ENVID_F_ENABLE);
	writel(cfg, ctrl->regs + S3C_VIDCON0);

	dev_dbg(ctrl->dev, "global display is on\n");

	return 0;
}

int s3cfb_display_off(struct s3cfb_global *ctrl)
{
	u32 cfg;

	cfg = readl(ctrl->regs + S3C_VIDCON0);
	cfg &= ~S3C_VIDCON0_ENVID_ENABLE;
	writel(cfg, ctrl->regs + S3C_VIDCON0);

	cfg &= ~S3C_VIDCON0_ENVID_F_ENABLE;
	writel(cfg, ctrl->regs + S3C_VIDCON0);

	dev_dbg(ctrl->dev, "global display is off\n");

	return 0;
}

int s3cfb_frame_off(struct s3cfb_global *ctrl)
{
	u32 cfg;

	cfg = readl(ctrl->regs + S3C_VIDCON0);
	cfg &= ~S3C_VIDCON0_ENVID_F_ENABLE;
	writel(cfg, ctrl->regs + S3C_VIDCON0);

	dev_dbg(ctrl->dev, "current frame display is off\n");

	return 0;
}

int s3cfb_set_clock(struct s3cfb_global *ctrl)
{
	struct s3c_platform_fb *pdata = to_fb_plat(ctrl->dev);
	u32 cfg, maxclk, src_clk, vclk, div;

	maxclk = 86 * 1000000;

	cfg = readl(ctrl->regs + S3C_VIDCON0);

#ifdef CONFIG_FB_S3C_MIPI_LCD
	if (pdata->hw_ver == 0x70) {
		cfg &= ~(S3C_VIDCON0_CLKVALUP_MASK |
			S3C_VIDCON0_VCLKEN_MASK);
		cfg |= (S3C_VIDCON0_CLKVALUP_START_FRAME |
			S3C_VIDCON0_VCLKEN_NORMAL);

		src_clk = clk_get_rate(ctrl->clock);
		printk(KERN_INFO "FIMD src sclk = %d\n", src_clk);
	} else {
		cfg &= ~(S3C_VIDCON0_CLKSEL_MASK |
			S3C_VIDCON0_CLKVALUP_MASK |
			S3C_VIDCON0_VCLKEN_MASK |
			S3C_VIDCON0_CLKDIR_MASK);
		cfg |= (S3C_VIDCON0_CLKVALUP_START_FRAME |
			S3C_VIDCON0_VCLKEN_NORMAL |
			S3C_VIDCON0_CLKDIR_DIVIDED);
#else
	if (pdata->hw_ver == 0x70) {
		cfg &= ~(S3C_VIDCON0_CLKVALUP_MASK |
			S3C_VIDCON0_VCLKEN_MASK);
		cfg |= (S3C_VIDCON0_CLKVALUP_ALWAYS |
			S3C_VIDCON0_VCLKEN_NORMAL);

		src_clk = clk_get_rate(ctrl->clock);
		printk(KERN_INFO "FIMD src sclk = %d\n", src_clk);
	} else {
		cfg &= ~(S3C_VIDCON0_CLKSEL_MASK |
			S3C_VIDCON0_CLKVALUP_MASK |
			S3C_VIDCON0_VCLKEN_MASK |
			S3C_VIDCON0_CLKDIR_MASK);
		cfg |= (S3C_VIDCON0_CLKVALUP_ALWAYS |
			S3C_VIDCON0_VCLKEN_NORMAL |
			S3C_VIDCON0_CLKDIR_DIVIDED);
#endif
		if (strcmp(pdata->clk_name, "sclk_fimd") == 0) {
			cfg |= S3C_VIDCON0_CLKSEL_SCLK;
			src_clk = clk_get_rate(ctrl->clock);
			printk(KERN_INFO "FIMD src sclk = %d\n", src_clk);

		} else {
			cfg |= S3C_VIDCON0_CLKSEL_HCLK;
			src_clk = ctrl->clock->parent->rate;
			printk(KERN_INFO "FIMD src hclk = %d\n", src_clk);
		}
	}

	vclk = PICOS2KHZ(ctrl->fb[pdata->default_win]->var.pixclock) * 1000;

	if (vclk > maxclk) {
		dev_info(ctrl->dev, "vclk(%d) should be smaller than %d\n",
			vclk, maxclk);
		/* vclk = maxclk; */
	}

	div = src_clk / vclk;
#ifndef CONFIG_FB_S3C_MIPI_LCD
	if (src_clk % vclk)
		div++;
#endif

	if ((src_clk/div) > maxclk)
		dev_info(ctrl->dev, "vclk(%d) should be smaller than %d Hz\n",
			src_clk/div, maxclk);

	cfg |= S3C_VIDCON0_CLKVAL_F(div - 1);
	writel(cfg, ctrl->regs + S3C_VIDCON0);

	dev_dbg(ctrl->dev, "parent clock: %d, vclk: %d, vclk div: %d\n",
			src_clk, vclk, div);

	return 0;
}

int s3cfb_set_polarity(struct s3cfb_global *ctrl)
{
	struct s3cfb_lcd_polarity *pol;
	u32 cfg;

	pol = &ctrl->lcd->polarity;
	cfg = 0;

	/* Set VCLK hold scheme */
	cfg &= S3C_VIDCON1_FIXVCLK_MASK;
	cfg |= S3C_VIDCON1_FIXVCLK_VCLK_RUN;

	if (pol->rise_vclk)
		cfg |= S3C_VIDCON1_IVCLK_RISING_EDGE;

	if (pol->inv_hsync)
		cfg |= S3C_VIDCON1_IHSYNC_INVERT;

	if (pol->inv_vsync)
		cfg |= S3C_VIDCON1_IVSYNC_INVERT;

	if (pol->inv_vden)
		cfg |= S3C_VIDCON1_IVDEN_INVERT;

	writel(cfg, ctrl->regs + S3C_VIDCON1);

	return 0;
}

int s3cfb_set_timing(struct s3cfb_global *ctrl)
{
	struct s3cfb_lcd_timing *time;
	u32 cfg;

	time = &ctrl->lcd->timing;
	cfg = 0;

	cfg |= S3C_VIDTCON0_VBPDE(time->v_bpe - 1);
	cfg |= S3C_VIDTCON0_VBPD(time->v_bp - 1);
	cfg |= S3C_VIDTCON0_VFPD(time->v_fp - 1);
	cfg |= S3C_VIDTCON0_VSPW(time->v_sw - 1);

	writel(cfg, ctrl->regs + S3C_VIDTCON0);

	cfg = 0;

	cfg |= S3C_VIDTCON1_VFPDE(time->v_fpe - 1);
	cfg |= S3C_VIDTCON1_HBPD(time->h_bp - 1);
	cfg |= S3C_VIDTCON1_HFPD(time->h_fp - 1);
	cfg |= S3C_VIDTCON1_HSPW(time->h_sw - 1);

	writel(cfg, ctrl->regs + S3C_VIDTCON1);

	return 0;
}

int s3cfb_set_lcd_size(struct s3cfb_global *ctrl)
{
	u32 cfg = 0;

#ifdef CONFIG_FB_S3C_WA101S
	cfg |= S3C_VIDTCON2_HOZVAL(ctrl->lcd->width - 1 + 6);
#else
	cfg |= S3C_VIDTCON2_HOZVAL(ctrl->lcd->width - 1);
#endif

	cfg |= S3C_VIDTCON2_LINEVAL(ctrl->lcd->height - 1);

	writel(cfg, ctrl->regs + S3C_VIDTCON2);

	return 0;
}

int s3cfb_set_global_interrupt(struct s3cfb_global *ctrl, int enable)
{
	u32 cfg = 0;

	cfg = readl(ctrl->regs + S3C_VIDINTCON0);
	cfg &= ~(S3C_VIDINTCON0_INTFRMEN_ENABLE | S3C_VIDINTCON0_INT_ENABLE);

	if (enable) {
		dev_dbg(ctrl->dev, "video interrupt is on\n");
		cfg |= (S3C_VIDINTCON0_INTFRMEN_ENABLE |
			S3C_VIDINTCON0_INT_ENABLE);
	} else {
		dev_dbg(ctrl->dev, "video interrupt is off\n");
		cfg |= (S3C_VIDINTCON0_INTFRMEN_DISABLE |
			S3C_VIDINTCON0_INT_DISABLE);
	}

	writel(cfg, ctrl->regs + S3C_VIDINTCON0);

	return 0;
}

int s3cfb_set_vsync_interrupt(struct s3cfb_global *ctrl, int enable)
{
	u32 cfg = 0;

	cfg = readl(ctrl->regs + S3C_VIDINTCON0);
	cfg &= ~S3C_VIDINTCON0_FRAMESEL0_MASK;

	if (enable) {
		dev_dbg(ctrl->dev, "vsync interrupt is on\n");
		cfg |= S3C_VIDINTCON0_FRAMESEL0_VSYNC;
	} else {
		dev_dbg(ctrl->dev, "vsync interrupt is off\n");
		cfg &= ~S3C_VIDINTCON0_FRAMESEL0_VSYNC;
	}

	writel(cfg, ctrl->regs + S3C_VIDINTCON0);

	return 0;
}

int s3cfb_get_vsync_interrupt(struct s3cfb_global *ctrl)
{
	u32 cfg = 0;

	cfg = readl(ctrl->regs + S3C_VIDINTCON0);
	cfg &= S3C_VIDINTCON0_FRAMESEL0_VSYNC;

	if (cfg & S3C_VIDINTCON0_FRAMESEL0_VSYNC) {
		dev_dbg(ctrl->dev, "vsync interrupt is on\n");
		return 1;
	} else {
		dev_dbg(ctrl->dev, "vsync interrupt is off\n");
		return 0;
	}
}


#ifdef CONFIG_FB_S3C_TRACE_UNDERRUN
int s3cfb_set_fifo_interrupt(struct s3cfb_global *ctrl, int enable)
{
	u32 cfg = 0;

	cfg = readl(ctrl->regs + S3C_VIDINTCON0);

	cfg &= ~(S3C_VIDINTCON0_FIFOSEL_MASK | S3C_VIDINTCON0_FIFOLEVEL_MASK);
	cfg |= (S3C_VIDINTCON0_FIFOSEL_ALL | S3C_VIDINTCON0_FIFOLEVEL_EMPTY);

	if (enable) {
		dev_dbg(ctrl->dev, "fifo interrupt is on\n");
		cfg |= (S3C_VIDINTCON0_INTFIFO_ENABLE |
			S3C_VIDINTCON0_INT_ENABLE);
	} else {
		dev_dbg(ctrl->dev, "fifo interrupt is off\n");
		cfg &= ~(S3C_VIDINTCON0_INTFIFO_ENABLE |
			S3C_VIDINTCON0_INT_ENABLE);
	}

	writel(cfg, ctrl->regs + S3C_VIDINTCON0);

	return 0;
}
#endif

int s3cfb_clear_interrupt(struct s3cfb_global *ctrl)
{
	u32 cfg = 0;

	cfg = readl(ctrl->regs + S3C_VIDINTCON1);

	if (cfg & S3C_VIDINTCON1_INTFIFOPEND)
		dev_info(ctrl->dev, "fifo underrun occur\n");

	cfg |= (S3C_VIDINTCON1_INTVPPEND | S3C_VIDINTCON1_INTI80PEND |
		S3C_VIDINTCON1_INTFRMPEND | S3C_VIDINTCON1_INTFIFOPEND);

	writel(cfg, ctrl->regs + S3C_VIDINTCON1);

	return 0;
}

int s3cfb_channel_localpath_on(struct s3cfb_global *ctrl, int id)
{
	struct s3c_platform_fb *pdata = to_fb_plat(ctrl->dev);
	u32 cfg;

	if ((pdata->hw_ver == 0x62) || (pdata->hw_ver == 0x70)) {
		cfg = readl(ctrl->regs + S3C_WINSHMAP);
		cfg |= S3C_WINSHMAP_LOCAL_ENABLE(id);
		writel(cfg, ctrl->regs + S3C_WINSHMAP);
	}

	dev_dbg(ctrl->dev, "[fb%d] local path enabled\n", id);

	return 0;
}

int s3cfb_channel_localpath_off(struct s3cfb_global *ctrl, int id)
{
	struct s3c_platform_fb *pdata = to_fb_plat(ctrl->dev);
	u32 cfg;

	if ((pdata->hw_ver == 0x62) || (pdata->hw_ver == 0x70)) {
		cfg = readl(ctrl->regs + S3C_WINSHMAP);
		cfg &= ~S3C_WINSHMAP_LOCAL_DISABLE(id);
		writel(cfg, ctrl->regs + S3C_WINSHMAP);
	}

	dev_dbg(ctrl->dev, "[fb%d] local path disabled\n", id);

	return 0;
}

int s3cfb_window_on(struct s3cfb_global *ctrl, int id)
{
	struct s3c_platform_fb *pdata = to_fb_plat(ctrl->dev);
	u32 cfg;

	if ((pdata->hw_ver == 0x62) || (pdata->hw_ver == 0x70)) {
		cfg = readl(ctrl->regs + S3C_WINSHMAP);
		cfg |= S3C_WINSHMAP_CH_ENABLE(id);
		writel(cfg, ctrl->regs + S3C_WINSHMAP);
	}

	cfg = readl(ctrl->regs + S3C_WINCON(id));
	cfg |= S3C_WINCON_ENWIN_ENABLE;
	writel(cfg, ctrl->regs + S3C_WINCON(id));

	dev_dbg(ctrl->dev, "[fb%d] turn on\n", id);

	return 0;
}

int s3cfb_window_off(struct s3cfb_global *ctrl, int id)
{
	struct s3c_platform_fb *pdata = to_fb_plat(ctrl->dev);
	u32 cfg;

	cfg = readl(ctrl->regs + S3C_WINCON(id));
	cfg &= ~(S3C_WINCON_ENWIN_ENABLE | S3C_WINCON_DATAPATH_MASK);
	cfg |= S3C_WINCON_DATAPATH_DMA;
	writel(cfg, ctrl->regs + S3C_WINCON(id));

	if ((pdata->hw_ver == 0x62) || (pdata->hw_ver == 0x70)) {
		cfg = readl(ctrl->regs + S3C_WINSHMAP);
		cfg &= ~S3C_WINSHMAP_CH_DISABLE(id);
		writel(cfg, ctrl->regs + S3C_WINSHMAP);
	}

	dev_dbg(ctrl->dev, "[fb%d] turn off\n", id);

	return 0;
}

int s3cfb_win_map_on(struct s3cfb_global *ctrl, int id, int color)
{
	u32 cfg = 0;

	cfg |= S3C_WINMAP_ENABLE;
	cfg |= S3C_WINMAP_COLOR(color);
	writel(cfg, ctrl->regs + S3C_WINMAP(id));

	dev_dbg(ctrl->dev, "[fb%d] win map on : 0x%08x\n", id, color);

	return 0;
}

int s3cfb_win_map_off(struct s3cfb_global *ctrl, int id)
{
	writel(0, ctrl->regs + S3C_WINMAP(id));

	dev_dbg(ctrl->dev, "[fb%d] win map off\n", id);

	return 0;
}

int s3cfb_set_window_control(struct s3cfb_global *ctrl, int id)
{
	struct s3c_platform_fb *pdata = to_fb_plat(ctrl->dev);
	struct fb_info *fb = ctrl->fb[id];
	struct fb_var_screeninfo *var = &fb->var;
	struct s3cfb_window *win = fb->par;
	u32 cfg;
	u32 shw;

	if ((pdata->hw_ver == 0x62) || (pdata->hw_ver == 0x70)) {
		shw = readl(ctrl->regs + S3C_WINSHMAP);
		shw |= S3C_WINSHMAP_PROTECT(id);
		writel(shw, ctrl->regs + S3C_WINSHMAP);
	}

	cfg = readl(ctrl->regs + S3C_WINCON(id));

	cfg &= ~(S3C_WINCON_BITSWP_ENABLE | S3C_WINCON_BYTESWP_ENABLE |
		S3C_WINCON_HAWSWP_ENABLE | S3C_WINCON_WSWP_ENABLE |
		S3C_WINCON_BURSTLEN_MASK | S3C_WINCON_BPPMODE_MASK |
		S3C_WINCON_INRGB_MASK | S3C_WINCON_DATAPATH_MASK);

	if (win->path != DATA_PATH_DMA) {
		dev_dbg(ctrl->dev, "[fb%d] data path: fifo\n", id);

		cfg |= S3C_WINCON_DATAPATH_LOCAL;

		if (win->path == DATA_PATH_FIFO) {
			cfg |= S3C_WINCON_INRGB_RGB;
			cfg |= S3C_WINCON_BPPMODE_24BPP_888;
		} else if (win->path == DATA_PATH_IPC) {
			cfg |= S3C_WINCON_INRGB_YUV;
			cfg |= S3C_WINCON_BPPMODE_24BPP_888;
		}

		if (id == 1) {
			cfg &= ~(S3C_WINCON1_LOCALSEL_MASK |
				S3C_WINCON1_VP_ENABLE);

			if (win->local_channel == 0) {
				cfg |= S3C_WINCON1_LOCALSEL_FIMC1;
			} else {
				cfg |= (S3C_WINCON1_LOCALSEL_VP |
					S3C_WINCON1_VP_ENABLE);
			}
		}
	} else {
		dev_dbg(ctrl->dev, "[fb%d] data path: dma\n", id);

		cfg |= S3C_WINCON_DATAPATH_DMA;

		if (fb->var.bits_per_pixel == 16 && pdata->swap & FB_SWAP_HWORD)
			cfg |= S3C_WINCON_HAWSWP_ENABLE;

		if (fb->var.bits_per_pixel == 32 && pdata->swap & FB_SWAP_WORD)
			cfg |= S3C_WINCON_WSWP_ENABLE;

		/* dma burst */
		if (win->dma_burst == 4)
			cfg |= S3C_WINCON_BURSTLEN_4WORD;
		else if (win->dma_burst == 8)
			cfg |= S3C_WINCON_BURSTLEN_8WORD;
		else
			cfg |= S3C_WINCON_BURSTLEN_16WORD;

		/* bpp mode set */
		switch (fb->var.bits_per_pixel) {
		case 16:
			if (var->transp.length == 1) {
				dev_dbg(ctrl->dev,
					"[fb%d] bpp mode: A1-R5-G5-B5\n", id);
				cfg |= S3C_WINCON_BPPMODE_16BPP_A555;
			} else if (var->transp.length == 4) {
				dev_dbg(ctrl->dev,
					"[fb%d] bpp mode: A4-R4-G4-B4\n", id);
				cfg |= S3C_WINCON_BPPMODE_16BPP_A444;
			} else {
				dev_dbg(ctrl->dev,
					"[fb%d] bpp mode: R5-G6-B5\n", id);
				cfg |= S3C_WINCON_BPPMODE_16BPP_565;
			}
			break;

		case 24: /* packed 24 bpp: nothing to do for 6.x fimd */
			break;

		case 32:
			if (var->transp.length == 0) {
				dev_dbg(ctrl->dev,
					"[fb%d] bpp mode: R8-G8-B8\n", id);
				cfg |= S3C_WINCON_BPPMODE_24BPP_888;
			} else {
				dev_dbg(ctrl->dev,
					"[fb%d] bpp mode: A8-R8-G8-B8\n", id);
				cfg |= S3C_WINCON_BPPMODE_32BPP;
			}
			break;
		}
	}

	writel(cfg, ctrl->regs + S3C_WINCON(id));

	if ((pdata->hw_ver == 0x62) || (pdata->hw_ver == 0x70)) {
		shw = readl(ctrl->regs + S3C_WINSHMAP);
		shw &= ~(S3C_WINSHMAP_PROTECT(id));
		writel(shw, ctrl->regs + S3C_WINSHMAP);
	}

	return 0;
}

int s3cfb_set_buffer_address(struct s3cfb_global *ctrl, int id)
{
	struct fb_fix_screeninfo *fix = &ctrl->fb[id]->fix;
	struct fb_var_screeninfo *var = &ctrl->fb[id]->var;
	struct s3c_platform_fb *pdata = to_fb_plat(ctrl->dev);
	dma_addr_t start_addr = 0, end_addr = 0;
	u32 shw;

	if (fix->smem_start) {
		start_addr = fix->smem_start + ((var->xres_virtual *
				var->yoffset + var->xoffset) *
				(var->bits_per_pixel / 8));

		end_addr = start_addr + fix->line_length * var->yres;
	}

	if ((pdata->hw_ver == 0x62) || (pdata->hw_ver == 0x70)) {
		shw = readl(ctrl->regs + S3C_WINSHMAP);
		shw |= S3C_WINSHMAP_PROTECT(id);
		writel(shw, ctrl->regs + S3C_WINSHMAP);
	}

	writel(start_addr, ctrl->regs + S3C_VIDADDR_START0(id));
	writel(end_addr, ctrl->regs + S3C_VIDADDR_END0(id));

	if ((pdata->hw_ver == 0x62) || (pdata->hw_ver == 0x70)) {
		shw = readl(ctrl->regs + S3C_WINSHMAP);
		shw &= ~(S3C_WINSHMAP_PROTECT(id));
		writel(shw, ctrl->regs + S3C_WINSHMAP);
	}

	dev_dbg(ctrl->dev, "[fb%d] start_addr: 0x%08x, end_addr: 0x%08x\n",
		id, start_addr, end_addr);

	return 0;
}

int s3cfb_set_alpha_value(struct s3cfb_global *ctrl, int value)
{
	writel(value, ctrl->regs + S3C_BLENDCON);

	return 0;
}

int s3cfb_set_alpha_blending(struct s3cfb_global *ctrl, int id)
{
	struct s3c_platform_fb *pdata = to_fb_plat(ctrl->dev);
	struct s3cfb_window *win = ctrl->fb[id]->par;
	struct s3cfb_alpha *alpha = &win->alpha;
	u32 avalue = 0, cfg;
	u32 shw;

	if (id == 0) {
		dev_err(ctrl->dev, "[fb%d] does not support alpha blending\n",
			id);
		return -EINVAL;
	}

	if ((pdata->hw_ver == 0x62) || (pdata->hw_ver == 0x70)) {
		shw = readl(ctrl->regs + S3C_WINSHMAP);
		shw |= S3C_WINSHMAP_PROTECT(id);
		writel(shw, ctrl->regs + S3C_WINSHMAP);
	}

	cfg = readl(ctrl->regs + S3C_WINCON(id));
	cfg &= ~(S3C_WINCON_BLD_MASK | S3C_WINCON_ALPHA_SEL_MASK);

	if (alpha->mode == PIXEL_BLENDING) {
		dev_dbg(ctrl->dev, "[fb%d] alpha mode: pixel blending\n", id);

		/* fixing to DATA[31:24] for alpha value */
		cfg |= (S3C_WINCON_BLD_PIXEL | S3C_WINCON_ALPHA1_SEL);
	} else {
		dev_dbg(ctrl->dev, "[fb%d] alpha mode: plane %d blending\n",
			id, alpha->channel);

		cfg |= S3C_WINCON_BLD_PLANE;

		if (alpha->channel == 0) {
			cfg |= S3C_WINCON_ALPHA0_SEL;
			avalue = (alpha->value << S3C_VIDOSD_ALPHA0_SHIFT);
		} else {
			cfg |= S3C_WINCON_ALPHA1_SEL;
			avalue = (alpha->value << S3C_VIDOSD_ALPHA1_SHIFT);
		}
	}

	writel(cfg, ctrl->regs + S3C_WINCON(id));
	writel(avalue, ctrl->regs + S3C_VIDOSD_C(id));

	if ((pdata->hw_ver == 0x62) || (pdata->hw_ver == 0x70)) {
		shw = readl(ctrl->regs + S3C_WINSHMAP);
		shw &= ~(S3C_WINSHMAP_PROTECT(id));
		writel(shw, ctrl->regs + S3C_WINSHMAP);
	}

	return 0;
}

int s3cfb_set_window_position(struct s3cfb_global *ctrl, int id)
{
	struct fb_var_screeninfo *var = &ctrl->fb[id]->var;
	struct s3cfb_window *win = ctrl->fb[id]->par;
	u32 cfg, shw;

	shw = readl(ctrl->regs + S3C_WINSHMAP);
	shw |= S3C_WINSHMAP_PROTECT(id);
	writel(shw, ctrl->regs + S3C_WINSHMAP);

	cfg = S3C_VIDOSD_LEFT_X(win->x) | S3C_VIDOSD_TOP_Y(win->y);
	writel(cfg, ctrl->regs + S3C_VIDOSD_A(id));

	cfg = S3C_VIDOSD_RIGHT_X(win->x + var->xres - 1) |
		S3C_VIDOSD_BOTTOM_Y(win->y + var->yres - 1);

	writel(cfg, ctrl->regs + S3C_VIDOSD_B(id));

	shw = readl(ctrl->regs + S3C_WINSHMAP);
	shw &= ~(S3C_WINSHMAP_PROTECT(id));
	writel(shw, ctrl->regs + S3C_WINSHMAP);

	dev_dbg(ctrl->dev, "[fb%d] offset: (%d, %d, %d, %d)\n", id,
		win->x, win->y, win->x + var->xres - 1, win->y + var->yres - 1);

	return 0;
}

int s3cfb_set_window_size(struct s3cfb_global *ctrl, int id)
{
	struct fb_var_screeninfo *var = &ctrl->fb[id]->var;
	u32 cfg;

	if (id > 2)
		return 0;

	cfg = S3C_VIDOSD_SIZE(var->xres * var->yres);

	if (id == 0)
		writel(cfg, ctrl->regs + S3C_VIDOSD_C(id));
	else
		writel(cfg, ctrl->regs + S3C_VIDOSD_D(id));

	dev_dbg(ctrl->dev, "[fb%d] resolution: %d x %d\n", id,
		var->xres, var->yres);

	return 0;
}

int s3cfb_set_buffer_size(struct s3cfb_global *ctrl, int id)
{
	struct s3c_platform_fb *pdata = to_fb_plat(ctrl->dev);
	struct fb_var_screeninfo *var = &ctrl->fb[id]->var;
	u32 offset = (var->xres_virtual - var->xres) * var->bits_per_pixel / 8;
	u32 cfg = 0;
	u32 shw;

	cfg = S3C_VIDADDR_PAGEWIDTH(var->xres * var->bits_per_pixel / 8);
	cfg |= S3C_VIDADDR_OFFSIZE(offset);

	if ((pdata->hw_ver == 0x62) || (pdata->hw_ver == 0x70)) {
		shw = readl(ctrl->regs + S3C_WINSHMAP);
		shw |= S3C_WINSHMAP_PROTECT(id);
		writel(shw, ctrl->regs + S3C_WINSHMAP);
	}

	writel(cfg, ctrl->regs + S3C_VIDADDR_SIZE(id));

	if ((pdata->hw_ver == 0x62) || (pdata->hw_ver == 0x70)) {
		shw = readl(ctrl->regs + S3C_WINSHMAP);
		shw &= ~(S3C_WINSHMAP_PROTECT(id));
		writel(shw, ctrl->regs + S3C_WINSHMAP);
	}

	return 0;
}

int s3cfb_set_chroma_key(struct s3cfb_global *ctrl, int id)
{
	struct s3c_platform_fb *pdata = to_fb_plat(ctrl->dev);
	struct s3cfb_window *win = ctrl->fb[id]->par;
	struct s3cfb_chroma *chroma = &win->chroma;
	u32 cfg = 0;
	u32 shw;

	if (id == 0) {
		dev_err(ctrl->dev, "[fb%d] does not support chroma key\n", id);
		return -EINVAL;
	}

	cfg = (S3C_KEYCON0_KEYBLEN_DISABLE | S3C_KEYCON0_DIRCON_MATCH_FG);

	if (chroma->enabled)
		cfg |= S3C_KEYCON0_KEY_ENABLE;

	if ((pdata->hw_ver == 0x62) || (pdata->hw_ver == 0x70)) {
		shw = readl(ctrl->regs + S3C_WINSHMAP);
		shw |= S3C_WINSHMAP_PROTECT(id);
		writel(shw, ctrl->regs + S3C_WINSHMAP);
	}

	writel(cfg, ctrl->regs + S3C_KEYCON(id));

	cfg = S3C_KEYCON1_COLVAL(chroma->key);
	writel(cfg, ctrl->regs + S3C_KEYVAL(id));

	if ((pdata->hw_ver == 0x62) || (pdata->hw_ver == 0x70)) {
		shw = readl(ctrl->regs + S3C_WINSHMAP);
		shw &= ~(S3C_WINSHMAP_PROTECT(id));
		writel(shw, ctrl->regs + S3C_WINSHMAP);
	}

	dev_dbg(ctrl->dev, "[fb%d] chroma key: 0x%08x, %s\n", id, cfg,
		chroma->enabled ? "enabled" : "disabled");

	return 0;
}
