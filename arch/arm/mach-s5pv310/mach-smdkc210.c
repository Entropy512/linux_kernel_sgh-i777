/* linux/arch/arm/mach-s5pv310/mach-smdkc210.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/platform_device.h>
#include <linux/serial_core.h>
#include <linux/delay.h>
#include <linux/usb/ch9.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/mmc/host.h>
#include <linux/smsc911x.h>
#include <linux/io.h>
#include <linux/clk.h>
#if defined(CONFIG_S5P_MEM_CMA)
#include <linux/cma.h>
#endif
#include <linux/regulator/machine.h>
#include <linux/regulator/max8649.h>
#include <linux/regulator/fixed.h>
#ifdef CONFIG_ANDROID_PMEM
#include <linux/android_pmem.h>
#endif

#include <asm/pmu.h>
#include <asm/mach/arch.h>
#include <asm/mach-types.h>
#include <plat/regs-serial.h>
#include <plat/s5pv310.h>
#include <plat/cpu.h>
#include <plat/fb.h>
#include <plat/iic.h>
#include <plat/devs.h>
#include <plat/adc.h>
#include <plat/ts.h>
#include <plat/fimg2d.h>
#include <plat/sdhci.h>
#include <plat/mshci.h>
#include <plat/regs-otg.h>
#include <plat/pm.h>
#include <plat/tvout.h>

#include <plat/csis.h>
#include <plat/fimc.h>
#include <plat/gpio-cfg.h>
#include <plat/media.h>

#include <plat/s3c64xx-spi.h>
#include <plat/gpio-cfg.h>
#include <plat/pd.h>

#include <media/s5k3ba_platform.h>
#include <media/s5k4ba_platform.h>
#include <media/s5k4ea_platform.h>
#include <media/s5k6aa_platform.h>

#include <mach/regs-gpio.h>
#include <mach/regs-srom.h>

#include <mach/map.h>
#include <mach/regs-mem.h>
#include <mach/regs-clock.h>
#include <mach/media.h>
#include <mach/gpio.h>

#include <mach/spi-clocks.h>

#if defined(CONFIG_SND_SOC_WM8994) || defined(CONFIG_SND_SOC_WM8994_MODULE)
#include <linux/mfd/wm8994/pdata.h>
#endif

#if defined(CONFIG_DEV_THERMAL)
#include <plat/s5p-tmu.h>
#include <mach/regs-tmu.h>
#endif

/* Following are default values for UCON, ULCON and UFCON UART registers */
#define SMDKC210_UCON_DEFAULT	(S3C2410_UCON_TXILEVEL |	\
				 S3C2410_UCON_RXILEVEL |	\
				 S3C2410_UCON_TXIRQMODE |	\
				 S3C2410_UCON_RXIRQMODE |	\
				 S3C2410_UCON_RXFIFO_TOI |	\
				 S3C2443_UCON_RXERR_IRQEN)

#define SMDKC210_ULCON_DEFAULT	S3C2410_LCON_CS8

#define SMDKC210_UFCON_DEFAULT	(S3C2410_UFCON_FIFOMODE |	\
				 S5PV210_UFCON_TXTRIG4 |	\
				 S5PV210_UFCON_RXTRIG4)

static struct s3c2410_uartcfg smdkc210_uartcfgs[] __initdata = {
	[0] = {
		.hwport		= 0,
		.flags		= 0,
		.ucon		= SMDKC210_UCON_DEFAULT,
		.ulcon		= SMDKC210_ULCON_DEFAULT,
		.ufcon		= SMDKC210_UFCON_DEFAULT,
	},
	[1] = {
		.hwport		= 1,
		.flags		= 0,
		.ucon		= SMDKC210_UCON_DEFAULT,
		.ulcon		= SMDKC210_ULCON_DEFAULT,
		.ufcon		= SMDKC210_UFCON_DEFAULT,
	},
	[2] = {
		.hwport		= 2,
		.flags		= 0,
		.ucon		= SMDKC210_UCON_DEFAULT,
		.ulcon		= SMDKC210_ULCON_DEFAULT,
		.ufcon		= SMDKC210_UFCON_DEFAULT,
	},
	[3] = {
		.hwport		= 3,
		.flags		= 0,
		.ucon		= SMDKC210_UCON_DEFAULT,
		.ulcon		= SMDKC210_ULCON_DEFAULT,
		.ufcon		= SMDKC210_UFCON_DEFAULT,
	},
	[4] = {
		.hwport		= 4,
		.flags		= 0,
		.ucon		= SMDKC210_UCON_DEFAULT,
		.ulcon		= SMDKC210_ULCON_DEFAULT,
		.ufcon		= SMDKC210_UFCON_DEFAULT,
	},
};

#undef WRITEBACK_ENABLED

#ifdef CONFIG_VIDEO_FIMC
/*
 * External camera reset
 * Because the most of cameras take i2c bus signal, so that
 * you have to reset at the boot time for other i2c slave devices.
 * This function also called at fimc_init_camera()
 * Do optimization for cameras on your platform.
*/
#ifdef CONFIG_ITU_A
static int smdkv310_cam0_reset(int dummy)
{
	int err;
	/* Camera A */
	err = gpio_request(S5PV310_GPX1(2), "GPX1");
	if (err)
		printk(KERN_ERR "#### failed to request GPX1_2 ####\n");

	s3c_gpio_setpull(S5PV310_GPX1(2), S3C_GPIO_PULL_NONE);
	gpio_direction_output(S5PV310_GPX1(2), 0);
	gpio_direction_output(S5PV310_GPX1(2), 1);
	gpio_free(S5PV310_GPX1(2));

	return 0;
}
#endif
#ifdef CONFIG_ITU_B
static int smdkv310_cam1_reset(int dummy)
{
	int err;

	/* Camera B */
	err = gpio_request(S5PV310_GPX1(0), "GPX1");
	if (err)
		printk(KERN_ERR "#### failed to request GPX1_0 ####\n");

	s3c_gpio_setpull(S5PV310_GPX1(0), S3C_GPIO_PULL_NONE);
	gpio_direction_output(S5PV310_GPX1(0), 0);
	gpio_direction_output(S5PV310_GPX1(0), 1);
	gpio_free(S5PV310_GPX1(0));

	return 0;
}
#endif
/* for 12M camera */
#ifdef CE143_MONACO
static int smdkv310_cam0_standby(void)
{
	int err;
	/* Camera A */
	err = gpio_request(S5PV310_GPX3(3), "GPX3");
	if (err)
		printk(KERN_ERR "#### failed to request GPX3_3 ####\n");
	s3c_gpio_setpull(S5PV310_GPX3(3), S3C_GPIO_PULL_NONE);
	gpio_direction_output(S5PV310_GPX3(3), 0);
	gpio_direction_output(S5PV310_GPX3(3), 1);
	gpio_free(S5PV310_GPX3(3));

	return 0;
}

static int smdkv310_cam1_standby(void)
{
	int err;

	/* Camera B */
	err = gpio_request(S5PV310_GPX1(1), "GPX1");
	if (err)
		printk(KERN_ERR "#### failed to request GPX1_1 ####\n");
	s3c_gpio_setpull(S5PV310_GPX1(1), S3C_GPIO_PULL_NONE);
	gpio_direction_output(S5PV310_GPX1(1), 0);
	gpio_direction_output(S5PV310_GPX1(1), 1);
	gpio_free(S5PV310_GPX1(1));

	return 0;
}
#endif

/* Set for MIPI-CSI Camera module Reset */
#ifdef CONFIG_CSI_C
static int smdkv310_mipi_cam0_reset(int dummy)
{
	int err;

	err = gpio_request(S5PV310_GPX1(2), "GPX1");
	if (err)
		printk(KERN_ERR "#### failed to reset(GPX1_2) MIPI CAM\n");

	s3c_gpio_setpull(S5PV310_GPX1(2), S3C_GPIO_PULL_NONE);
	gpio_direction_output(S5PV310_GPX1(2), 0);
	gpio_direction_output(S5PV310_GPX1(2), 1);
	gpio_free(S5PV310_GPX1(2));

	return 0;
}
#endif
#ifdef CONFIG_CSI_D
static int smdkv310_mipi_cam1_reset(int dummy)
{
	int err;

	err = gpio_request(S5PV310_GPX1(0), "GPX1");
	if (err)
		printk(KERN_ERR "#### failed to reset(GPX1_0) MIPI CAM\n");

	s3c_gpio_setpull(S5PV310_GPX1(0), S3C_GPIO_PULL_NONE);
	gpio_direction_output(S5PV310_GPX1(0), 0);
	gpio_direction_output(S5PV310_GPX1(0), 1);
	gpio_free(S5PV310_GPX1(0));

	return 0;
}
#endif

