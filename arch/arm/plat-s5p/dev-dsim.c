/* linux/arch/arm/plat-s5pc11x/dev-dsim.c
 *
 * Copyright 2009 Samsung Electronics
 *	InKi Dae <inki.dae@samsung.com>
 *
 * S5PC1XX series device definition for MIPI-DSIM
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>

#include <mach/map.h>
#include <asm/irq.h>

#include <plat/devs.h>
#include <plat/cpu.h>

#include <mach/dsim.h>
#include <mach/mipi_ddi.h>
//#include <mach/max8998_function.h>
//#include <linux/regulator/max8998.h>
#include <linux/mfd/max8997.h>


static struct dsim_config dsim_info = {
	/* only evt1 */
	.auto_flush = false,		/* main frame fifo auto flush at VSYNC pulse */

	.eot_disable = false,		/* only DSIM_1_02 or DSIM_1_03(for c110) */

	.auto_vertical_cnt = true,
	.hse = false,
	.hfp = true,
	.hbp = false,
	.hsa = false,

	.e_no_data_lane = DSIM_DATA_LANE_4,
	.e_byte_clk = DSIM_PLL_OUT_DIV8,

	/* 420MHz */
#ifdef CONFIG_FB_S3C_S6D6AA0
	.p = 3,
	.m = 62,
	.s = 0,
#endif
#ifdef CONFIG_FB_S3C_S6E8AA0
	.p = 3,
	.m = 120,
	.s = 1,
#endif


	.pll_stable_time = 500,		/* D-PHY PLL stable time spec :min = 200usec ~ max 400usec */

	.esc_clk = 20 * 1000000,	/* escape clk : 10MHz */

	.stop_holding_cnt = 0x7ff,	/* stop state holding counter after bta change count 0 ~ 0xfff */
	.bta_timeout = 0xff,		/* bta timeout 0 ~ 0xff */
	.rx_timeout = 0xffff,		/* lp rx timeout 0 ~ 0xffff */

	.e_lane_swap = DSIM_NO_CHANGE,
};

/* define ddi platform data based on MIPI-DSI. */
static struct mipi_ddi_platform_data mipi_ddi_pd = {
	.backlight_on = NULL,
};

static struct dsim_lcd_config dsim_lcd_info = {
	.e_interface		= DSIM_VIDEO,

	.parameter[DSI_VIRTUAL_CH_ID]	= (unsigned int) DSIM_VIRTUAL_CH_0,
	.parameter[DSI_FORMAT]		= (unsigned int) DSIM_24BPP_888,
	.parameter[DSI_VIDEO_MODE_SEL]	= (unsigned int) DSIM_BURST_SYNC_EVENT,
	.mipi_ddi_pd		= (void *) &mipi_ddi_pd,
};

static struct resource s5p_dsim_resource[] = {
	[0] = {
		.start = S5P_PA_DSIM0,
		.end   = S5P_PA_DSIM0 + S5P_SZ_DSIM0 - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_MIPIDSI0,
		.end   = IRQ_MIPIDSI0,
		.flags = IORESOURCE_IRQ,
	},
};

extern int get_usb_cable_state(void);

static void s5p_dsim_mipi_power(int enable)
{
#if 0
	if (enable) {
  		max8997_ldo_enable_direct(MAX8997_LDO3);		
 		max8997_ldo_enable_direct(MAX8997_LDO6);		 
 	} else {
	       if( get_usb_cable_state()==0)
		       	max8997_ldo_disable_direct(MAX8997_LDO3);
		   
		max8997_ldo_disable_direct(MAX8997_LDO6);
	}
#endif

	return;
}

static struct s5p_platform_dsim dsim_platform_data = {
	.clk_name = "dsim0",
	.dsim_info = &dsim_info,
	.dsim_lcd_info = &dsim_lcd_info,

	.mipi_power = s5p_dsim_mipi_power,
	.enable_clk = s5p_dsim_enable_clk,
	.part_reset = s5p_dsim_part_reset,
	.init_d_phy = s5p_dsim_init_d_phy,

	/* default platform revision is 0(evt0). */
	.platform_rev = 0,
};

struct platform_device s5p_device_dsim = {
	.name			= "s5p-dsim",
	.id			= 0,
	.num_resources		= ARRAY_SIZE(s5p_dsim_resource),
	.resource		= s5p_dsim_resource,
	.dev			= {
		.platform_data = (void *) &dsim_platform_data,
	},
};