#ifdef CONFIG_VIDEO_S5K3BA
static struct s5k3ba_platform_data s5k3ba_plat = {
	.default_width = 640,
	.default_height = 480,
	.pixelformat = V4L2_PIX_FMT_VYUY,
	.freq = 24000000,
	.is_mipi = 0,
};

static struct i2c_board_info  s5k3ba_i2c_info = {
	I2C_BOARD_INFO("S5K3BA", 0x2d),
	.platform_data = &s5k3ba_plat,
};

static struct s3c_platform_camera s5k3ba = {
#ifdef CONFIG_ITU_A
	.id		= CAMERA_PAR_A,
	.clk_name	= "sclk_cam0",
	.i2c_busnum	= 0,
	.cam_power	= smdkv310_cam0_reset,
#endif
#ifdef CONFIG_ITU_B
	.id		= CAMERA_PAR_B,
	.clk_name	= "sclk_cam1",
	.i2c_busnum	= 1,
	.cam_power	= smdkv310_cam1_reset,
#endif
	.type		= CAM_TYPE_ITU,
	.fmt		= ITU_601_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_CRYCBY,
	.info		= &s5k3ba_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_VYUY,
	.srclk_name	= "xusbxti",
	.clk_rate	= 24000000,
	.line_length	= 1920,
	.width		= 640,
	.height		= 480,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 640,
		.height	= 480,
	},

	/* Polarity */
	.inv_pclk	= 0,
	.inv_vsync	= 1,
	.inv_href	= 0,
	.inv_hsync	= 0,

	.initialized	= 0,
};
#endif

#ifdef CONFIG_VIDEO_S5K4BA
static struct s5k4ba_platform_data s5k4ba_plat = {
	.default_width = 800,
	.default_height = 600,
	.pixelformat = V4L2_PIX_FMT_YUYV,
	.freq = 24000000,
	.is_mipi = 0,
};

static struct i2c_board_info  s5k4ba_i2c_info = {
	I2C_BOARD_INFO("S5K4BA", 0x2d),
	.platform_data = &s5k4ba_plat,
};

static struct s3c_platform_camera s5k4ba = {
#ifdef CONFIG_ITU_A
	.id		= CAMERA_PAR_A,
	.clk_name	= "sclk_cam0",
	.i2c_busnum	= 0,
	.cam_power	= smdkv310_cam0_reset,
#endif
#ifdef CONFIG_ITU_B
	.id		= CAMERA_PAR_B,
	.clk_name	= "sclk_cam1",
	.i2c_busnum	= 1,
	.cam_power	= smdkv310_cam1_reset,
#endif
	.type		= CAM_TYPE_ITU,
	.fmt		= ITU_601_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_YCBYCR,
	.info		= &s5k4ba_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_YUYV,
	.srclk_name	= "mout_mpll",
	.clk_rate	= 60000000,
	.line_length	= 1920,
	.width		= 800,
	.height		= 600,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 800,
		.height	= 600,
	},

	/* Polarity */
	.inv_pclk	= 0,
	.inv_vsync	= 1,
	.inv_href	= 0,
	.inv_hsync	= 0,
	.reset_camera	= 1,
	.initialized	= 0,
};
#endif

/* 2 MIPI Cameras */
#ifdef CONFIG_VIDEO_S5K4EA
static struct s5k4ea_platform_data s5k4ea_plat = {
	.default_width = 1920,
	.default_height = 1080,
	.pixelformat = V4L2_PIX_FMT_UYVY,
	.freq = 24000000,
	.is_mipi = 1,
};

static struct i2c_board_info  s5k4ea_i2c_info = {
	I2C_BOARD_INFO("S5K4EA", 0x2d),
	.platform_data = &s5k4ea_plat,
};

static struct s3c_platform_camera s5k4ea = {
#ifdef CONFIG_CSI_C
	.id		= CAMERA_CSI_C,
	.clk_name	= "sclk_cam0",
	.i2c_busnum	= 0,
	.cam_power	= smdkv310_mipi_cam0_reset,
#endif
#ifdef CONFIG_CSI_D
	.id		= CAMERA_CSI_D,
	.clk_name	= "sclk_cam1",
	.i2c_busnum	= 1,
	.cam_power	= smdkv310_mipi_cam1_reset,
#endif
	.type		= CAM_TYPE_MIPI,
	.fmt		= MIPI_CSI_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_CBYCRY,
	.info		= &s5k4ea_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_UYVY,
	.srclk_name	= "mout_mpll",
	.clk_rate	= 48000000,
	.line_length	= 1920,
	.width		= 1920,
	.height		= 1080,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 1920,
		.height	= 1080,
	},

	.mipi_lanes	= 2,
	.mipi_settle	= 12,
	.mipi_align	= 32,

	/* Polarity */
	.inv_pclk	= 0,
	.inv_vsync	= 1,
	.inv_href	= 0,
	.inv_hsync	= 0,

	.initialized	= 0,
};
#endif

#ifdef CONFIG_VIDEO_S5K6AA
static struct s5k6aa_platform_data s5k6aa_plat = {
	.default_width = 640,
	.default_height = 480,
	.pixelformat = V4L2_PIX_FMT_UYVY,
	.freq = 24000000,
	.is_mipi = 1,
};

static struct i2c_board_info  s5k6aa_i2c_info = {
	I2C_BOARD_INFO("S5K6AA", 0x3c),
	.platform_data = &s5k6aa_plat,
};

static struct s3c_platform_camera s5k6aa = {
#ifdef CONFIG_CSI_C
	.id		= CAMERA_CSI_C,
	.clk_name	= "sclk_cam0",
	.i2c_busnum	= 0,
	.cam_power	= smdkv310_mipi_cam0_reset,
#endif
#ifdef CONFIG_CSI_D
	.id		= CAMERA_CSI_D,
	.clk_name	= "sclk_cam1",
	.i2c_busnum	= 1,
	.cam_power	= smdkv310_mipi_cam1_reset,
#endif
	.type		= CAM_TYPE_MIPI,
	.fmt		= MIPI_CSI_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_CBYCRY,
	.info		= &s5k6aa_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_UYVY,
	.srclk_name	= "xusbxti",
	.clk_rate	= 24000000,
	.line_length	= 1920,
	/* default resol for preview kind of thing */
	.width		= 640,
	.height		= 480,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 640,
		.height	= 480,
	},

	.mipi_lanes	= 1,
	.mipi_settle	= 6,
	.mipi_align	= 32,

	/* Polarity */
	.inv_pclk	= 0,
	.inv_vsync	= 1,
	.inv_href	= 0,
	.inv_hsync	= 0,

	.initialized	= 0,
};
#endif

#ifdef WRITEBACK_ENABLED
static struct i2c_board_info  writeback_i2c_info = {
	I2C_BOARD_INFO("WriteBack", 0x0),
};

static struct s3c_platform_camera writeback = {
	.id		= CAMERA_WB,
	.fmt		= ITU_601_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_CBYCRY,
	.i2c_busnum	= 0,
	.info		= &writeback_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_YUV444,
	.line_length	= 800,
	.width		= 480,
	.height		= 800,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 480,
		.height	= 800,
	},

	.initialized	= 0,
};
#endif

/* Interface setting */
static struct s3c_platform_fimc fimc_plat = {
#ifdef CONFIG_ITU_A
	.default_cam	= CAMERA_PAR_A,
#endif
#ifdef CONFIG_ITU_B
	.default_cam	= CAMERA_PAR_B,
#endif
#ifdef CONFIG_CSI_C
	.default_cam	= CAMERA_CSI_C,
#endif
#ifdef CONFIG_CSI_D
	.default_cam	= CAMERA_CSI_D,
#endif
#ifdef WRITEBACK_ENABLED
	.default_cam	= CAMERA_WB,
#endif
	.camera		= {
#ifdef CONFIG_VIDEO_S5K3BA
		&s5k3ba,
#endif
#ifdef CONFIG_VIDEO_S5K4BA
		&s5k4ba,
#endif
#ifdef CONFIG_VIDEO_S5K4EA
		&s5k4ea,
#endif
#ifdef CONFIG_VIDEO_S5K6AA
		&s5k6aa,
#endif
#ifdef WRITEBACK_ENABLED
		&writeback,
#endif
	},
	.hw_ver		= 0x51,
};
#endif /* CONFIG_VIDEO_FIMC */

static struct resource smdkc210_smsc911x_resources[] = {
	[0] = {
		.start = S5PV310_PA_SROM1,
		.end   = S5PV310_PA_SROM1 + SZ_64K - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_EINT(5),
		.end   = IRQ_EINT(5),
		.flags = IORESOURCE_IRQ | IRQF_TRIGGER_HIGH,
	},
};

static struct smsc911x_platform_config smsc9215 = {
	.irq_polarity = SMSC911X_IRQ_POLARITY_ACTIVE_HIGH,
	.irq_type = SMSC911X_IRQ_TYPE_PUSH_PULL,
	.flags = SMSC911X_USE_16BIT | SMSC911X_FORCE_INTERNAL_PHY,
	.phy_interface = PHY_INTERFACE_MODE_MII,
	.mac = {0x00, 0x80, 0x00, 0x23, 0x45, 0x67},
};

static struct platform_device smdkc210_smsc911x = {
	.name          = "smc911x",
	.id            = -1,
	.num_resources = ARRAY_SIZE(smdkc210_smsc911x_resources),
	.resource      = smdkc210_smsc911x_resources,
	.dev = {
		.platform_data = &smsc9215,
	},
};

static struct regulator_consumer_supply max8952_supply[] = {
	REGULATOR_SUPPLY("vdd_arm", NULL),
};

static struct regulator_consumer_supply max8649_supply[] = {
	REGULATOR_SUPPLY("vdd_int", NULL),
};

static struct regulator_consumer_supply max8649a_supply[] = {
	REGULATOR_SUPPLY("vdd_g3d", NULL),
};

static struct regulator_init_data max8952_init_data = {
	.constraints	= {
		.name		= "vdd_arm range",
		.min_uV		= 770000,
		.max_uV		= 1400000,
		.always_on	= 1,
		.boot_on	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE,
		.state_mem	= {
			.uV		= 1200000,
			.disabled	= 1,
		},
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &max8952_supply[0],
};

static struct regulator_init_data max8649_init_data = {
	.constraints	= {
		.name		= "vdd_int range",
		.min_uV		= 750000,
		.max_uV		= 1380000,
		.always_on	= 1,
		.boot_on	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE,
		.state_mem	= {
			.uV		= 1100000,
			.disabled	= 1,
		},
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &max8649_supply[0],
};

static struct regulator_init_data max8649a_init_data = {
	.constraints	= {
		.name		= "vdd_g3d range",
		.min_uV		= 750000,
		.max_uV		= 1380000,
		.always_on	= 0,
		.boot_on	= 0,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE,
		.state_mem	= {
			.uV		= 1200000,
			.disabled	= 1,
		},
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &max8649a_supply[0],
};

static struct max8649_platform_data s5pv310_max8952_info = {
	.mode		= 3,	/* VID1 = 1, VID0 = 1 */
	.extclk		= 0,
	.ramp_timing	= MAX8649_RAMP_32MV,
	.regulator	= &max8952_init_data,
};

static struct max8649_platform_data s5pv310_max8649_info = {
	.mode		= 2,	/* VID1 = 1, VID0 = 0 */
	.extclk		= 0,
	.ramp_timing	= MAX8649_RAMP_32MV,
	.regulator	= &max8649_init_data,
};

static struct max8649_platform_data s5pv310_max8649a_info = {
	.mode		= 2,	/* VID1 = 1, VID0 = 0 */
	.extclk		= 0,
	.ramp_timing	= MAX8649_RAMP_32MV,
	.regulator	= &max8649a_init_data,
};

#if defined(CONFIG_SND_SOC_WM8994) || defined(CONFIG_SND_SOC_WM8994_MODULE)

#ifdef CONFIG_REGULATOR_WM8994
static struct regulator_consumer_supply wm8994_fixed_voltage0_supplies[] = {
	{
		.dev_name	= "1-001a",
		.supply		= "AVDD2",
	}, {
		.dev_name	= "1-001a",
		.supply		= "CPVDD",
	},
};

static struct regulator_consumer_supply wm8994_fixed_voltage1_supplies[] = {
	{
		.dev_name	= "1-001a",
		.supply		= "SPKVDD1",
	}, {
		.dev_name	= "1-001a",
		.supply		= "SPKVDD2",
	},
};

static struct regulator_consumer_supply wm8994_fixed_voltage2_supplies[] = {
	{
		.dev_name	= "1-001a",
		.supply		= "DBVDD",
	},
};

static struct regulator_init_data wm8994_fixed_voltage0_init_data = {
	.constraints = {
		.always_on = 1,
	},
	.num_consumer_supplies	= ARRAY_SIZE(wm8994_fixed_voltage0_supplies),
	.consumer_supplies	= wm8994_fixed_voltage0_supplies,
};

static struct regulator_init_data wm8994_fixed_voltage1_init_data = {
	.constraints = {
		.always_on = 1,
	},
	.num_consumer_supplies	= ARRAY_SIZE(wm8994_fixed_voltage1_supplies),
	.consumer_supplies	= wm8994_fixed_voltage1_supplies,
};

static struct regulator_init_data wm8994_fixed_voltage2_init_data = {
	.constraints = {
		.always_on = 1,
	},
	.num_consumer_supplies	= ARRAY_SIZE(wm8994_fixed_voltage2_supplies),
	.consumer_supplies	= wm8994_fixed_voltage2_supplies,
};

static struct fixed_voltage_config wm8994_fixed_voltage0_config = {
	.supply_name	= "VDD_1.8V",
	.microvolts	= 1800000,
	.gpio		= -EINVAL,
	.init_data	= &wm8994_fixed_voltage0_init_data,
};

static struct fixed_voltage_config wm8994_fixed_voltage1_config = {
	.supply_name	= "DC_5V",
	.microvolts	= 5000000,
	.gpio		= -EINVAL,
	.init_data	= &wm8994_fixed_voltage1_init_data,
};

static struct fixed_voltage_config wm8994_fixed_voltage2_config = {
	.supply_name	= "VDD_3.3V",
	.microvolts	= 3300000,
	.gpio		= -EINVAL,
	.init_data	= &wm8994_fixed_voltage2_init_data,
};

static struct platform_device wm8994_fixed_voltage0 = {
	.name		= "reg-fixed-voltage",
	.id		= 0,
	.dev		= {
		.platform_data	= &wm8994_fixed_voltage0_config,
	},
};

static struct platform_device wm8994_fixed_voltage1 = {
	.name		= "reg-fixed-voltage",
	.id		= 1,
	.dev		= {
		.platform_data	= &wm8994_fixed_voltage1_config,
	},
};

static struct platform_device wm8994_fixed_voltage2 = {
	.name		= "reg-fixed-voltage",
	.id		= 2,
	.dev		= {
		.platform_data	= &wm8994_fixed_voltage2_config,
	},
};

static struct regulator_consumer_supply wm8994_avdd1_supply = {
	.dev_name	= "1-001a",
	.supply		= "AVDD1",
};

static struct regulator_consumer_supply wm8994_dcvdd_supply = {
	.dev_name	= "1-001a",
	.supply		= "DCVDD",
};

static struct regulator_init_data wm8994_ldo1_data = {
	.constraints	= {
		.name		= "AVDD1",
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &wm8994_avdd1_supply,
};

static struct regulator_init_data wm8994_ldo2_data = {
	.constraints	= {
		.name		= "DCVDD",
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &wm8994_dcvdd_supply,
};
#endif

static struct wm8994_pdata wm8994_platform_data = {
	/* configure gpio1 function: 0x0001(Logic level input/output) */
	.gpio_defaults[0] = 0x0001,
	/* configure gpio3/4/5/7 function for AIF2 voice */
	.gpio_defaults[2] = 0x8100,/*BCLK2 in*/
	.gpio_defaults[3] = 0x8100,/*LRCLK2 in*/
	.gpio_defaults[4] = 0x8100,/*DACDAT2 in*/
	/* configure gpio6 function: 0x0001(Logic level input/output) */
	.gpio_defaults[5] = 0x0001,
	.gpio_defaults[6] = 0x0100,/*ADCDAT2 out*/
#ifdef CONFIG_REGULATOR_WM8994
	.ldo[0]	= { 0, NULL, &wm8994_ldo1_data },
	.ldo[1] = { 0, NULL, &wm8994_ldo2_data },
#endif
};
#endif

#ifdef CONFIG_I2C_S3C2410
/* I2C0 */
static struct i2c_board_info i2c_devs0[] __initdata = {
	{
		I2C_BOARD_INFO("24c128", 0x50),		/* Samsung S524AD0XD1 */
	}, {
		I2C_BOARD_INFO("24c128", 0x52),		/* Samsung S524AD0XD1 */
	}, {
		I2C_BOARD_INFO("max8952", 0x60),
		.platform_data	= &s5pv310_max8952_info,
	}, {
		I2C_BOARD_INFO("max8649", 0x62),
		.platform_data	= &s5pv310_max8649a_info,
	},
};
#ifdef CONFIG_S3C_DEV_I2C1
/* I2C1 */
static struct i2c_board_info i2c_devs1[] __initdata = {
#if defined(CONFIG_SND_SOC_WM8994) || defined(CONFIG_SND_SOC_WM8994_MODULE)
	{
		I2C_BOARD_INFO("wm8994", 0x1a),
		.platform_data	= &wm8994_platform_data,
	},
#endif

#ifdef CONFIG_VIDEO_TVOUT
	{
		I2C_BOARD_INFO("s5p_ddc", (0x74 >> 1)),
	},
#endif
	{
		I2C_BOARD_INFO("max8649", 0x60),
		.platform_data	= &s5pv310_max8649_info,
	},
};
#endif
#ifdef CONFIG_S3C_DEV_I2C2
/* I2C2 */
static struct i2c_board_info i2c_devs2[] __initdata = {
};
#endif
#ifdef CONFIG_S3C_DEV_I2C3
/* I2C3 */
static struct i2c_board_info i2c_devs3[] __initdata = {
};
#endif
#ifdef CONFIG_S3C_DEV_I2C4
/* I2C4 */
static struct i2c_board_info i2c_devs4[] __initdata = {
};
#endif
#ifdef CONFIG_S3C_DEV_I2C5
/* I2C5 */
static struct i2c_board_info i2c_devs5[] __initdata = {
};
#endif
#ifdef CONFIG_S3C_DEV_I2C6
/* I2C6 */
static struct i2c_board_info i2c_devs6[] __initdata = {
};
#endif
#ifdef CONFIG_S3C_DEV_I2C7
/* I2C7 */
static struct i2c_board_info i2c_devs7[] __initdata = {
};
#endif
#endif

#ifdef CONFIG_S3C_DEV_HSMMC
static struct s3c_sdhci_platdata smdkc210_hsmmc0_pdata __initdata = {
	.cd_type		= S3C_MSHCI_CD_INTERNAL,
#if defined(CONFIG_S5PV310_SD_CH0_8BIT)
	.max_width		= 8,
	.host_caps		= MMC_CAP_8_BIT_DATA,
#endif
};
#endif
#ifdef CONFIG_S3C_DEV_HSMMC1
static struct s3c_sdhci_platdata smdkc210_hsmmc1_pdata __initdata = {
	.cd_type		= S3C_MSHCI_CD_INTERNAL,
};
#endif
#ifdef CONFIG_S3C_DEV_HSMMC2
static struct s3c_sdhci_platdata smdkc210_hsmmc2_pdata __initdata = {
	.cd_type		= S3C_MSHCI_CD_INTERNAL,
#if defined(CONFIG_S5PV310_SD_CH2_8BIT)
	.max_width		= 8,
	.host_caps		= MMC_CAP_8_BIT_DATA,
#endif
};
#endif
#ifdef CONFIG_S3C_DEV_HSMMC3
static struct s3c_sdhci_platdata smdkc210_hsmmc3_pdata __initdata = {
	.cd_type		= S3C_MSHCI_CD_INTERNAL,
};
#endif
#ifdef CONFIG_S5P_DEV_MSHC
static struct s3c_mshci_platdata smdkv310_mshc_pdata __initdata = {
	.cd_type		= S3C_MSHCI_CD_INTERNAL,
#if defined(CONFIG_S5PV310_MSHC_CH0_8BIT) && \
defined(CONFIG_S5PV310_MSHC_CH0_DDR)
	.max_width		= 8,
	.host_caps		= MMC_CAP_8_BIT_DATA | MMC_CAP_DDR,
#elif defined(CONFIG_S5PV310_MSHC_CH0_8BIT)
	.max_width		= 8,
	.host_caps		= MMC_CAP_8_BIT_DATA,
#elif defined(CONFIG_S5PV310_MSHC_CH0_DDR)
	.host_caps				= MMC_CAP_DDR,
#endif
};
#endif

#ifdef CONFIG_VIDEO_FIMG2D
static struct fimg2d_platdata fimg2d_data __initdata = {
	.hw_ver = 30,
	.parent_clkname = "mout_mpll",
	.clkname = "sclk_fimg2d",
	.gate_clkname = "fimg2d",
	.clkrate = 250 * 1000000,
};
#endif

#if defined(CONFIG_S3C64XX_DEV_SPI)
static struct s3c64xx_spi_csinfo spi0_csi[] = {
	[0] = {
		.line = S5PV310_GPB(1),
		.set_level = gpio_set_value,
	},
};

static struct spi_board_info spi0_board_info[] __initdata = {
	{
		.modalias = "spidev",
		.platform_data = NULL,
		.max_speed_hz = 10*1000*1000,
		.bus_num = 0,
		.chip_select = 0,
		.mode = SPI_MODE_0,
		.controller_data = &spi0_csi[0],
	}
};
#endif

#ifdef CONFIG_FB_S3C_AMS369FG06

static struct s3c_platform_fb ams369fg06_data __initdata = {
	.hw_ver = 0x70,
	.clk_name = "sclk_lcd",
	.nr_wins = 5,
	.default_win = CONFIG_FB_S3C_DEFAULT_WINDOW,
	.swap = FB_SWAP_HWORD | FB_SWAP_WORD,
};

#define	LCD_BUS_NUM	1
static struct s3c64xx_spi_csinfo spi1_csi[] = {
	[0] = {
		.line = S5PV310_GPB(5),
		.set_level = gpio_set_value,
	},
};

static struct spi_board_info spi_board_info[] __initdata = {
	{
		.modalias	= "ams369fg06",
		.platform_data	= NULL,
		.max_speed_hz	= 1200000,
		.bus_num	= LCD_BUS_NUM,
		.chip_select	= 0,
		.mode		= SPI_MODE_3,
		.controller_data = &spi1_csi[0],
	}
};

#elif defined(CONFIG_S3C64XX_DEV_SPI)
static struct s3c64xx_spi_csinfo spi1_csi[] = {
	[0] = {
		.line = S5PV310_GPB(5),
		.set_level = gpio_set_value,
	},
};

static struct spi_board_info spi1_board_info[] __initdata = {
	{
		.modalias = "spidev",
		.platform_data = NULL,
		.max_speed_hz = 1200000,
		.bus_num = 1,
		.chip_select = 0,
		.mode = SPI_MODE_3,
		.controller_data = &spi1_csi[0],
	}
};
#endif

#if defined(CONFIG_S3C64XX_DEV_SPI)
static struct s3c64xx_spi_csinfo spi2_csi[] = {
	[0] = {
		.line = S5PV310_GPC1(2),
		.set_level = gpio_set_value,
	},
};

static struct spi_board_info spi2_board_info[] __initdata = {
	{
		.modalias = "spidev",
		.platform_data = NULL,
		.max_speed_hz = 10*1000*1000,
		.bus_num = 2,
		.chip_select = 0,
		.mode = SPI_MODE_0,
		.controller_data = &spi2_csi[0],
	}
};
#endif

#ifdef CONFIG_ANDROID_PMEM
static struct android_pmem_platform_data pmem_pdata = {
	.name = "pmem",
	.no_allocator = 1,
	.cached = 0,
	.start = 0, // will be set during proving pmem driver.
	.size = 0 // will be set during proving pmem driver.
};

static struct android_pmem_platform_data pmem_gpu1_pdata = {
	.name = "pmem_gpu1",
	.no_allocator = 1,
	.cached = 0,
	/* .buffered = 1, */
	.start = 0,
	.size = 0,
};

static struct platform_device pmem_device = {
	.name = "android_pmem",
	.id = 0,
	.dev = { .platform_data = &pmem_pdata },
};

static struct platform_device pmem_gpu1_device = {
	.name = "android_pmem",
	.id = 1,
	.dev = { .platform_data = &pmem_gpu1_pdata },
};

static void __init android_pmem_set_platdata(void)
{
#if defined(CONFIG_S5P_MEM_CMA)
	pmem_pdata.size = CONFIG_ANDROID_PMEM_MEMSIZE_PMEM * SZ_1K;
	pmem_gpu1_pdata.size = CONFIG_ANDROID_PMEM_MEMSIZE_PMEM_GPU1 * SZ_1K;
#elif defined(CONFIG_S5P_MEM_BOOTMEM)
	pmem_pdata.start = (u32)s5p_get_media_memory_bank(S5P_MDEV_PMEM, 0);
	pmem_pdata.size = (u32)s5p_get_media_memsize_bank(S5P_MDEV_PMEM, 0);

	pmem_gpu1_pdata.start = (u32)s5p_get_media_memory_bank(S5P_MDEV_PMEM_GPU1, 0);
	pmem_gpu1_pdata.size = (u32)s5p_get_media_memsize_bank(S5P_MDEV_PMEM_GPU1, 0);
#endif
}
#endif

#ifdef CONFIG_BATTERY_S3C
struct platform_device sec_device_battery = {
	.name	= "sec-fake-battery",
	.id	= -1,
};
#endif

static struct resource pmu_resource[] = {
	[0] = {
		.start = IRQ_PMU_0,
		.end   = IRQ_PMU_0,
		.flags = IORESOURCE_IRQ,
	},
	[1] = {
		.start = IRQ_PMU_1,
		.end   = IRQ_PMU_1,
		.flags = IORESOURCE_IRQ,
	}
};

static struct platform_device pmu_device = {
	.name		= "arm-pmu",
	.id		= ARM_PMU_DEVICE_CPU,
	.resource	= pmu_resource,
	.num_resources	= 2,
};

static struct platform_device *smdkc210_devices[] __initdata = {
#ifdef CONFIG_S5PV310_DEV_PD
	&s5pv310_device_pd[PD_MFC],
	&s5pv310_device_pd[PD_G3D],
	&s5pv310_device_pd[PD_LCD0],
	&s5pv310_device_pd[PD_LCD1],
	&s5pv310_device_pd[PD_CAM],
	&s5pv310_device_pd[PD_GPS],
	&s5pv310_device_pd[PD_TV],
	/* &s5pv310_device_pd[PD_MAUDIO], */
#endif

	&smdkc210_smsc911x,
#ifdef CONFIG_BATTERY_S3C
	&sec_device_battery,
#endif
#ifdef CONFIG_KEYPAD_S3C
	&s3c_device_keypad,
#endif
#ifdef CONFIG_FB_S3C
	&s3c_device_fb,
#endif
#ifdef CONFIG_I2C_S3C2410
	&s3c_device_i2c0,
#if defined(CONFIG_S3C_DEV_I2C1)
	&s3c_device_i2c1,
#endif
#if defined(CONFIG_S3C_DEV_I2C2)
	&s3c_device_i2c2,
#endif
#if defined(CONFIG_S3C_DEV_I2C3)
	&s3c_device_i2c3,
#endif
#if defined(CONFIG_S3C_DEV_I2C4)
	&s3c_device_i2c4,
#endif
#if defined(CONFIG_S3C_DEV_I2C5)
	&s3c_device_i2c5,
#endif
#if defined(CONFIG_S3C_DEV_I2C6)
	&s3c_device_i2c6,
#endif
#if defined(CONFIG_S3C_DEV_I2C7)
	&s3c_device_i2c7,
#endif
#endif
#ifdef CONFIG_SND_S3C64XX_SOC_I2S_V4
	&s5pv310_device_iis0,
#endif
#ifdef CONFIG_SND_S3C_SOC_PCM
	&s5pv310_device_pcm1,
#endif
#ifdef CONFIG_SND_SOC_SMDK_WM9713
	&s5pv310_device_ac97,
#endif
#ifdef CONFIG_SND_SAMSUNG_SOC_SPDIF
	&s5pv310_device_spdif,
#endif
#ifdef CONFIG_SND_S5P_RP
	&s5pv310_device_rp,
#endif

#ifdef CONFIG_MTD_NAND
	&s3c_device_nand,
#endif
#ifdef CONFIG_MTD_ONENAND
	&s5p_device_onenand,
#endif

#ifdef CONFIG_S3C_DEV_HSMMC
	&s3c_device_hsmmc0,
#endif
#ifdef CONFIG_S3C_DEV_HSMMC1
	&s3c_device_hsmmc1,
#endif
#ifdef CONFIG_S3C_DEV_HSMMC2
	&s3c_device_hsmmc2,
#endif
#ifdef CONFIG_S3C_DEV_HSMMC3
	&s3c_device_hsmmc3,
#endif
#ifdef CONFIG_S5P_DEV_MSHC
	&s3c_device_mshci,
#endif
#ifdef CONFIG_TOUCHSCREEN_S3C2410
#ifdef CONFIG_S3C_DEV_ADC
	&s3c_device_ts,
#endif
#ifdef CONFIG_S3C_DEV_ADC1
	&s3c_device_ts1,
#endif
#endif

#ifdef CONFIG_TOUCHSCREEN_S5PV310
	&s3c_device_ts,
#endif

#ifdef CONFIG_S3C_ADC
	&s3c_device_adc,
#endif

#ifdef CONFIG_VIDEO_TVOUT
	&s5p_device_tvout,
	&s5p_device_cec,
	&s5p_device_hpd,
#endif

#ifdef CONFIG_S3C2410_WATCHDOG
	&s3c_device_wdt,
#endif

#ifdef CONFIG_USB
	&s3c_device_usb_ehci,
	&s3c_device_usb_ohci,
#endif

#ifdef CONFIG_ANDROID_PMEM
	&pmem_device,
	&pmem_gpu1_device,
#endif

#ifdef CONFIG_VIDEO_FIMC
	&s3c_device_fimc0,
	&s3c_device_fimc1,
	&s3c_device_fimc2,
	&s3c_device_fimc3,
#ifdef CONFIG_VIDEO_FIMC_MIPI
	&s3c_device_csis0,
	&s3c_device_csis1,
#endif
#endif
#ifdef CONFIG_VIDEO_JPEG
	&s5p_device_jpeg,
#endif
#if defined(CONFIG_VIDEO_MFC50) || defined(CONFIG_VIDEO_MFC5X)
	&s5p_device_mfc,
#endif

#ifdef CONFIG_VIDEO_FIMG2D
	&s5p_device_fimg2d,
#endif

#ifdef CONFIG_USB_GADGET
	&s3c_device_usbgadget,
#endif
#ifdef CONFIG_USB_ANDROID_RNDIS
	&s3c_device_rndis,
#endif
#ifdef CONFIG_USB_ANDROID
	&s3c_device_android_usb,
	&s3c_device_usb_mass_storage,
#endif


#if defined(CONFIG_S3C64XX_DEV_SPI)
	&s5pv310_device_spi0,
#endif
#ifdef CONFIG_FB_S3C_AMS369FG06
	&s5pv310_device_spi1,
#elif defined(CONFIG_S3C64XX_DEV_SPI)
	&s5pv310_device_spi1,
#endif
#if defined(CONFIG_S3C64XX_DEV_SPI)
	&s5pv310_device_spi2,
#endif

#ifdef CONFIG_S5P_SYSMMU
	&s5p_device_sysmmu,
#endif

#ifdef CONFIG_S3C_DEV_GIB
	&s3c_device_gib,
#endif

#ifdef CONFIG_S3C_DEV_RTC
	&s3c_device_rtc,
#endif

	&s5p_device_ace,
#if (defined(CONFIG_SND_SOC_WM8994) || \
		defined(CONFIG_SND_SOC_WM8994_MODULE)) && \
		defined(CONFIG_REGULATOR_WM8994)
	&wm8994_fixed_voltage0,
	&wm8994_fixed_voltage1,
	&wm8994_fixed_voltage2,
#endif

#ifdef CONFIG_SATA_AHCI_PLATFORM
	&s5pv310_device_sata,
#endif
#ifdef CONFIG_DEV_THERMAL
	&s5p_device_tmu,
#endif

	&pmu_device,
};

#if defined(CONFIG_VIDEO_TVOUT)
static struct s5p_platform_hpd hdmi_hpd_data __initdata = {

};
static struct s5p_platform_cec hdmi_cec_data __initdata = {

};
#endif

static void __init sromc_setup(void)
{
	u32 tmp;

	tmp = __raw_readl(S5P_SROM_BW);
	tmp &= ~(0xffff);
	tmp |= (0x9999);
	__raw_writel(tmp, S5P_SROM_BW);

	__raw_writel(0x11061C91, S5P_SROM_BC1);

	tmp = __raw_readl(S5P_VA_GPIO + 0x120);
	tmp &= ~(0xffffff);
	tmp |= (0x221121);
	__raw_writel(tmp, (S5P_VA_GPIO + 0x120));

	__raw_writel(0x22222222, (S5P_VA_GPIO + 0x180));
	__raw_writel(0x22222222, (S5P_VA_GPIO + 0x1a0));
	__raw_writel(0x22222222, (S5P_VA_GPIO + 0x1c0));
	__raw_writel(0x22222222, (S5P_VA_GPIO + 0x1e0));
}

static void __init smdkc210_smsc911x_init(void)
{
	u32 cs1;

	/* configure nCS1 width to 16 bits */
	cs1 = __raw_readl(S5PV310_SROM_BW) &
		    ~(S5PV310_SROM_BW__CS_MASK <<
				    S5PV310_SROM_BW__NCS1__SHIFT);
	cs1 |= ((1 << S5PV310_SROM_BW__DATAWIDTH__SHIFT) |
		(1 << S5PV310_SROM_BW__WAITENABLE__SHIFT) |
		(1 << S5PV310_SROM_BW__BYTEENABLE__SHIFT)) <<
		S5PV310_SROM_BW__NCS1__SHIFT;
	__raw_writel(cs1, S5PV310_SROM_BW);

	/* set timing for nCS1 suitable for ethernet chip */
	__raw_writel((0x1 << S5PV310_SROM_BCX__PMC__SHIFT) |
		     (0x9 << S5PV310_SROM_BCX__TACP__SHIFT) |
		     (0xc << S5PV310_SROM_BCX__TCAH__SHIFT) |
		     (0x1 << S5PV310_SROM_BCX__TCOH__SHIFT) |
		     (0x6 << S5PV310_SROM_BCX__TACC__SHIFT) |
		     (0x1 << S5PV310_SROM_BCX__TCOS__SHIFT) |
		     (0x1 << S5PV310_SROM_BCX__TACS__SHIFT), S5PV310_SROM_BC1);
}


#if defined(CONFIG_S5P_MEM_CMA)
static void __init s5pv310_reserve(void);
#endif
static void __init smdkc210_map_io(void)
{
	s5p_init_io(NULL, 0, S5P_VA_CHIPID);
	s3c24xx_init_clocks(24000000);
	s3c24xx_init_uarts(smdkc210_uartcfgs, ARRAY_SIZE(smdkc210_uartcfgs));

#ifdef CONFIG_MTD_NAND
	s3c_device_nand.name = "s5pv310-nand";
#endif

#if defined(CONFIG_S5P_MEM_CMA)
	s5pv310_reserve();
#elif defined(CONFIG_S5P_MEM_BOOTMEM)
	s5p_reserve_bootmem();
#endif
}

#ifdef CONFIG_TOUCHSCREEN_S3C2410
static struct s3c2410_ts_mach_info s3c_ts_platform __initdata = {
	.delay			= 10000,
	.presc			= 49,
	.oversampling_shift	= 2,
};
#endif

#ifdef  CONFIG_TOUCHSCREEN_S5PV310
static struct s3c_ts_mach_info s3c_ts_platform __initdata = {
	.delay                  = 10000,
	.presc                  = 49,
	.oversampling_shift     = 2,
	.resol_bit              = 12,
	.s3c_adc_con            = ADC_TYPE_2,
};
#endif

#ifdef  CONFIG_TOUCHSCREEN_S5PV310
static struct s3c_adc_mach_info s3c_adc_platform __initdata = {
	/*    s5pc100 supports 12-bit resolution */
	.delay          = 10000,
	.presc          = 49,
	.resolution     = 12,
};
#endif

static void __init smdkc210_machine_init(void)
{
#if defined(CONFIG_S3C64XX_DEV_SPI)
	struct device *spi0_dev = &s5pv310_device_spi0.dev;
#endif
#ifdef CONFIG_FB_S3C_AMS369FG06
	struct clk *sclk = NULL;
	struct clk *prnt = NULL;
	struct device *spi_dev = &s5pv310_device_spi1.dev;
#elif defined(CONFIG_S3C64XX_DEV_SPI)
	struct clk *sclk = NULL;
	struct clk *prnt = NULL;
	struct device *spi1_dev = &s5pv310_device_spi1.dev;
#endif
#if defined(CONFIG_S3C64XX_DEV_SPI)
	struct device *spi2_dev = &s5pv310_device_spi2.dev;
#endif

#ifdef CONFIG_ANDROID_PMEM
	android_pmem_set_platdata();
#endif
	s3c_pm_init();

#if defined(CONFIG_S5PV310_DEV_PD) && !defined(CONFIG_PM_RUNTIME)
	/*
	 * These power domains should be always on
	 * without runtime pm support.
	 */
	s5pv310_pd_enable(&s5pv310_device_pd[PD_MFC].dev);
	s5pv310_pd_enable(&s5pv310_device_pd[PD_G3D].dev);
	s5pv310_pd_enable(&s5pv310_device_pd[PD_LCD0].dev);
	s5pv310_pd_enable(&s5pv310_device_pd[PD_LCD1].dev);
	s5pv310_pd_enable(&s5pv310_device_pd[PD_CAM].dev);
	s5pv310_pd_enable(&s5pv310_device_pd[PD_TV].dev);
	s5pv310_pd_enable(&s5pv310_device_pd[PD_GPS].dev);
#endif

	smdkc210_smsc911x_init();
#ifdef CONFIG_I2C_S3C2410
	s3c_i2c0_set_platdata(NULL);
	i2c_register_board_info(0, i2c_devs0, ARRAY_SIZE(i2c_devs0));
#ifdef CONFIG_S3C_DEV_I2C1
	s3c_i2c1_set_platdata(NULL);
	i2c_register_board_info(1, i2c_devs1, ARRAY_SIZE(i2c_devs1));
#endif
#ifdef CONFIG_S3C_DEV_I2C2
	s3c_i2c2_set_platdata(NULL);
	i2c_register_board_info(2, i2c_devs2, ARRAY_SIZE(i2c_devs2));
#endif
#ifdef CONFIG_S3C_DEV_I2C3
	s3c_i2c3_set_platdata(NULL);
	i2c_register_board_info(3, i2c_devs3, ARRAY_SIZE(i2c_devs3));
#endif
#ifdef CONFIG_S3C_DEV_I2C4
	s3c_i2c4_set_platdata(NULL);
	i2c_register_board_info(4, i2c_devs4, ARRAY_SIZE(i2c_devs4));
#endif
#ifdef CONFIG_S3C_DEV_I2C5
	s3c_i2c5_set_platdata(NULL);
	i2c_register_board_info(5, i2c_devs5, ARRAY_SIZE(i2c_devs5));
#endif
#ifdef CONFIG_S3C_DEV_I2C6
	s3c_i2c6_set_platdata(NULL);
	i2c_register_board_info(6, i2c_devs6, ARRAY_SIZE(i2c_devs6));
#endif
#ifdef CONFIG_S3C_DEV_I2C7
	s3c_i2c7_set_platdata(NULL);
	i2c_register_board_info(7, i2c_devs7, ARRAY_SIZE(i2c_devs7));
#endif
#endif

#ifdef CONFIG_FB_S3C
#ifdef CONFIG_FB_S3C_AMS369FG06
#else
	s3cfb_set_platdata(NULL);
#endif
#endif

#if 1
//open the rtc external clock for GPS
	static void __iomem *s3c_rtc_base;
	s3c_rtc_base = ioremap(0x10070000, 0xff + 1);
	printk("s3c_rtc_base: %x\n", s3c_rtc_base);

	if (s3c_rtc_base == NULL) {
		printk("Error: no memory mapping for rtc\n");
	}
	__raw_writel(0x200, s3c_rtc_base + 0x40);
	iounmap(s3c_rtc_base);
#endif

#ifdef CONFIG_VIDEO_FIMC
	/* fimc */
	s3c_fimc0_set_platdata(&fimc_plat);
	s3c_fimc1_set_platdata(&fimc_plat);
	s3c_fimc2_set_platdata(&fimc_plat);
	s3c_fimc3_set_platdata(&fimc_plat);
#ifdef CONFIG_ITU_A
	smdkv310_cam0_reset(1);
#endif
#ifdef CONFIG_ITU_B
	smdkv310_cam1_reset(1);
#endif
#ifdef CONFIG_VIDEO_FIMC_MIPI
	s3c_csis0_set_platdata(NULL);
	s3c_csis1_set_platdata(NULL);
	smdkv310_cam1_reset(1);
	smdkv310_cam1_reset(1);
#endif
#endif

#ifdef CONFIG_VIDEO_MFC5X
#ifdef CONFIG_S5PV310_DEV_PD
	/*s5p_device_mfc.dev.parent = &s5pv310_device_pd[PD_MFC].dev;*/
#endif
#endif

#ifdef CONFIG_VIDEO_FIMG2D
	s5p_fimg2d_set_platdata(&fimg2d_data);
#ifdef CONFIG_S5PV310_DEV_PD
	s5p_device_fimg2d.dev.parent = &s5pv310_device_pd[PD_LCD0].dev;
#endif
#endif
#ifdef CONFIG_S3C_DEV_HSMMC
	s3c_sdhci0_set_platdata(&smdkc210_hsmmc0_pdata);
#endif
#ifdef CONFIG_S3C_DEV_HSMMC1
	s3c_sdhci1_set_platdata(&smdkc210_hsmmc1_pdata);
#endif
#ifdef CONFIG_S3C_DEV_HSMMC2
	s3c_sdhci2_set_platdata(&smdkc210_hsmmc2_pdata);
#endif
#ifdef CONFIG_S3C_DEV_HSMMC3
	s3c_sdhci3_set_platdata(&smdkc210_hsmmc3_pdata);
#endif
#ifdef CONFIG_S5P_DEV_MSHC
	s3c_mshci_set_platdata(&smdkc210_mshc_pdata);
#endif

#ifdef CONFIG_TOUCHSCREEN_S3C2410
#ifdef CONFIG_S3C_DEV_ADC
	s3c24xx_ts_set_platdata(&s3c_ts_platform);
#endif
#ifdef CONFIG_S3C_DEV_ADC1
	s3c24xx_ts1_set_platdata(&s3c_ts_platform);
#endif
#endif
#ifdef CONFIG_DEV_THERMAL
	s5p_tmu_set_platdata(NULL);
#endif

#ifdef  CONFIG_TOUCHSCREEN_S5PV310
	s3c_adc_set_platdata(&s3c_adc_platform);
	s3c_ts_set_platdata(&s3c_ts_platform);
#endif

#if defined(CONFIG_VIDEO_TVOUT)
	s5p_hdmi_hpd_set_platdata(&hdmi_hpd_data);
	s5p_hdmi_cec_set_platdata(&hdmi_cec_data);

#ifdef CONFIG_S5PV310_DEV_PD
	s5p_device_tvout.dev.parent = &s5pv310_device_pd[PD_TV].dev;
#endif
#endif

#ifdef CONFIG_S5PV310_DEV_PD
#ifdef CONFIG_FB_S3C
	s3c_device_fb.dev.parent = &s5pv310_device_pd[PD_LCD0].dev;
#endif
#endif

#ifdef CONFIG_S5PV310_DEV_PD
#ifdef CONFIG_VIDEO_FIMC
	s3c_device_fimc0.dev.parent = &s5pv310_device_pd[PD_CAM].dev;
	s3c_device_fimc1.dev.parent = &s5pv310_device_pd[PD_CAM].dev;
	s3c_device_fimc2.dev.parent = &s5pv310_device_pd[PD_CAM].dev;
	s3c_device_fimc3.dev.parent = &s5pv310_device_pd[PD_CAM].dev;
#endif
#endif

#ifdef CONFIG_S5PV310_DEV_PD
#ifdef CONFIG_SND_S3C64XX_SOC_I2S_V4
	/* s5pv310_device_iis0.dev.parent = &s5pv310_device_pd[PD_MAUDIO].dev; */
#endif
#ifdef CONFIG_SND_S3C_SOC_PCM
	/* s5pv310_device_pcm1.dev.parent = &s5pv310_device_pd[PD_MAUDIO].dev; */
#endif
#ifdef CONFIG_SND_SOC_SMDK_WM9713
	/* s5pv310_device_ac97.dev.parent = &s5pv310_device_pd[PD_MAUDIO].dev; */
#endif
#ifdef CONFIG_SND_SAMSUNG_SOC_SPDIF
	/* s5pv310_device_spdif.dev.parent = &s5pv310_device_pd[PD_MAUDIO].dev; */
#endif
#endif

	platform_add_devices(smdkc210_devices, ARRAY_SIZE(smdkc210_devices));

#if defined(CONFIG_S3C64XX_DEV_SPI)
	sclk = clk_get(spi0_dev, "sclk_spi");
	if (IS_ERR(sclk))
		dev_err(spi0_dev, "failed to get sclk for SPI-0\n");
	prnt = clk_get(spi0_dev, "mout_mpll");
	if (IS_ERR(prnt))
		dev_err(spi0_dev, "failed to get prnt\n");
	clk_set_parent(sclk, prnt);
	clk_put(prnt);

	if (!gpio_request(S5PV310_GPB(1), "SPI_CS0")) {
		gpio_direction_output(S5PV310_GPB(1), 1);
		s3c_gpio_cfgpin(S5PV310_GPB(1), S3C_GPIO_SFN(1));
		s3c_gpio_setpull(S5PV310_GPB(1), S3C_GPIO_PULL_UP);
		s5pv310_spi_set_info(0, S5PV310_SPI_SRCCLK_SCLK,
			ARRAY_SIZE(spi0_csi));
	}
	spi_register_board_info(spi0_board_info, ARRAY_SIZE(spi0_board_info));
#endif

#ifdef CONFIG_FB_S3C_AMS369FG06
	sclk = clk_get(spi_dev, "sclk_spi");
	if (IS_ERR(sclk))
		dev_err(spi_dev, "failed to get sclk for SPI-1\n");
	prnt = clk_get(spi_dev, "xusbxti");
	if (IS_ERR(prnt))
		dev_err(spi_dev, "failed to get prnt\n");
	clk_set_parent(sclk, prnt);
	clk_put(prnt);

	if (!gpio_request(S5PV310_GPB(5), "LCD_CS")) {
		gpio_direction_output(S5PV310_GPB(5), 1);
		s3c_gpio_cfgpin(S5PV310_GPB(5), S3C_GPIO_SFN(1));
		s3c_gpio_setpull(S5PV310_GPB(5), S3C_GPIO_PULL_UP);
		s5pv310_spi_set_info(LCD_BUS_NUM, S5PV310_SPI_SRCCLK_SCLK,
			ARRAY_SIZE(spi1_csi));
	}
	spi_register_board_info(spi_board_info, ARRAY_SIZE(spi_board_info));
	s3cfb_set_platdata(&ams369fg06_data);
#elif defined(CONFIG_S3C64XX_DEV_SPI)
	sclk = clk_get(spi1_dev, "sclk_spi");
	if (IS_ERR(sclk))
		dev_err(spi1_dev, "failed to get sclk for SPI-1\n");
	prnt = clk_get(spi1_dev, "mout_mpll");
	if (IS_ERR(prnt))
		dev_err(spi1_dev, "failed to get prnt\n");
	clk_set_parent(sclk, prnt);
	clk_put(prnt);

	if (!gpio_request(S5PV310_GPB(5), "SPI_CS1")) {
		gpio_direction_output(S5PV310_GPB(5), 1);
		s3c_gpio_cfgpin(S5PV310_GPB(5), S3C_GPIO_SFN(1));
		s3c_gpio_setpull(S5PV310_GPB(5), S3C_GPIO_PULL_UP);
		s5pv310_spi_set_info(1, S5PV310_SPI_SRCCLK_SCLK,
			ARRAY_SIZE(spi1_csi));
	}
	spi_register_board_info(spi1_board_info, ARRAY_SIZE(spi1_board_info));
#endif

#if defined(CONFIG_S3C64XX_DEV_SPI)
	sclk = clk_get(spi2_dev, "sclk_spi");
	if (IS_ERR(sclk))
		dev_err(spi2_dev, "failed to get sclk for SPI-2\n");
	prnt = clk_get(spi2_dev, "mout_mpll");
	if (IS_ERR(prnt))
		dev_err(spi2_dev, "failed to get prnt\n");
	clk_set_parent(sclk, prnt);
	clk_put(prnt);

	if (!gpio_request(S5PV310_GPC1(2), "SPI_CS2")) {
		gpio_direction_output(S5PV310_GPC1(2), 1);
		s3c_gpio_cfgpin(S5PV310_GPC1(2), S3C_GPIO_SFN(1));
		s3c_gpio_setpull(S5PV310_GPC1(2), S3C_GPIO_PULL_UP);
		s5pv310_spi_set_info(2, S5PV310_SPI_SRCCLK_SCLK,
			ARRAY_SIZE(spi2_csi));
	}
	spi_register_board_info(spi2_board_info, ARRAY_SIZE(spi2_board_info));
#endif
}

#if defined(CONFIG_KEYPAD_S3C) || defined(CONFIG_KEYPAD_S3C_MODULE)
void s3c_setup_keypad_cfg_gpio(int rows, int columns)
{
	unsigned int gpio;
	unsigned int end;

	/* Set all the necessary GPX3 pins to special-function 0 */
	if (rows > 8) {
		end = S5PV310_GPX3(rows - 9);
		for (gpio = S5PV310_GPX3(0); gpio < end; gpio++) {
			s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
			s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP);
		}
	}

	/* Set all the necessary GPX2 pins to special-function 0 */
	end = S5PV310_GPX2(8);
	for (gpio = S5PV310_GPX2(0); gpio < end; gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP);
	}

	/* Set all the necessary GPX1 pins to special-function 0 */
	end = S5PV310_GPX1(columns);
	for (gpio = S5PV310_GPX1(0); gpio < end; gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	}
}
EXPORT_SYMBOL(s3c_setup_keypad_cfg_gpio);
#endif

#if defined(CONFIG_S5P_MEM_CMA)
static void __init s5pv310_reserve(void)
{
	static struct cma_region regions[] = {
#ifdef CONFIG_ANDROID_PMEM_MEMSIZE_PMEM
		{
			.name = "pmem",
			.size = CONFIG_ANDROID_PMEM_MEMSIZE_PMEM * SZ_1K,
			.start = 0,
		},
#endif
#ifdef CONFIG_ANDROID_PMEM_MEMSIZE_PMEM_GPU1
		{
			.name = "pmem_gpu1",
			.size = CONFIG_ANDROID_PMEM_MEMSIZE_PMEM_GPU1 * SZ_1K,
			.start = 0,
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMD
	{
		.name = "fimd",
		.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMD * SZ_1K,
		.start = 0
	},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC
		{
			.name = "mfc",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC * SZ_1K,
			.start = 0
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC0
		{
			.name = "mfc0",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC0 * SZ_1K,
			{
				.alignment = 1 << 17,
			},
			.start = 0,
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC1
		{
			.name = "mfc1",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC1 * SZ_1K,
			{
				.alignment = 1 << 17,
			},
			.start = 0,
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC0
		{
			.name = "fimc0",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC0 * SZ_1K,
			.start = 0
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC1
		{
			.name = "fimc1",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC1 * SZ_1K,
			.start = 0
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC2
	{
		.name = "fimc2",
		.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC2 * SZ_1K,
		.start = 0
	},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC3
		{
			.name = "fimc3",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC3 * SZ_1K,
			.start = 0
		},
#endif
#ifdef CONFIG_AUDIO_SAMSUNG_MEMSIZE_SRP
		{
			.name = "srp",
			.size = CONFIG_AUDIO_SAMSUNG_MEMSIZE_SRP * SZ_1K,
			.start = 0,
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_JPEG
		{
			.name = "jpeg",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_JPEG * SZ_1K,
			.start = 0
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMG2D
               {
                       .name = "fimg2d",
                       .size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMG2D * SZ_1K,
                       .start = 0
               },
#endif
		{}
	};

	static const char map[] __initconst =
		"android_pmem.0=pmem;android_pmem.1=pmem_gpu1;s3cfb.0=fimd;"
		"s3c-fimc.0=fimc0;s3c-fimc.1=fimc1;s3c-fimc.2=fimc2;"
		"s3c-fimc.3=fimc3;s3c-mfc=mfc,mfc0,mfc1;s5p-rp=srp;"
		"s5p-jpeg=jpeg;"
		"s5p-fimg2d=fimg2d";
	int i = 0;
	unsigned int bank0_end = meminfo.bank[0].start +
					meminfo.bank[0].size;
	unsigned int bank1_end = meminfo.bank[1].start +
					meminfo.bank[1].size;

	printk(KERN_ERR "mem infor: bank0 start-> 0x%x, bank0 size-> 0x%x\
			\nbank1 start-> 0x%x, bank1 size-> 0x%x\n",
			(int)meminfo.bank[0].start, (int)meminfo.bank[0].size,
			(int)meminfo.bank[1].start, (int)meminfo.bank[1].size);
	for (; i < ARRAY_SIZE(regions) ; i++) {
		if (regions[i].start == 0) {
			regions[i].start = bank0_end - regions[i].size;
			bank0_end = regions[i].start;
		} else if (regions[i].start == 1) {
			regions[i].start = bank1_end - regions[i].size;
			bank1_end = regions[i].start;
		}
		printk(KERN_ERR "CMA reserve : %s, addr is 0x%x, size is 0x%x\n",
			regions[i].name, regions[i].start, regions[i].size);
	}

	cma_set_defaults(regions, map);
	cma_early_regions_reserve(NULL);
}
#endif

MACHINE_START(SMDKC210, "SMDKC210")
	/* Maintainer: Kukjin Kim <kgene.kim@samsung.com> */
	.phys_io	= S3C_PA_UART & 0xfff00000,
	.io_pg_offst	= (((u32)S3C_VA_UART) >> 18) & 0xfffc,
	.boot_params	= S5P_PA_SDRAM + 0x100,
	.init_irq	= s5pv310_init_irq,
	.map_io		= smdkc210_map_io,
	.init_machine	= smdkc210_machine_init,
	.timer		= &s5pv310_timer,
MACHINE_END
