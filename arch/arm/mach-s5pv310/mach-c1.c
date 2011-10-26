/* linux/arch/arm/mach-s5pv310/mach-c1.c
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
#include <linux/i2c-gpio.h>
#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/gpio_keys.h>
#include <linux/spi/spi.h>
#include <linux/mmc/host.h>
#include <linux/smsc911x.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/spi/spi_gpio.h>
#include <linux/i2c/mxt224_u1.h>
#include <linux/mfd/max8998.h>
#include <linux/mfd/max8997.h>
#include <linux/regulator/max8649.h>
#include <linux/max17040_battery.h>
#include <linux/max17042-fuelgauge.h>
#include <linux/max8922-charger.h>
#include <linux/power_supply.h>
#include <linux/fsa9480.h>
#include <linux/lcd.h>
#include <linux/ld9040.h>
#if defined(CONFIG_S5P_MEM_CMA)
#include <linux/cma.h>
#endif
#include <linux/regulator/machine.h>
#include <linux/regulator/fixed.h>
#ifdef CONFIG_ANDROID_PMEM
#include <linux/android_pmem.h>
#endif

#include <asm/pmu.h>
#include <asm/mach/arch.h>
#include <asm/mach-types.h>
#include <asm/cacheflush.h>
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
#include <plat/pd.h>
#include <plat/media.h>
#include <plat/udc-hs.h>
#include <plat/s5p-otghost.h>

#include <plat/s3c64xx-spi.h>

#include <media/s5k3ba_platform.h>
#include <media/s5k4ba_platform.h>
#include <media/s5k4ea_platform.h>
#include <media/s5k6aa_platform.h>
#ifdef CONFIG_VIDEO_M5MO
#include <media/m5mo_platform.h>
#endif
#ifdef CONFIG_VIDEO_S5K6AAFX
#include <media/s5k6aafx_platform.h>
#endif
#ifdef CONFIG_VIDEO_S5K5BAFX
#include <media/s5k5bafx_platform.h>
#endif
#ifdef CONFIG_VIDEO_S5K5BBGX
#include <media/s5k5bbgx_platform.h>
#endif
#if defined(CONFIG_DEV_THERMAL)
#include <plat/s5p-tmu.h>
#include <mach/regs-tmu.h>
#endif

#include <mach/regs-gpio.h>

#include <mach/map.h>
#include <mach/regs-mem.h>
#include <mach/regs-clock.h>
#include <mach/media.h>
#include <mach/gpio.h>
#include <mach/system.h>

#ifdef CONFIG_FB_S3C_MIPI_LCD
#include <mach/mipi_ddi.h>
#include <mach/dsim.h>
#endif

#include <mach/spi-clocks.h>
#include <mach/pm-core.h>

#include <mach/sec_battery.h>
#include <mach/sec_debug.h>

#ifdef CONFIG_PN544
#include <linux/pn544.h>
#endif /* CONFIG_PN544	*/

#ifdef CONFIG_EPEN_WACOM_G5SP
#include <linux/wacom_i2c.h>
static struct wacom_g5_callbacks *wacom_callbacks;
#endif /* CONFIG_EPEN_WACOM_G5SP */

#include <linux/sec_jack.h>
#include <linux/input/k3g.h>
#include <linux/k3dh.h>
#include <linux/i2c/ak8975.h>
#include <linux/cm3663.h>
#include <linux/mfd/mc1n2_pdata.h>

#include "../../../drivers/usb/gadget/s3c_udc.h"
#include <../../../drivers/video/samsung/s3cfb.h>

#include <linux/wimax/samsung/wimax732.h>

#include <linux/usb/android_composite.h>

#ifdef CONFIG_USBHUB_USB3803
#include <linux/usb3803.h>
#endif

#include "c1.h"

extern struct sys_timer s5pv310_timer;

struct class *sec_class;
EXPORT_SYMBOL(sec_class);

struct device *switch_dev;
EXPORT_SYMBOL(switch_dev);

struct device *gps_dev;
EXPORT_SYMBOL(gps_dev);

#ifdef CONFIG_TARGET_LOCALE_KOR
#define SYSTEM_REV_SND 0x07
#else
#define SYSTEM_REV_SND 0x09
#endif

static ssize_t c1_switch_show_vbus(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int i;
	struct regulator *regulator;

	regulator = regulator_get(NULL, "safeout1");
	if (IS_ERR(regulator)) {
		pr_warn("%s: fail to get regulator\n", __func__);
		return sprintf(buf, "UNKNOWN\n");
	}
	if (regulator_is_enabled(regulator))
		i = sprintf(buf, "VBUS is enabled\n");
	else
		i = sprintf(buf, "VBUS is disabled\n");

	regulator_put(regulator);

	return i;
}

static ssize_t c1_switch_store_vbus(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	int disable, ret, usb_mode;
	struct regulator *regulator;
	struct s3c_udc *udc = platform_get_drvdata(&s3c_device_usbgadget);

	if (!strncmp(buf, "0", 1))
		disable = 0;
	else if (!strncmp(buf, "1", 1))
		disable = 1;
	else {
		pr_warn("%s: Wrong command\n", __func__);
		return count;
	}

	pr_info("%s: disable=%d\n", __func__, disable);
	usb_mode = disable ? USB_CABLE_DETACHED_WITHOUT_NOTI : USB_CABLE_ATTACHED;
	ret = udc->change_usb_mode(usb_mode);
	if (ret < 0)
		pr_err("%s: fail to change mode!!!\n", __func__);

	regulator = regulator_get(NULL, "safeout1");
	if (IS_ERR(regulator)) {
		pr_warn("%s: fail to get regulator\n", __func__);
		return count;
	}

	if (disable) {
		if (regulator_is_enabled(regulator))
			regulator_force_disable(regulator);
		if (!regulator_is_enabled(regulator))
			regulator_enable(regulator);
	} else {
		if (!regulator_is_enabled(regulator))
			regulator_enable(regulator);
	}
	regulator_put(regulator);

	return count;
}

static DEVICE_ATTR(disable_vbus, 0664, c1_switch_show_vbus,
		   c1_switch_store_vbus);

#ifdef CONFIG_TARGET_LOCALE_KOR
/* usb access control for SEC DM */
struct device *usb_lock;
static int usb_access_lock;

static ssize_t c1_switch_show_usb_lock(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	if (usb_access_lock)
		return snprintf(buf, PAGE_SIZE, "USB_LOCK");
	else
		return snprintf(buf, PAGE_SIZE, "USB_UNLOCK");
}

static ssize_t c1_switch_store_usb_lock(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int lock, ret;
	struct s3c_udc *udc = platform_get_drvdata(&s3c_device_usbgadget);

	if (!strncmp(buf, "0", 1))
		lock = 0;
	else if (!strncmp(buf, "1", 1))
		lock = 1;
	else {
		pr_warn("%s: Wrong command\n", __func__);
		return count;
	}

	if (IS_ERR_OR_NULL(udc) ||
		IS_ERR_OR_NULL(udc->change_usb_mode))
		return count;

	pr_info("%s: lock=%d\n", __func__, lock);

	if (lock != usb_access_lock) {
		usb_access_lock = lock;

		if (lock) {
			ret = udc->change_usb_mode(USB_CABLE_DETACHED);
			if (ret < 0)
				pr_err("%s: fail to detach usb\n", __func__);
		}
	}

	return count;
}

static DEVICE_ATTR(enable, 0664,
		   c1_switch_show_usb_lock, c1_switch_store_usb_lock);
#endif

static void c1_sec_switch_init(void)
{
	int ret;
	sec_class = class_create(THIS_MODULE, "sec");

	if (IS_ERR(sec_class))
		pr_err("Failed to create class(sec)!\n");

	switch_dev = device_create(sec_class, NULL, 0, NULL, "switch");

	if (IS_ERR(switch_dev))
		pr_err("Failed to create device(switch)!\n");

	ret = device_create_file(switch_dev, &dev_attr_disable_vbus);
	if (ret)
		pr_err("Failed to create device file(disable_vbus)!\n");

#ifdef CONFIG_TARGET_LOCALE_KOR
	usb_lock = device_create(sec_class, switch_dev,
				MKDEV(0, 0), NULL, ".usb_lock");

	if (IS_ERR(usb_lock)) {
		pr_err("Failed to create device (usb_lock)!\n");
		return;
	}

	if (device_create_file(usb_lock, &dev_attr_enable) < 0)
		pr_err("Failed to create device file(.usblock/enable)!\n");
#endif
};

static void uart_switch_init(void)
{
	int ret, val;
	struct device *uartswitch_dev;

	uartswitch_dev = device_create(sec_class, NULL, 0, NULL, "uart_switch");
	if (IS_ERR(uartswitch_dev)) {
		pr_err("Failed to create device(uart_switch)!\n");
		return;
	}

	ret = gpio_request(GPIO_UART_SEL, "UART_SEL");
	if (ret < 0) {
		pr_err("Failed to request GPIO_UART_SEL!\n");
		return;
	}
	s3c_gpio_setpull(GPIO_UART_SEL, S3C_GPIO_PULL_NONE);
	val = gpio_get_value(GPIO_UART_SEL);
	gpio_direction_output(GPIO_UART_SEL, val);

	gpio_export(GPIO_UART_SEL, 1);

	gpio_export_link(uartswitch_dev, "UART_SEL", GPIO_UART_SEL);
}

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
		.cfg_gpio	= s3c_setup_uart_cfg_gpio,
	},
	[1] = {
		.hwport		= 1,
		.flags		= 0,
		.ucon		= SMDKC210_UCON_DEFAULT,
		.ulcon		= SMDKC210_ULCON_DEFAULT,
		.ufcon		= SMDKC210_UFCON_DEFAULT,
		.cfg_gpio	= s3c_setup_uart_cfg_gpio,
	},
	[2] = {
		.hwport		= 2,
		.flags		= 0,
		.ucon		= SMDKC210_UCON_DEFAULT,
		.ulcon		= SMDKC210_ULCON_DEFAULT,
		.ufcon		= SMDKC210_UFCON_DEFAULT,
		.cfg_gpio	= s3c_setup_uart_cfg_gpio,
	},
	[3] = {
		.hwport		= 3,
		.flags		= 0,
		.ucon		= SMDKC210_UCON_DEFAULT,
		.ulcon		= SMDKC210_ULCON_DEFAULT,
		.ufcon		= SMDKC210_UFCON_DEFAULT,
		.cfg_gpio	= s3c_setup_uart_cfg_gpio,
	},
	[4] = {
		.hwport		= 4,
		.flags		= 0,
		.ucon		= SMDKC210_UCON_DEFAULT,
		.ulcon		= SMDKC210_ULCON_DEFAULT,
		.ufcon		= SMDKC210_UFCON_DEFAULT,
		.cfg_gpio	= s3c_setup_uart_cfg_gpio,
	},
};

#if defined(CONFIG_S3C64XX_DEV_SPI)
static struct s3c64xx_spi_csinfo spi0_csi[] = {
	[0] = {
		.line = S5PV310_GPB(1),
		.set_level = gpio_set_value,
	},
};

static struct spi_board_info spi0_board_info[] __initdata = {
#if defined(CONFIG_TDMB)
	{
		.modalias = "tdmbspi",
		.platform_data = NULL,
		.max_speed_hz = 5000000,
		.bus_num = 0,
		.chip_select = 0,
		.mode = SPI_MODE_0,
		.controller_data = &spi0_csi[0],
	},
#elif defined(CONFIG_ISDBT_FC8100)
	{
		.modalias	=	"isdbtspi",
		.platform_data	=	NULL,
		.max_speed_hz	=	400000,
		.bus_num	=	0,
		.chip_select	=	0,
		.mode		=	(SPI_MODE_0|SPI_CS_HIGH),
		.controller_data	=	&spi0_csi[0],
	},
#else
	{
		.modalias = "spidev",
		.platform_data = NULL,
		.max_speed_hz = 10*1000*1000,
		.bus_num = 0,
		.chip_select = 0,
		.mode = SPI_MODE_0,
		.controller_data = &spi0_csi[0],
	}
#endif
};
#endif

#ifdef CONFIG_VIDEO_FIMC

#define WRITEBACK_ENABLED
/*
 * External camera reset
 * Because the most of cameras take i2c bus signal, so that
 * you have to reset at the boot time for other i2c slave devices.
 * This function also called at fimc_init_camera()
 * Do optimization for cameras on your platform.
*/

#ifdef CONFIG_VIDEO_M5MO
#define CAM_CHECK_ERR_RET(x, msg)	\
	if (unlikely((x) < 0)) { \
		printk(KERN_ERR "\nfail to %s: err = %d\n", msg, x); \
		return x; \
	}
#define CAM_CHECK_ERR(x, msg)	\
		if (unlikely((x) < 0)) { \
			printk(KERN_ERR "\nfail to %s: err = %d\n", msg, x); \
		}

static int m5mo_get_i2c_busnum(void)
{
#ifdef CONFIG_VIDEO_M5MO_USE_SWI2C
	return 25;
#else
	return 0;
#endif
}

static int m5mo_power_on(void)
{
	struct regulator *regulator;
	int ret = 0;

	printk(KERN_DEBUG "%s: in\n", __func__);

	ret = gpio_request(GPIO_CAM_VGA_nSTBY, "GPL2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_VGA_nSTBY)\n");
		return ret;
	}
	ret = gpio_request(GPIO_CAM_VGA_nRST, "GPL2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_VGA_nRST)\n");
		return ret;
	}
	ret = gpio_request(GPIO_CAM_SENSOR_CORE, "GPE2");
	if (ret) {
		printk(KERN_ERR "fail to request gpio(CAM_SENSOR_CORE)\n");
		return ret;
	}
	ret = gpio_request(GPIO_CAM_IO_EN, "GPE2");
	if (ret) {
		printk(KERN_ERR "fail to request gpio(CAM_IO_EN)\n");
		return ret;
	}
	ret = gpio_request(GPIO_VT_CAM_15V, "GPE2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_VT_CAM_15V)\n");
		return ret;
	}
	ret = gpio_request(GPIO_ISP_RESET, "GPY3");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(ISP_RESET)\n");
		return ret;
	}
#if defined(CONFIG_MACH_C1_REV02) || defined(CONFIG_MACH_C1Q1_REV02) \
 || defined(CONFIG_MACH_P6_REV02) || defined(CONFIG_MACH_TALBOT_REV02) || defined(CONFIG_TARGET_LOCALE_NA)
	ret = gpio_request(GPIO_8M_AF_EN, "GPK1");
	if (ret) {
		printk(KERN_ERR "fail to request gpio(8M_AF_EN)\n");
		return ret;
	}
#endif

	/* CAM_VT_nSTBY low */
	ret = gpio_direction_output(GPIO_CAM_VGA_nSTBY, 0);
	CAM_CHECK_ERR_RET(ret, "output VGA_nSTBY");

	/* CAM_VT_nRST	low */
	gpio_direction_output(GPIO_CAM_VGA_nRST, 0);
	CAM_CHECK_ERR_RET(ret, "output VGA_nRST");
	udelay(10);

	/* CAM_ISP_CORE_1.2V */
	regulator = regulator_get(NULL, "cam_isp_core");
	if (IS_ERR(regulator))
		return -ENODEV;
	ret = regulator_enable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR_RET(ret, "enable cam_isp_core");
	/* No delay */

	/* CAM_SENSOR_CORE_1.2V */
	ret = gpio_direction_output(GPIO_CAM_SENSOR_CORE, 1);
	CAM_CHECK_ERR_RET(ret, "output senser_core");
	udelay(10);

	/* CAM_SENSOR_A2.8V */
	ret = gpio_direction_output(GPIO_CAM_IO_EN, 1);
	CAM_CHECK_ERR_RET(ret, "output IO_EN");
	/* it takes about 100us at least during level transition.*/
	udelay(160); /* 130us -> 160us */

	/* VT_CORE_1.5V */
	ret = gpio_direction_output(GPIO_VT_CAM_15V, 1);
#ifdef CONFIG_TARGET_LOCALE_NA
	s3c_gpio_setpull(GPIO_VT_CAM_15V, S3C_GPIO_PULL_NONE);
#endif /* CONFIG_TARGET_LOCALE_NA */
	CAM_CHECK_ERR_RET(ret, "output VT_CAM_1.5V");
	udelay(20);

	/* CAM_AF_2.8V */
#if defined(CONFIG_MACH_C1_REV02) || defined(CONFIG_MACH_C1Q1_REV02) \
 || defined(CONFIG_MACH_P6_REV02) || defined(CONFIG_MACH_TALBOT_REV02) \
 || defined(CONFIG_TARGET_LOCALE_NA)
	/* 8M_AF_2.8V_EN */
	ret = gpio_direction_output(GPIO_8M_AF_EN, 1);
	CAM_CHECK_ERR(ret, "output AF");
#else
	regulator = regulator_get(NULL, "cam_af");
	if (IS_ERR(regulator))
		return -ENODEV;
	ret = regulator_enable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR(ret, "output cam_af");
#endif
	mdelay(7);

	/* VT_CAM_1.8V */
	regulator = regulator_get(NULL, "vt_cam_1.8v");
	if (IS_ERR(regulator))
		return -ENODEV;
	ret = regulator_enable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR_RET(ret, "enable vt_1.8v");
	udelay(10);

	/* CAM_ISP_1.8V */
	regulator = regulator_get(NULL, "cam_isp");
	if (IS_ERR(regulator))
		return -ENODEV;
	ret = regulator_enable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR_RET(ret, "enable cam_isp");
	udelay(120); /* at least */

	/* CAM_SENSOR_IO_1.8V */
	regulator = regulator_get(NULL, "cam_sensor_io");
	if (IS_ERR(regulator))
		return -ENODEV;
	ret = regulator_enable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR_RET(ret, "enable sensor_io");
	udelay(30);

	/* MCLK */
	ret = s3c_gpio_cfgpin(GPIO_CAM_MCLK, S3C_GPIO_SFN(2));
	CAM_CHECK_ERR_RET(ret, "cfg mclk");
	s3c_gpio_setpull(GPIO_CAM_MCLK, S3C_GPIO_PULL_NONE);
	udelay(70);

	/* ISP_RESET */
	ret = gpio_direction_output(GPIO_ISP_RESET, 1);
	CAM_CHECK_ERR_RET(ret, "output reset");
	mdelay(4);

	gpio_free(GPIO_CAM_VGA_nSTBY);
	gpio_free(GPIO_CAM_VGA_nRST);
	gpio_free(GPIO_CAM_SENSOR_CORE);
	gpio_free(GPIO_CAM_IO_EN);
	gpio_free(GPIO_VT_CAM_15V);
	gpio_free(GPIO_ISP_RESET);
#if defined(CONFIG_MACH_C1_REV02) || defined(CONFIG_MACH_C1Q1_REV02) \
 || defined(CONFIG_MACH_P6_REV02) || defined(CONFIG_MACH_TALBOT_REV02) || defined(CONFIG_TARGET_LOCALE_NA)
	gpio_free(GPIO_8M_AF_EN);
#endif

	return ret;
}

static int m5mo_power_down(void)
{
	struct regulator *regulator;
	int ret = 0;

	printk(KERN_DEBUG "%s: in\n", __func__);

#if defined(CONFIG_MACH_C1_REV02) || defined(CONFIG_MACH_C1Q1_REV02) \
 || defined(CONFIG_MACH_P6_REV02) || defined(CONFIG_MACH_TALBOT_REV02) || defined(CONFIG_TARGET_LOCALE_NA)
	ret = gpio_request(GPIO_8M_AF_EN, "GPK1");
	if (ret) {
		printk(KERN_ERR "fail to request gpio(8M_AF_EN)\n");
		return ret;
	}
#endif
	ret = gpio_request(GPIO_ISP_RESET, "GPY3");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(ISP_RESET)\n");
		return ret;
	}
	ret = gpio_request(GPIO_CAM_IO_EN, "GPE2");
	if (ret) {
		printk(KERN_ERR "fail to request gpio(GPIO_CAM_IO_EN)\n");
		return ret;
	}
	ret = gpio_request(GPIO_CAM_SENSOR_CORE, "GPE2");
	if (ret) {
		printk(KERN_ERR "fail to request gpio(CAM_SENSOR_COR)\n");
		return ret;
	}
	ret = gpio_request(GPIO_VT_CAM_15V, "GPE2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_VT_CAM_15V)\n");
		return ret;
	}

	/* s3c_i2c0_force_stop(); */

	mdelay(3);

	/* ISP_RESET */
	ret = gpio_direction_output(GPIO_ISP_RESET, 0);
	CAM_CHECK_ERR(ret, "output reset");
	#ifdef CONFIG_TARGET_LOCALE_KOR
	mdelay(3); /* fix without seeing signal form for kor.*/
	#else
	mdelay(2);
	#endif

	/* MCLK */
	ret = s3c_gpio_cfgpin(GPIO_CAM_MCLK, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_CAM_MCLK, S3C_GPIO_PULL_DOWN);
	CAM_CHECK_ERR(ret, "cfg mclk");
	udelay(20);

	/* CAM_AF_2.8V */
#if defined(CONFIG_MACH_C1_REV02) || defined(CONFIG_MACH_C1Q1_REV02) \
 || defined(CONFIG_MACH_P6_REV02) || defined(CONFIG_MACH_TALBOT_REV02) \
 || defined(CONFIG_TARGET_LOCALE_NA)
	/* 8M_AF_2.8V_EN */
	ret = gpio_direction_output(GPIO_8M_AF_EN, 0);
	CAM_CHECK_ERR(ret, "output AF");
#else
	regulator = regulator_get(NULL, "cam_af");
	if (IS_ERR(regulator))
		return -ENODEV;
	if (regulator_is_enabled(regulator))
		ret = regulator_force_disable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR(ret, "disable cam_af");
#endif

	/* CAM_SENSOR_IO_1.8V */
	regulator = regulator_get(NULL, "cam_sensor_io");
	if (IS_ERR(regulator))
		return -ENODEV;
	if (regulator_is_enabled(regulator))
		ret = regulator_force_disable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR(ret, "disable, sensor_io");
	udelay(10);

	/* CAM_ISP_1.8V */
	regulator = regulator_get(NULL, "cam_isp");
	if (IS_ERR(regulator))
		return -ENODEV;
	if (regulator_is_enabled(regulator))
		ret = regulator_force_disable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR(ret, "disable cam_isp");
	udelay(500); /* 100us -> 500us */

	/* VT_CAM_1.8V */
	regulator = regulator_get(NULL, "vt_cam_1.8v");
	if (IS_ERR(regulator))
		return -ENODEV;
	if (regulator_is_enabled(regulator))
		ret = regulator_disable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR(ret, "disable vt_1.8v");
	udelay(250); /* 10us -> 250us */

	/* VT_CORE_1.5V */
	ret = gpio_direction_output(GPIO_VT_CAM_15V, 0);
	CAM_CHECK_ERR(ret, "output VT_CAM_1.5V");
	udelay(300); /*10 -> 300 us */

	/* CAM_SENSOR_A2.8V */
	ret = gpio_direction_output(GPIO_CAM_IO_EN, 0);
	CAM_CHECK_ERR(ret, "output IO_EN");
	udelay(800);

	/* CAM_SENSOR_CORE_1.2V */
	ret = gpio_direction_output(GPIO_CAM_SENSOR_CORE, 0);
	CAM_CHECK_ERR(ret, "output SENSOR_CORE");
	udelay(5);

	/* CAM_ISP_CORE_1.2V */
	regulator = regulator_get(NULL, "cam_isp_core");
	if (IS_ERR(regulator))
		return -ENODEV;
	if (regulator_is_enabled(regulator))
		ret = regulator_force_disable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR(ret, "disable isp_core");

#if defined(CONFIG_MACH_C1_REV02) || defined(CONFIG_MACH_C1Q1_REV02) \
 || defined(CONFIG_MACH_P6_REV02) || defined(CONFIG_MACH_TALBOT_REV02) || defined(CONFIG_TARGET_LOCALE_NA)
	gpio_free(GPIO_8M_AF_EN);
#endif
	gpio_free(GPIO_ISP_RESET);
	gpio_free(GPIO_CAM_IO_EN);
	gpio_free(GPIO_CAM_SENSOR_CORE);
	gpio_free(GPIO_VT_CAM_15V);

	return ret;
}

int s3c_csis_power(int enable)
{
	struct regulator *regulator;
	int ret = 0;

	/* mipi_1.1v ,mipi_1.8v are always powered-on.
	* If they are off, we then power them on.
	*/
	if (enable) {
		/* VMIPI_1.1V */
		regulator = regulator_get(NULL, "vmipi_1.1v");
		if (IS_ERR(regulator))
			goto error_out;
		if (!regulator_is_enabled(regulator)) {
			printk(KERN_WARNING "%s: vmipi_1.1v is off. so ON\n",
				__func__);
			ret = regulator_enable(regulator);
			CAM_CHECK_ERR(ret, "enable vmipi_1.1v");
		}
		regulator_put(regulator);

		/* VMIPI_1.8V */
		regulator = regulator_get(NULL, "vmipi_1.8v");
		if (IS_ERR(regulator))
			goto error_out;
		if (!regulator_is_enabled(regulator)) {
			printk(KERN_WARNING "%s: vmipi_1.8v is off. so ON\n",
				__func__);
			ret = regulator_enable(regulator);
			CAM_CHECK_ERR(ret, "enable vmipi_1.8v");
		}
		regulator_put(regulator);
	}

	return 0;

error_out:
	printk(KERN_ERR "%s: ERROR: failed to check mipi-power\n", __func__);
	return 0;
}

static int m5mo_flash_power(int enable)
{
	struct regulator *flash = regulator_get(NULL, "led_flash");
	struct regulator *movie = regulator_get(NULL, "led_movie");

	if (enable) {
		regulator_set_current_limit(flash, 490000, 530000);
		regulator_enable(flash);
		regulator_set_current_limit(movie, 90000, 110000);
		regulator_enable(movie);
	} else {
		if (regulator_is_enabled(flash))
			regulator_disable(flash);
		if (regulator_is_enabled(movie))
			regulator_disable(movie);
	}
	regulator_put(flash);
	regulator_put(movie);

	return 0;
}

static int m5mo_power(int enable)
{
	int ret = 0;

	printk("%s %s\n", __func__, enable ? "on" : "down");
	if (enable) {
		ret = m5mo_power_on();
		if (unlikely(ret))
			goto error_out;
	} else
		ret = m5mo_power_down();

	ret = s3c_csis_power(enable);
	m5mo_flash_power(enable);

error_out:
	return ret;
}

static int m5mo_config_isp_irq(void)
{
	s3c_gpio_cfgpin(GPIO_ISP_INT, S3C_GPIO_SFN(0xF));
	s3c_gpio_setpull(GPIO_ISP_INT, S3C_GPIO_PULL_NONE);
	return 0;
}

static struct m5mo_platform_data m5mo_plat = {
	.default_width = 640, /* 1920 */
	.default_height = 480, /* 1080 */
	.pixelformat = V4L2_PIX_FMT_UYVY,
	.freq = 24000000,
	.is_mipi = 1,
	.config_isp_irq = m5mo_config_isp_irq,
};

static struct i2c_board_info  m5mo_i2c_info = {
	I2C_BOARD_INFO("M5MO", 0x1F),
	.platform_data = &m5mo_plat,
	.irq = IRQ_EINT(13),
};

static struct s3c_platform_camera m5mo = {
	.id		= CAMERA_CSI_C,
	.clk_name	= "sclk_cam0",
	.get_i2c_busnum	= m5mo_get_i2c_busnum,
	.cam_power	= m5mo_power, /*smdkv310_mipi_cam0_reset,*/
	.type		= CAM_TYPE_MIPI,
	.fmt		= ITU_601_YCBCR422_8BIT, /*MIPI_CSI_YCBCR422_8BIT*/
	.order422	= CAM_ORDER422_8BIT_CBYCRY,
	.info		= &m5mo_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_UYVY,
	.srclk_name	= "xusbxti", /* "mout_mpll" */
	.clk_rate	= 24000000, /* 48000000 */
	.line_length	= 1920,
	.width		= 640,
	.height		= 480,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 640,
		.height	= 480,
	},

	.mipi_lanes	= 2,
	.mipi_settle	= 12,
	.mipi_align	= 32,

	/* Polarity */
	.inv_pclk	= 0,
	.inv_vsync	= 1,
	.inv_href	= 0,
	.inv_hsync	= 0,
	.reset_camera	= 0,
	.initialized	= 0,
};
#endif /* #ifdef CONFIG_VIDEO_M5MO */

#ifdef CONFIG_VIDEO_S5K6AAFX
static int s5k6aafx_get_i2c_busnum(void)
{
#if !defined(CONFIG_MACH_C1_REV00)
	if (system_rev >= 3)
		return 12;
	else
		return 6;
#else
	return 12;
#endif
}

static int s5k6aafx_power_on(void)
{
#if !defined (CONFIG_MACH_C1_REV00)
	struct regulator *regulator;
#endif
	int err = 0;

#if defined (CONFIG_VIDEO_M5MO)
	err = m5mo_power(1);
	if (err) {
		printk(KERN_ERR "%s ERROR: fail to power on\n", __func__);
		return err;
	}
#endif
	err = gpio_request(GPIO_CAM_IO_EN, "GPE2");
	if (err) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_IO_EN)\n");
		return err;
	}

#if defined(CONFIG_MACH_C1_REV00)
	err = gpio_request(GPIO_VT_CAM_15V, "GPE4");
#else
	err = gpio_request(GPIO_VT_CAM_15V, "GPE2");
#endif
	if (err) {
		printk(KERN_ERR "faile to request gpio(GPIO_VT_CAM_15V)\n");
		return err;
	}
#if defined (CONFIG_MACH_C1_REV00)
	err = gpio_request(GPIO_VT_CAM_18V, "GPE4");
	if (err) {
		printk(KERN_ERR "faile to request gpio(GPIO_VT_CAM_18V)\n");
		return err;
	}
#endif

	err = gpio_request(GPIO_CAM_VGA_nSTBY, "GPL2");
	if (err) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_VGA_nSTBY)\n");
		return err;
	}

	err = gpio_request(GPIO_CAM_VGA_nRST, "GPL2");
	if (err) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_VGA_nRST)\n");
		return err;
	}

	/* CAM_SENSOR_A2.8V, CAM_IO_EN */
	gpio_direction_output(GPIO_CAM_IO_EN, 1);
	mdelay(1);

	/* VT_CAM_1.5V */
	gpio_direction_output(GPIO_VT_CAM_15V, 1);
	mdelay(1);

	/* VT_CAM_1.8V */
#if defined (CONFIG_MACH_C1_REV00)
	gpio_direction_output(GPIO_VT_CAM_18V, 1);
#else
	regulator = regulator_get(NULL, "vt_cam_1.8v");
	if (IS_ERR(regulator))
		return -ENODEV;
	regulator_enable(regulator);
	regulator_put(regulator);
#endif
	mdelay(1);

	/* CAM_VGA_nSTBY */
	gpio_direction_output(GPIO_CAM_VGA_nSTBY, 1);
	udelay(500);

	/* Mclk */
	s3c_gpio_cfgpin(GPIO_CAM_MCLK, S3C_GPIO_SFN(2));
	s3c_gpio_setpull(GPIO_CAM_MCLK, S3C_GPIO_PULL_NONE);
	udelay(500);

	/* CAM_VGA_nRST	 */
	gpio_direction_output(GPIO_CAM_VGA_nRST, 1);
	mdelay(2);

	gpio_free(GPIO_CAM_IO_EN);
	gpio_free(GPIO_VT_CAM_15V);
#if defined (CONFIG_MACH_C1_REV00)
	gpio_free(GPIO_VT_CAM_18V);
#endif
	gpio_free(GPIO_CAM_VGA_nSTBY);
	gpio_free(GPIO_CAM_VGA_nRST);

	return 0;
}

static int s5k6aafx_power_off(void)
{
#if !defined (CONFIG_MACH_C1_REV00)
	struct regulator *regulator;
#endif
	int err = 0;

#if defined (CONFIG_VIDEO_M5MO)
	err = m5mo_power(0);
	if (err) {
		printk(KERN_ERR "%s ERROR: fail to power down\n", __func__);
		return err;
	}
#endif

	err = gpio_request(GPIO_CAM_IO_EN, "GPE2");
	if (err) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_IO_EN)\n");
		return err;
	}

#if defined(CONFIG_MACH_C1_REV00)
	err = gpio_request(GPIO_VT_CAM_15V, "GPE4");
#else
	err = gpio_request(GPIO_VT_CAM_15V, "GPE2");
#endif
	if (err) {
		printk(KERN_ERR "faile to request gpio(GPIO_VT_CAM_15V)\n");
		return err;
	}

#if defined (CONFIG_MACH_C1_REV00)
	err = gpio_request(GPIO_VT_CAM_18V, "GPE4");
	if (err) {
		printk(KERN_ERR "faile to request gpio(GPIO_VT_CAM_18V)\n");
		return err;
	}
#endif

	err = gpio_request(GPIO_CAM_VGA_nSTBY, "GPL2");
	if (err) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_VGA_nSTBY)\n");
		return err;
	}

	err = gpio_request(GPIO_CAM_VGA_nRST, "GPL2");
	if (err) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_VGA_nRST)\n");
		return err;
	}

	/* CAM_VGA_nRST	 */
	gpio_direction_output(GPIO_CAM_VGA_nRST, 0);
	udelay(200);

	/* Mclk */
	s3c_gpio_cfgpin(GPIO_CAM_MCLK, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_CAM_MCLK, S3C_GPIO_PULL_DOWN);
	udelay(200);

	/* CAM_VGA_nSTBY */
	gpio_direction_output(GPIO_CAM_VGA_nSTBY, 0);
	udelay(200);

	/* VT_CAM_1.8V */
#if defined (CONFIG_MACH_C1_REV00)
	gpio_direction_output(GPIO_VT_CAM_18V, 0);
#else
	regulator = regulator_get(NULL, "vt_cam_1.8v");
	if (IS_ERR(regulator))
		return -ENODEV;
	if (regulator_is_enabled(regulator))
		regulator_force_disable(regulator);
	regulator_put(regulator);
#endif
	mdelay(1);

	/* VT_CAM_1.5V */
	gpio_direction_output(GPIO_VT_CAM_15V, 0);
	mdelay(1);

	/* CAM_SENSOR_A2.8V, CAM_IO_EN */
	gpio_direction_output(GPIO_CAM_IO_EN, 0);

	gpio_free(GPIO_CAM_IO_EN);
	gpio_free(GPIO_VT_CAM_15V);
#if defined (CONFIG_MACH_C1_REV00)
	gpio_free(GPIO_VT_CAM_18V);
#endif
	gpio_free(GPIO_CAM_VGA_nSTBY);
	gpio_free(GPIO_CAM_VGA_nRST);

	return 0;
}

static int s5k6aafx_power(int onoff)
{
	int ret = 0;

	printk("%s(): %s \n", __func__, onoff ? "on" : "down");

	if (onoff)
		ret = s5k6aafx_power_on();
	else {
		ret = s5k6aafx_power_off();
		/* s3c_i2c0_force_stop();*/ /* DSLIM. Should be implemented */
	}

	return ret;
}

static struct s5k6aafx_platform_data s5k6aafx_plat = {
	.default_width = 640,
	.default_height = 480,
	.pixelformat = V4L2_PIX_FMT_UYVY,
#if defined (CONFIG_VIDEO_S5K6AAFX_MIPI)
	.freq = 24000000,
	.is_mipi = 1,
#endif
};

static struct i2c_board_info  s5k6aafx_i2c_info = {
	I2C_BOARD_INFO("S5K6AAFX", 0x78 >> 1),
	.platform_data = &s5k6aafx_plat,
};

static struct s3c_platform_camera s5k6aafx = {
#if defined (CONFIG_VIDEO_S5K6AAFX_MIPI)
	.id		= CAMERA_CSI_D,
	.type		= CAM_TYPE_MIPI,
	.fmt		= ITU_601_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_CBYCRY,

	.mipi_lanes	= 1,
	.mipi_settle	= 6,
	.mipi_align	= 32,
#else
	.id		= CAMERA_PAR_A,
	.type		= CAM_TYPE_ITU,
	.fmt		= ITU_601_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_YCBYCR,
#endif
	.get_i2c_busnum	= s5k6aafx_get_i2c_busnum,
	.info		= &s5k6aafx_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_UYVY,
	.srclk_name	= "xusbxti",
	.clk_name	= "sclk_cam0",
	.clk_rate	= 24000000,
	.line_length	= 640,
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
	.reset_camera	= 0,
	.initialized	= 0,
	.cam_power	= s5k6aafx_power,
};
#endif

#ifdef CONFIG_VIDEO_S5K5BAFX
static int s5k5bafx_get_i2c_busnum(void)
{
	if (system_rev >= 3)
		return 12;
	else
		return 6;
}

static int s5k5bafx_power_on(void)
{
	struct regulator *regulator;
	int ret = 0;

	/* printk("%s: in\n", __func__); */

	ret = gpio_request(GPIO_ISP_RESET, "GPY3");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(ISP_RESET)\n");
		return ret;
	}
	ret = gpio_request(GPIO_CAM_IO_EN, "GPE2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_IO_EN)\n");
		return ret;
	}
	ret = gpio_request(GPIO_VT_CAM_15V, "GPE2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_VT_CAM_15V)\n");
		return ret;
	}
	ret = gpio_request(GPIO_CAM_VGA_nSTBY, "GPL2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_VGA_nSTBY)\n");
		return ret;
	}
	ret = gpio_request(GPIO_CAM_VGA_nRST, "GPL2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_VGA_nRST)\n");
		return ret;
	}

	if (system_rev >= 9) {
		s3c_gpio_setpull(VT_CAM_SDA_18V, S3C_GPIO_PULL_NONE);
		s3c_gpio_setpull(VT_CAM_SCL_18V, S3C_GPIO_PULL_NONE);
	}

	/* ISP_RESET low*/
	ret = gpio_direction_output(GPIO_ISP_RESET, 0);
	CAM_CHECK_ERR_RET(ret, "output reset");
	udelay(100);

	/* CAM_ISP_CORE_1.2V */
	regulator = regulator_get(NULL, "cam_isp_core");
	if (IS_ERR(regulator))
		return -ENODEV;
	ret = regulator_enable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR_RET(ret, "enable isp_core");
	udelay(10);

	/* CAM_SENSOR_A2.8V */
	ret = gpio_direction_output(GPIO_CAM_IO_EN, 1);
	CAM_CHECK_ERR_RET(ret, "output io_en");
	udelay(300); /* don't change me */

	/* VT_CORE_1.5V */
	ret = gpio_direction_output(GPIO_VT_CAM_15V, 1);
	CAM_CHECK_ERR_RET(ret, "output vt_15v");
	udelay(100);

	/* CAM_ISP_1.8V */
	regulator = regulator_get(NULL, "cam_isp");
	if (IS_ERR(regulator))
		return -ENODEV;
	ret = regulator_enable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR_RET(ret, "enable cam_isp");
	udelay(10);

	/* VT_CAM_1.8V */
	regulator = regulator_get(NULL, "vt_cam_1.8v");
	if (IS_ERR(regulator))
		return -ENODEV;
	ret = regulator_enable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR_RET(ret, "enable vt_1.8v");
	udelay(10);

	/* CAM_VGA_nSTBY */
	ret = gpio_direction_output(GPIO_CAM_VGA_nSTBY, 1);
	CAM_CHECK_ERR_RET(ret, "output VGA_nSTBY");
	udelay(50);

	/* Mclk */
	ret = s3c_gpio_cfgpin(GPIO_CAM_MCLK, S3C_GPIO_SFN(2));
	s3c_gpio_setpull(GPIO_CAM_MCLK, S3C_GPIO_PULL_NONE);
	CAM_CHECK_ERR_RET(ret, "cfg mclk");
	udelay(100);

	/* CAM_VGA_nRST	 */
	ret = gpio_direction_output(GPIO_CAM_VGA_nRST, 1);
	CAM_CHECK_ERR_RET(ret, "output VGA_nRST");
	mdelay(2);

	gpio_free(GPIO_ISP_RESET);
	gpio_free(GPIO_CAM_IO_EN);
	gpio_free(GPIO_VT_CAM_15V);
	gpio_free(GPIO_CAM_VGA_nSTBY);
	gpio_free(GPIO_CAM_VGA_nRST);

	return 0;
}

static int s5k5bafx_power_off(void)
{
	struct regulator *regulator;
	int ret = 0;

	/* printk("n%s: in\n", __func__); */

	ret = gpio_request(GPIO_CAM_VGA_nRST, "GPL2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_VGA_nRST)\n");
		return ret;
	}
	ret = gpio_request(GPIO_CAM_VGA_nSTBY, "GPL2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_VGA_nSTBY)\n");
		return ret;
	}
	ret = gpio_request(GPIO_VT_CAM_15V, "GPE2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_VT_CAM_15V)\n");
		return ret;
	}
	ret = gpio_request(GPIO_CAM_IO_EN, "GPE2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_IO_EN)\n");
		return ret;
	}

	/* CAM_VGA_nRST	 */
	ret = gpio_direction_output(GPIO_CAM_VGA_nRST, 0);
	CAM_CHECK_ERR(ret, "output VGA_nRST");
	udelay(100);

	/* Mclk */
	ret = s3c_gpio_cfgpin(GPIO_CAM_MCLK, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_CAM_MCLK, S3C_GPIO_PULL_DOWN);
	CAM_CHECK_ERR(ret, "cfg mclk");
	udelay(20);

	/* CAM_VGA_nSTBY */
	ret = gpio_direction_output(GPIO_CAM_VGA_nSTBY, 0);
	CAM_CHECK_ERR(ret, "output VGA_nSTBY");
	udelay(20);

	/* VT_CAM_1.8V */
	regulator = regulator_get(NULL, "vt_cam_1.8v");
	if (IS_ERR(regulator))
		return -ENODEV;
	if (regulator_is_enabled(regulator))
		ret = regulator_disable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR(ret, "disable vt_1.8v");
	udelay(10);

	/* CAM_ISP_1.8V */
	regulator = regulator_get(NULL, "cam_isp");
	if (IS_ERR(regulator))
		return -ENODEV;
	if (regulator_is_enabled(regulator))
		ret = regulator_force_disable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR(ret, "disable cam_isp");
	udelay(10);

	/* VT_CORE_1.5V */
	ret = gpio_direction_output(GPIO_VT_CAM_15V, 0);
	CAM_CHECK_ERR(ret, "output vt_1.5v");
	udelay(10);

	/* CAM_SENSOR_A2.8V */
	ret = gpio_direction_output(GPIO_CAM_IO_EN, 0);
	CAM_CHECK_ERR(ret, "output io_en");
	udelay(10);

	/* CAM_ISP_CORE_1.2V */
	regulator = regulator_get(NULL, "cam_isp_core");
	if (IS_ERR(regulator))
		return -ENODEV;
	if (regulator_is_enabled(regulator))
		ret = regulator_force_disable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR(ret, "disable isp_core");

	if (system_rev >= 9) {
		gpio_direction_input(VT_CAM_SDA_18V);
		s3c_gpio_setpull(VT_CAM_SDA_18V, S3C_GPIO_PULL_DOWN);
		gpio_direction_input(VT_CAM_SCL_18V);
		s3c_gpio_setpull(VT_CAM_SCL_18V, S3C_GPIO_PULL_DOWN);
	}

	gpio_free(GPIO_CAM_VGA_nRST);
	gpio_free(GPIO_CAM_VGA_nSTBY);
	gpio_free(GPIO_VT_CAM_15V);
	gpio_free(GPIO_CAM_IO_EN);

	return 0;
}

static int s5k5bafx_power(int onoff)
{
	int ret = 0;

	printk(KERN_INFO "%s(): %s\n", __func__, onoff ? "on" : "down");
	if (onoff) {
		ret = s5k5bafx_power_on();
		if (unlikely(ret))
			goto error_out;
	} else {
		ret = s5k5bafx_power_off();
		/* s3c_i2c0_force_stop();*/ /* DSLIM. Should be implemented */
	}

	ret = s3c_csis_power(onoff);

error_out:
	return ret;
}

static struct s5k5bafx_platform_data s5k5bafx_plat = {
	.default_width = 640,
	.default_height = 480,
	.pixelformat = V4L2_PIX_FMT_UYVY,
	.freq = 24000000,
	.is_mipi = 1,
};

static struct i2c_board_info  s5k5bafx_i2c_info = {
	I2C_BOARD_INFO("S5K5BAFX", 0x5A >> 1),
	.platform_data = &s5k5bafx_plat,
};

static struct s3c_platform_camera s5k5bafx = {
	.id		= CAMERA_CSI_D,
	.type		= CAM_TYPE_MIPI,
	.fmt		= ITU_601_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_CBYCRY,
	.mipi_lanes	= 1,
	.mipi_settle	= 6,
	.mipi_align	= 32,

	.get_i2c_busnum	= s5k5bafx_get_i2c_busnum,
	.info		= &s5k5bafx_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_UYVY,
	.srclk_name	= "xusbxti",
	.clk_name	= "sclk_cam0",
	.clk_rate	= 24000000,
	.line_length	= 640,
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
	.reset_camera	= 0,
	.initialized	= 0,
	.cam_power	= s5k5bafx_power,
};
#endif

#ifdef CONFIG_VIDEO_S5K5BBGX
static int s5k5bbgx_get_i2c_busnum(void)
{
	return 12;
}

static int s5k5bbgx_power_on(void)
{
	struct regulator *regulator;
	int ret = 0;

	/* printk("%s: in\n", __func__); */

	ret = gpio_request(GPIO_ISP_RESET, "GPY3");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(ISP_RESET)\n");
		return ret;
	}
	ret = gpio_request(GPIO_CAM_IO_EN, "GPE2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_IO_EN)\n");
		return ret;
	}
	ret = gpio_request(GPIO_VT_CAM_15V, "GPE2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_VT_CAM_15V)\n");
		return ret;
	}
	ret = gpio_request(GPIO_CAM_VGA_nSTBY, "GPL2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_VGA_nSTBY)\n");
		return ret;
	}
	ret = gpio_request(GPIO_CAM_VGA_nRST, "GPL2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_VGA_nRST)\n");
		return ret;
	}

	s3c_gpio_setpull(VT_CAM_SDA_18V, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(VT_CAM_SCL_18V, S3C_GPIO_PULL_NONE);

	/* ISP_RESET low*/
	ret = gpio_direction_output(GPIO_ISP_RESET, 0);
	CAM_CHECK_ERR_RET(ret, "output reset");
	udelay(100);

	/* CAM_ISP_CORE_1.2V */
	regulator = regulator_get(NULL, "cam_isp_core");
	if (IS_ERR(regulator))
		return -ENODEV;
	ret = regulator_enable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR_RET(ret, "enable isp_core");
	udelay(10);

	/* CAM_SENSOR_A2.8V */
	ret = gpio_direction_output(GPIO_CAM_IO_EN, 1);
	CAM_CHECK_ERR_RET(ret, "output io_en");
	udelay(300); /* don't change me */

	/* VT_CORE_1.5V */
	ret = gpio_direction_output(GPIO_VT_CAM_15V, 1);
	CAM_CHECK_ERR_RET(ret, "output vt_15v");
	udelay(100);

	/* CAM_ISP_1.8V */
	regulator = regulator_get(NULL, "cam_isp");
	if (IS_ERR(regulator))
		return -ENODEV;
	ret = regulator_enable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR_RET(ret, "enable cam_isp");
	udelay(10);

	/* VT_CAM_1.8V */
	regulator = regulator_get(NULL, "vt_cam_1.8v");
	if (IS_ERR(regulator))
		return -ENODEV;
	ret = regulator_enable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR_RET(ret, "enable vt_1.8v");
	udelay(10);

	/* CAM_VGA_nSTBY */
	ret = gpio_direction_output(GPIO_CAM_VGA_nSTBY, 1);
	CAM_CHECK_ERR_RET(ret, "output VGA_nSTBY");
	udelay(50);

	/* Mclk */
	ret = s3c_gpio_cfgpin(GPIO_CAM_MCLK, S3C_GPIO_SFN(2));
	s3c_gpio_setpull(GPIO_CAM_MCLK, S3C_GPIO_PULL_NONE);
	CAM_CHECK_ERR_RET(ret, "cfg mclk");
	udelay(100);

	/* CAM_VGA_nRST	 */
	ret = gpio_direction_output(GPIO_CAM_VGA_nRST, 1);
	CAM_CHECK_ERR_RET(ret, "output VGA_nRST");
	udelay(100);

	gpio_free(GPIO_ISP_RESET);
	gpio_free(GPIO_CAM_IO_EN);
	gpio_free(GPIO_VT_CAM_15V);
	gpio_free(GPIO_CAM_VGA_nSTBY);
	gpio_free(GPIO_CAM_VGA_nRST);

	return 0;
}

static int s5k5bbgx_power_off(void)
{
	struct regulator *regulator;
	int ret = 0;

	/* printk("n%s: in\n", __func__); */

	ret = gpio_request(GPIO_CAM_VGA_nRST, "GPL2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_VGA_nRST)\n");
		return ret;
	}
	ret = gpio_request(GPIO_CAM_VGA_nSTBY, "GPL2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_VGA_nSTBY)\n");
		return ret;
	}
	ret = gpio_request(GPIO_VT_CAM_15V, "GPE2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_VT_CAM_15V)\n");
		return ret;
	}
	ret = gpio_request(GPIO_CAM_IO_EN, "GPE2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_IO_EN)\n");
		return ret;
	}

	/* CAM_VGA_nRST	 */
	ret = gpio_direction_output(GPIO_CAM_VGA_nRST, 0);
	CAM_CHECK_ERR(ret, "output VGA_nRST");
	udelay(100);

	/* Mclk */
	ret = s3c_gpio_cfgpin(GPIO_CAM_MCLK, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_CAM_MCLK, S3C_GPIO_PULL_DOWN);
	CAM_CHECK_ERR(ret, "cfg mclk");
	udelay(20);

	/* CAM_VGA_nSTBY */
	ret = gpio_direction_output(GPIO_CAM_VGA_nSTBY, 0);
	CAM_CHECK_ERR(ret, "output VGA_nSTBY");
	udelay(20);

	/* VT_CAM_1.8V */
	regulator = regulator_get(NULL, "vt_cam_1.8v");
	if (IS_ERR(regulator))
		return -ENODEV;
	if (regulator_is_enabled(regulator))
		ret = regulator_disable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR(ret, "disable vt_1.8v");
	udelay(10);

	/* CAM_ISP_1.8V */
	regulator = regulator_get(NULL, "cam_isp");
	if (IS_ERR(regulator))
		return -ENODEV;
	if (regulator_is_enabled(regulator))
		ret = regulator_force_disable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR(ret, "disable cam_isp");
	udelay(10);

	/* VT_CORE_1.5V */
	ret = gpio_direction_output(GPIO_VT_CAM_15V, 0);
	CAM_CHECK_ERR(ret, "output vt_1.5v");
	udelay(10);

	/* CAM_SENSOR_A2.8V */
	ret = gpio_direction_output(GPIO_CAM_IO_EN, 0);
	CAM_CHECK_ERR(ret, "output io_en");
	udelay(10);

	/* CAM_ISP_CORE_1.2V */
	regulator = regulator_get(NULL, "cam_isp_core");
	if (IS_ERR(regulator))
		return -ENODEV;
	if (regulator_is_enabled(regulator))
		ret = regulator_force_disable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR(ret, "disable isp_core");

	gpio_direction_input(VT_CAM_SDA_18V);
	s3c_gpio_setpull(VT_CAM_SDA_18V, S3C_GPIO_PULL_DOWN);
	gpio_direction_input(VT_CAM_SCL_18V);
	s3c_gpio_setpull(VT_CAM_SCL_18V, S3C_GPIO_PULL_DOWN);

	gpio_free(GPIO_CAM_VGA_nRST);
	gpio_free(GPIO_CAM_VGA_nSTBY);
	gpio_free(GPIO_VT_CAM_15V);
	gpio_free(GPIO_CAM_IO_EN);

	return 0;
}

static int s5k5bbgx_power(int onoff)
{
	int ret = 0;

	printk(KERN_INFO "%s(): %s\n", __func__, onoff ? "on" : "down");
	if (onoff) {
		ret = s5k5bbgx_power_on();
		if (unlikely(ret))
			goto error_out;
	} else {
		ret = s5k5bbgx_power_off();
		/* s3c_i2c0_force_stop();*/ /* DSLIM. Should be implemented */
	}

	/* ret = s3c_csis_power(onoff); */

error_out:
	return ret;
}

static struct s5k5bbgx_platform_data s5k5bbgx_plat = {
	.default_width = 640,
	.default_height = 480,
	.pixelformat = V4L2_PIX_FMT_UYVY,
	.freq = 24000000,
	.is_mipi = 0,
};

static struct i2c_board_info  s5k5bbgx_i2c_info = {
	I2C_BOARD_INFO("S5K5BBGX", 0x5A >> 1),
	.platform_data = &s5k5bbgx_plat,
};

static struct s3c_platform_camera s5k5bbgx = {
#if defined (CONFIG_VIDEO_S5K5BBGX_MIPI)
	.id		= CAMERA_CSI_D,
	.type		= CAM_TYPE_MIPI,
	.fmt		= ITU_601_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_CBYCRY,

	.mipi_lanes	= 1,
	.mipi_settle	= 6,
	.mipi_align	= 32,
#else
	.id		= CAMERA_PAR_A,
	.type		= CAM_TYPE_ITU,
	.fmt		= ITU_601_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_YCBYCR,
#endif
	.get_i2c_busnum	= s5k5bbgx_get_i2c_busnum,
	.info		= &s5k5bbgx_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_UYVY,
	.srclk_name	= "xusbxti",
	.clk_name	= "sclk_cam0",
	.clk_rate	= 24000000,
	.line_length	= 640,
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
	.reset_camera	= 0,
	.initialized	= 0,
	.cam_power	= s5k5bbgx_power,
};
#endif


#ifdef WRITEBACK_ENABLED
static int get_i2c_busnum_writeback(void)
{
	return 0;
}

static struct i2c_board_info  writeback_i2c_info = {
	I2C_BOARD_INFO("WriteBack", 0x0),
};

static struct s3c_platform_camera writeback = {
	.id		= CAMERA_WB,
	.fmt		= ITU_601_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_CBYCRY,
	.get_i2c_busnum	= get_i2c_busnum_writeback,
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

void cam_cfg_gpio(struct platform_device *pdev)
{
	int ret = 0;
	printk(KERN_INFO "\n\n\n%s: pdev->id=%d\n", __func__, pdev->id);

	if (pdev->id != 0)
		return;

#ifdef CONFIG_VIDEO_S5K5BAFX
	if (system_rev >= 9) {/* Rev0.9 */
		ret = gpio_direction_input(VT_CAM_SDA_18V);
		CAM_CHECK_ERR(ret, "VT_CAM_SDA_18V");
		s3c_gpio_setpull(VT_CAM_SDA_18V, S3C_GPIO_PULL_DOWN);

		ret = gpio_direction_input(VT_CAM_SCL_18V);
		CAM_CHECK_ERR(ret, "VT_CAM_SCL_18V");
		s3c_gpio_setpull(VT_CAM_SCL_18V, S3C_GPIO_PULL_DOWN);
	}
#endif
}

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
	.camera		= {
#ifdef CONFIG_VIDEO_M5MO
		&m5mo,
#endif
#ifdef CONFIG_VIDEO_S5K5BBGX
		&s5k5bbgx,
#endif
#ifdef CONFIG_VIDEO_S5K5BAFX
		&s5k5bafx,
#endif
#ifdef CONFIG_VIDEO_S5K6AAFX
		&s5k6aafx,
#endif
#ifdef WRITEBACK_ENABLED
		&writeback,
#endif
	},
	.hw_ver		= 0x51,
#ifndef CONFIG_VIDEO_S5K5BBGX
	.cfg_gpio	= cam_cfg_gpio,
#endif
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
	.name	       = "smsc911x",
	.id	       = -1,
	.num_resources = ARRAY_SIZE(smdkc210_smsc911x_resources),
	.resource      = smdkc210_smsc911x_resources,
	.dev = {
		.platform_data = &smsc9215,
	},
};

#ifdef CONFIG_REGULATOR_MAX8649
static struct regulator_consumer_supply max8952_supply[] = {
	REGULATOR_SUPPLY("vdd_arm", NULL),
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

static struct max8649_platform_data s5pv310_max8952_info = {
	.mode		= 3,	/* VID1 = 1, VID0 = 1 */
	.extclk		= 0,
	.ramp_timing	= MAX8649_RAMP_32MV,
	.regulator	= &max8952_init_data,
};
#endif /* CONFIG_REGULATOR_MAX8649 */

#ifdef CONFIG_REGULATOR_MAX8998
static struct regulator_consumer_supply ldo3_supply[] = {
	REGULATOR_SUPPLY("vusb_1.1v", NULL),
	REGULATOR_SUPPLY("ldo3", NULL),
};

static struct regulator_consumer_supply ldo7_supply[] = {
	REGULATOR_SUPPLY("ldo7", NULL),
};

static struct regulator_consumer_supply ldo8_supply[] = {
	REGULATOR_SUPPLY("vusb_3.3v", NULL),
	REGULATOR_SUPPLY("ldo8", NULL),
};

static struct regulator_consumer_supply ldo17_supply[] = {
	REGULATOR_SUPPLY("ldo17", NULL),
};

static struct regulator_consumer_supply ldo13_supply[] = {
	REGULATOR_SUPPLY("vhsic", NULL),
};

static struct regulator_consumer_supply buck1_supply[] = {
	REGULATOR_SUPPLY("buck1", NULL),
};

static struct regulator_consumer_supply buck2_supply[] = {
	REGULATOR_SUPPLY("buck2", NULL),
};

static struct regulator_consumer_supply safeout2_supply[] = {
	REGULATOR_SUPPLY("safeout2", NULL),
};

static struct regulator_init_data ldo3_init_data = {
	.constraints	= {
		.name		= "ldo3 range",
		.min_uV		= 1100000,
		.max_uV		= 1100000,
		.always_on	= 1,
		.boot_on	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &ldo3_supply[0],
};

static struct regulator_init_data ldo7_init_data = {
	.constraints	= {
		.name		= "ldo7 range",
		.min_uV		= 1800000,
		.max_uV		= 1800000,
		.always_on	= 1,
		.boot_on	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE,
		.state_mem	= {
			.uV		= 1200000,
			.disabled	= 1,
		},
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &ldo7_supply[0],
};

static struct regulator_init_data ldo8_init_data = {
	.constraints	= {
		.name		= "ldo8 range",
		.min_uV		= 3300000,
		.max_uV		= 3300000,
		.always_on	= 1,
		.boot_on	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &ldo8_supply[0],
};

static struct regulator_init_data ldo13_init_data = {
	.constraints	= {
		.name		= "vhsic range",
		.min_uV		= 1200000,
		.max_uV		= 1200000,
		.apply_uV	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.enabled = 0,
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo13_supply),
	.consumer_supplies	= ldo13_supply,
};

static struct regulator_init_data ldo17_init_data = {
	.constraints	= {
		.name		= "ldo17 range",
		.min_uV		= 3000000,
		.max_uV		= 3000000,
		.always_on	= 1,
		.boot_on	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &ldo17_supply[0],
};

static struct regulator_init_data buck1_init_data = {
	.constraints	= {
		.name		= "buck1 range",
		.min_uV		= 1100000,
		.max_uV		= 1100000,
		.always_on	= 1,
		.boot_on	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &buck1_supply[0],
};

static struct regulator_init_data buck2_init_data = {
	.constraints	= {
		.name		= "buck2 range",
		.min_uV		= 1100000,
		.max_uV		= 1100000,
		.always_on	= 1,
		.boot_on	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &buck2_supply[0],
};

static struct regulator_init_data safeout2_init_data = {
	.constraints	= {
		.name		= "safeout2 range",
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.enabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(safeout2_supply),
	.consumer_supplies	= safeout2_supply,
};

static struct max8998_regulator_data max8998_regulators[] = {
	{ MAX8998_LDO3, &ldo3_init_data, },
	{ MAX8998_LDO7, &ldo7_init_data, },
	{ MAX8998_LDO8, &ldo8_init_data, },
	{ MAX8998_LDO13, &ldo13_init_data, },
	{ MAX8998_LDO17, &ldo17_init_data, },
	{ MAX8998_BUCK1, &buck1_init_data, },
	{ MAX8998_BUCK2, &buck2_init_data, },
	{ MAX8998_ESAFEOUT2, &safeout2_init_data, },
};

static struct max8998_power_data max8998_power = {
	.batt_detect = 1,
};

static struct max8998_platform_data s5pv310_max8998_info = {
	.num_regulators = ARRAY_SIZE(max8998_regulators),
	.regulators	= max8998_regulators,
	.irq_base	= IRQ_BOARD_START,
	.buck1_max_voltage1 = 1100000,
	.buck1_max_voltage2 = 1100000,
	.buck2_max_voltage = 1100000,
	.buck1_set1 = GPIO_BUCK1_EN_A,
	.buck1_set2 = GPIO_BUCK1_EN_B,
	.buck2_set3 = GPIO_BUCK2_EN,
	.power = &max8998_power,
};
#endif /* CONFIG_REGULATOR_MAX8998 */

static int c1_charger_topoff_cb(void)
{
	struct power_supply *psy = power_supply_get_by_name("battery");
	union power_supply_propval value;

	if (!psy) {
		pr_err("%s: fail to get battery ps\n", __func__);
		return -ENODEV;
	}

	value.intval = POWER_SUPPLY_STATUS_FULL;
	return psy->set_property(psy, POWER_SUPPLY_PROP_STATUS, &value);
}

#ifdef CONFIG_REGULATOR_MAX8997
static struct regulator_consumer_supply ldo1_supply[] = {
	REGULATOR_SUPPLY("vadc_3.3v", NULL),
};

static struct regulator_consumer_supply ldo3_supply[] = {
	/* stk.lim: Mixed both TN and LSI code while resolving merge */
	REGULATOR_SUPPLY("vusb_1.1v", "s5p-ehci"),
	REGULATOR_SUPPLY("vusb_1.1v", "s3c-usbgadget"),
	REGULATOR_SUPPLY("vmipi_1.1v", "s3c-fimc.0"),
	REGULATOR_SUPPLY("vmipi_1.1v", NULL),
};

static struct regulator_consumer_supply ldo4_supply[] = {
	REGULATOR_SUPPLY("vmipi_1.8v", NULL),
};

static struct regulator_consumer_supply ldo5_supply[] = {
	REGULATOR_SUPPLY("vhsic", NULL),
};

static struct regulator_consumer_supply ldo7_supply[] = {
	REGULATOR_SUPPLY("cam_isp", NULL),
};

static struct regulator_consumer_supply ldo8_supply[] = {
	REGULATOR_SUPPLY("vusb_3.3v", NULL),
};

#if defined(CONFIG_S5PV310_HI_ARMCLK_THAN_1_2GHZ)
static struct regulator_consumer_supply ldo10_supply[] = {
	REGULATOR_SUPPLY("vpll_1.2v", NULL),
};
#else
static struct regulator_consumer_supply ldo10_supply[] = {
	REGULATOR_SUPPLY("vpll_1.1v", NULL),
};
#endif

static struct regulator_consumer_supply ldo11_supply[] = {
	REGULATOR_SUPPLY("touch", NULL),
};

static struct regulator_consumer_supply ldo12_supply[] = {
#if defined (CONFIG_MACH_C1_REV01) || defined (CONFIG_MACH_C1_REV02) || defined(CONFIG_MACH_C1Q1_REV02) \
 || defined(CONFIG_MACH_P6_REV02) || defined(CONFIG_MACH_TALBOT_REV02) || defined(CONFIG_TARGET_LOCALE_NA)
	REGULATOR_SUPPLY("vt_cam_1.8v", NULL),
#else
	REGULATOR_SUPPLY("cam_sensor_core", NULL),
#endif
};

static struct regulator_consumer_supply ldo13_supply[] = {
	REGULATOR_SUPPLY("vlcd_3.0v", NULL),
};

static struct regulator_consumer_supply ldo14_supply[] = {
	REGULATOR_SUPPLY("vmotor", NULL),
};

static struct regulator_consumer_supply ldo15_supply[] = {
	REGULATOR_SUPPLY("vled", NULL),
};

static struct regulator_consumer_supply ldo16_supply[] = {
	REGULATOR_SUPPLY("cam_sensor_io", NULL),
};

static struct regulator_consumer_supply ldo17_supply[] = {
#if defined(CONFIG_MACH_C1_REV02) || defined(CONFIG_MACH_C1Q1_REV02) \
 || defined(CONFIG_MACH_P6_REV02) || defined(CONFIG_MACH_TALBOT_REV02) || defined(CONFIG_TARGET_LOCALE_NA)
	REGULATOR_SUPPLY("vt_cam_core_1.8v", NULL),
#else
	REGULATOR_SUPPLY("cam_af", NULL),
#endif
};

static struct regulator_consumer_supply ldo17_rev04_supply[] = {
	REGULATOR_SUPPLY("vtf_2.8v", NULL),
};

static struct regulator_consumer_supply ldo18_supply[] = {
	REGULATOR_SUPPLY("touch_led", NULL),
};

static struct regulator_consumer_supply ldo21_supply[] = {
	REGULATOR_SUPPLY("vddq_m1m2", NULL),
};

static struct regulator_consumer_supply buck1_supply[] = {
	REGULATOR_SUPPLY("vdd_arm", NULL),
};

static struct regulator_consumer_supply buck2_supply[] = {
	REGULATOR_SUPPLY("vdd_int", NULL),
};

static struct regulator_consumer_supply buck3_supply[] = {
	REGULATOR_SUPPLY("vdd_g3d", NULL),
};

static struct regulator_consumer_supply buck4_supply[] = {
	REGULATOR_SUPPLY("cam_isp_core", NULL),
};

static struct regulator_consumer_supply buck7_supply[] = {
	REGULATOR_SUPPLY("vcc_sub", NULL),
};

static struct regulator_consumer_supply safeout1_supply[] = {
	REGULATOR_SUPPLY("safeout1", NULL),
};

static struct regulator_consumer_supply safeout2_supply[] = {
	REGULATOR_SUPPLY("safeout2", NULL),
};

static struct regulator_consumer_supply led_flash_supply[] = {
	REGULATOR_SUPPLY("led_flash", NULL),
};

static struct regulator_consumer_supply led_movie_supply[] = {
	REGULATOR_SUPPLY("led_movie", NULL),
};

#define REGULATOR_INIT(_ldo, _name, _min_uV, _max_uV, _always_on, _ops_mask,\
		_disabled) \
	static struct regulator_init_data _ldo##_init_data = {		\
		.constraints = {					\
			.name	= _name,				\
			.min_uV = _min_uV,				\
			.max_uV = _max_uV,				\
			.always_on	= _always_on,			\
			.boot_on	= _always_on,			\
			.apply_uV	= 1,				\
			.valid_ops_mask = _ops_mask,			\
			.state_mem	= {				\
				.disabled	= _disabled,		\
				.enabled	= !(_disabled),		\
			}						\
		},							\
		.num_consumer_supplies = ARRAY_SIZE(_ldo##_supply),	\
		.consumer_supplies = &_ldo##_supply[0],			\
	};

REGULATOR_INIT(ldo1, "VADC_3.3V_C210", 3300000, 3300000, 1,
		REGULATOR_CHANGE_STATUS, 1);
REGULATOR_INIT(ldo3, "VUSB_1.1V", 1100000, 1100000, 1,
		REGULATOR_CHANGE_STATUS, 1);
REGULATOR_INIT(ldo4, "VMIPI_1.8V", 1800000, 1800000, 1,
		REGULATOR_CHANGE_STATUS, 1);
REGULATOR_INIT(ldo5, "VHSIC_1.2V", 1200000, 1200000, 1,
		REGULATOR_CHANGE_STATUS, 1);
REGULATOR_INIT(ldo7, "CAM_ISP_1.8V", 1800000, 1800000, 0,
		REGULATOR_CHANGE_STATUS, 1);
REGULATOR_INIT(ldo8, "VUSB_3.3V", 3300000, 3300000, 1,
		REGULATOR_CHANGE_STATUS, 1);
#if defined(CONFIG_S5PV310_HI_ARMCLK_THAN_1_2GHZ)
REGULATOR_INIT(ldo10, "VPLL_1.2V", 1200000, 1200000, 1,
		REGULATOR_CHANGE_STATUS, 1);
#else
REGULATOR_INIT(ldo10, "VPLL_1.1V", 1100000, 1100000, 1,
		REGULATOR_CHANGE_STATUS, 1);
#endif
#if defined(CONFIG_TARGET_LOCALE_NAATT)
REGULATOR_INIT(ldo11, "TOUCH_2.8V", 3100000, 3100000, 0,
		REGULATOR_CHANGE_STATUS, 1);
#else
REGULATOR_INIT(ldo11, "TOUCH_2.8V", 2800000, 2800000, 0,
		REGULATOR_CHANGE_STATUS, 1);
#endif
#if defined(CONFIG_MACH_C1_REV00)
REGULATOR_INIT(ldo12, "CAM_SENSOR_CORE_1.2V", 1200000, 1200000, 0,
		REGULATOR_CHANGE_STATUS, 1);
#else
REGULATOR_INIT(ldo12, "VT_CAM_1.8V", 1800000, 1800000, 0,
		REGULATOR_CHANGE_STATUS, 1);
#endif
#ifdef CONFIG_FB_S3C_S6E8AA0
REGULATOR_INIT(ldo13, "VCC_3.0V_LCD", 3300000, 3300000, 1,
		REGULATOR_CHANGE_STATUS, 0);
#else
REGULATOR_INIT(ldo13, "VCC_3.0V_LCD", 3000000, 3000000, 1,
		REGULATOR_CHANGE_STATUS, 1);
#endif
REGULATOR_INIT(ldo14, "VCC_2.8V_MOTOR", 2800000, 2800000, 0,
		REGULATOR_CHANGE_STATUS, 1);
REGULATOR_INIT(ldo15, "LED_A_2.8V", 2800000, 2800000, 0,
		REGULATOR_CHANGE_STATUS, 1);
REGULATOR_INIT(ldo16, "CAM_SENSOR_IO_1.8V", 1800000, 1800000, 0,
		REGULATOR_CHANGE_STATUS, 1);
#if defined(CONFIG_MACH_C1_REV02) || defined(CONFIG_MACH_C1Q1_REV02) || defined(CONFIG_MACH_P6_REV02) || defined(CONFIG_MACH_TALBOT_REV02) || defined(CONFIG_TARGET_LOCALE_NA)
REGULATOR_INIT(ldo17, "VT_CAM_CORE_1.8V", 1800000, 1800000, 0,
		REGULATOR_CHANGE_STATUS, 1);
REGULATOR_INIT(ldo17_rev04, "VTF_2.8V", 2800000, 2800000, 0,
		REGULATOR_CHANGE_STATUS, 1);
#else
REGULATOR_INIT(ldo17, "CAM_AF_2.8V", 2800000, 2800000, 0,
		REGULATOR_CHANGE_STATUS, 1);
#endif
REGULATOR_INIT(ldo18, "TOUCH_LED_3.3V", 3000000, 3300000, 0,
		REGULATOR_CHANGE_STATUS | REGULATOR_CHANGE_VOLTAGE, 1);

REGULATOR_INIT(ldo21, "VDDQ_M1M2_1.2V", 1200000, 1200000, 1,
		REGULATOR_CHANGE_STATUS, 1);

static struct regulator_init_data buck1_init_data = {
	.constraints	= {
		.name		= "vdd_arm range",
		.min_uV		= 650000,
		.max_uV		= 2225000,
		.always_on	= 1,
		.boot_on	= 1,
		.apply_uV	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE |
				  REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.mode		= REGULATOR_MODE_NORMAL,
			.disabled	= 1,
		},
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &buck1_supply[0],
};

static struct regulator_init_data buck2_init_data = {
	.constraints	= {
		.name		= "vdd_int range",
		.min_uV		= 650000,
		.max_uV		= 2225000,
		.always_on	= 1,
		.boot_on	= 1,
		.apply_uV	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE |
				  REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.mode		= REGULATOR_MODE_NORMAL,
			.disabled	= 1,
		},
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &buck2_supply[0],
};

static struct regulator_init_data buck3_init_data = {
	.constraints	= {
		.name		= "G3D_1.1V",
		.min_uV		= 900000,
		.max_uV		= 1200000,
		.always_on	= 0,
		.boot_on	= 0,
		.apply_uV	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE |
				  REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.mode		= REGULATOR_MODE_NORMAL,
			.disabled	= 1,
		},
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &buck3_supply[0],
};

static struct regulator_init_data buck4_init_data = {
	.constraints	= {
		.name		= "CAM_ISP_CORE_1.2V",
		.min_uV		= 1200000,
		.max_uV		= 1200000,
		.apply_uV	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled	= 1,
		},
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &buck4_supply[0],
};

static struct regulator_init_data buck5_init_data = {
	.constraints	= {
		.name		= "VMEM_1.2V",
		.min_uV		= 1200000,
		.max_uV		= 1200000,
		.apply_uV	= 1,
		.always_on	= 1,
		.state_mem	= {
			.mode		= REGULATOR_MODE_NORMAL,
			.enabled = 1,
		},
	},
};

static struct regulator_init_data buck7_init_data = {
	.constraints	= {
		.name		= "VCC_SUB_2.0V",
		.min_uV		= 2000000,
		.max_uV		= 2000000,
		.apply_uV	= 1,
		.always_on	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.enabled = 1,
		},
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &buck7_supply[0],
};

static struct regulator_init_data safeout1_init_data = {
	.constraints	= {
		.name		= "safeout1 range",
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.always_on	= 0,
		.boot_on	= 1,
		.state_mem	= {
			.enabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(safeout1_supply),
	.consumer_supplies	= safeout1_supply,
};

static struct regulator_init_data safeout2_init_data = {
	.constraints	= {
		.name		= "safeout2 range",
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.always_on	= 0,
#ifdef CONFIG_TARGET_LOCALE_NA
		.boot_on        = 1,
#else
		.boot_on	= 0,
#endif /* CONFIG_TARGET_LOCALE_NA */
		.state_mem	= {
			.enabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(safeout2_supply),
	.consumer_supplies	= safeout2_supply,
};

static struct regulator_init_data led_flash_init_data = {
	.constraints = {
		.name	= "FLASH_CUR",
		.min_uA = 23440,
		.max_uA = 750080,
		.valid_ops_mask	= REGULATOR_CHANGE_CURRENT |
				  REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled	= 1,
		},
	},
	.num_consumer_supplies = 1,
	.consumer_supplies = &led_flash_supply[0],
};

static struct regulator_init_data led_movie_init_data = {
	.constraints = {
		.name	= "MOVIE_CUR",
		.min_uA = 15625,
		.max_uA = 250000,
		.valid_ops_mask	= REGULATOR_CHANGE_CURRENT |
				  REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled	= 1,
		},
	},
	.num_consumer_supplies = 1,
	.consumer_supplies = &led_movie_supply[0],
};

static int max8997_regulator_is_valid(int id,
		struct regulator_init_data *initdata)
{
	int ret;
	char *regulator_name;

	switch (id) {
	case MAX8997_LDO17:
		if (system_rev >= 0x3)
			regulator_name = "VTF_2.8V";
		else
			regulator_name = "VT_CAM_CORE_1.8V";

		ret = !strcmp(initdata->constraints.name, regulator_name);
		break;
	default:
		ret = 1;
		break;
	}

	return ret;
}

static struct max8997_regulator_data max8997_regulators[] = {
	{ MAX8997_LDO1,	 &ldo1_init_data, NULL, },
	{ MAX8997_LDO3,	 &ldo3_init_data, NULL, },
	{ MAX8997_LDO4,	 &ldo4_init_data, NULL, },
	{ MAX8997_LDO5,	 &ldo5_init_data, NULL, },
	{ MAX8997_LDO7,	 &ldo7_init_data, NULL, },
	{ MAX8997_LDO8,	 &ldo8_init_data, NULL, },
	{ MAX8997_LDO10, &ldo10_init_data, NULL, },
	{ MAX8997_LDO11, &ldo11_init_data, NULL, },
	{ MAX8997_LDO12, &ldo12_init_data, NULL, },
	{ MAX8997_LDO13, &ldo13_init_data, NULL, },
	{ MAX8997_LDO14, &ldo14_init_data, NULL, },
	{ MAX8997_LDO15, &ldo15_init_data, NULL, },
	{ MAX8997_LDO16, &ldo16_init_data, NULL, },
#if defined(CONFIG_MACH_C1_REV02) || defined(CONFIG_MACH_C1Q1_REV02) || defined(CONFIG_MACH_P6_REV02) || defined(CONFIG_MACH_TALBOT_REV02) || defined(CONFIG_TARGET_LOCALE_NA)
	{ MAX8997_LDO17, &ldo17_init_data, max8997_regulator_is_valid, },
	{ MAX8997_LDO17, &ldo17_rev04_init_data,
		max8997_regulator_is_valid, },
#else
	{ MAX8997_LDO17, &ldo17_init_data, NULL, },
#endif
	{ MAX8997_LDO18, &ldo18_init_data, NULL, },
	{ MAX8997_LDO21, &ldo21_init_data, NULL, },
	{ MAX8997_BUCK1, &buck1_init_data, NULL, },
	{ MAX8997_BUCK2, &buck2_init_data, NULL, },
	{ MAX8997_BUCK3, &buck3_init_data, NULL, },
	{ MAX8997_BUCK4, &buck4_init_data, NULL, },
	{ MAX8997_BUCK5, &buck5_init_data, NULL, },
	{ MAX8997_BUCK7, &buck7_init_data, NULL, },
	{ MAX8997_ESAFEOUT1, &safeout1_init_data, NULL, },
	{ MAX8997_ESAFEOUT2, &safeout2_init_data, NULL, },
	{ MAX8997_FLASH_CUR, &led_flash_init_data, NULL, },
	{ MAX8997_MOVIE_CUR, &led_movie_init_data, NULL, },
};

static int max8997_power_set_charger(int insert)
{
	struct power_supply *psy = power_supply_get_by_name("battery");
	union power_supply_propval value;

	if (!psy) {
		pr_err("%s: fail to get battery ps\n", __func__);
		return -ENODEV;
	}

	if (insert)
		value.intval = POWER_SUPPLY_TYPE_MAINS;
	else
		value.intval = POWER_SUPPLY_TYPE_BATTERY;

	return psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE, &value);
}

static struct max8997_power_data max8997_power = {
	.set_charger = max8997_power_set_charger,
	.topoff_cb = c1_charger_topoff_cb,
	.batt_detect = 1,
};

static int max8997_muic_set_safeout(int path)
{
	struct regulator *regulator;

	if (path == CP_USB_MODE) {
		regulator = regulator_get(NULL, "safeout1");
		if (IS_ERR(regulator))
			return -ENODEV;
		if (regulator_is_enabled(regulator))
			regulator_force_disable(regulator);
		regulator_put(regulator);

		regulator = regulator_get(NULL, "safeout2");
		if (IS_ERR(regulator))
			return -ENODEV;
		if (!regulator_is_enabled(regulator))
			regulator_enable(regulator);
		regulator_put(regulator);
	} else {
		/* AP_USB_MODE || AUDIO_MODE */
		regulator = regulator_get(NULL, "safeout1");
		if (IS_ERR(regulator))
			return -ENODEV;
		if (!regulator_is_enabled(regulator))
			regulator_enable(regulator);
		regulator_put(regulator);

		regulator = regulator_get(NULL, "safeout2");
		if (IS_ERR(regulator))
			return -ENODEV;
		if (regulator_is_enabled(regulator))
			regulator_force_disable(regulator);
		regulator_put(regulator);
	}

	return 0;
}

static struct c1_charging_status_callbacks {
	void	(*tsp_set_charging_cable) (int type);
} charging_cbs;

static bool is_cable_attached;

static int max8997_muic_charger_cb(cable_type_t cable_type)
{
	struct power_supply *psy = power_supply_get_by_name("battery");
	union power_supply_propval value;

	if (!psy) {
		pr_err("%s: fail to get battery ps\n", __func__);
		return -ENODEV;
	}

	switch (cable_type) {
	case CABLE_TYPE_NONE:
	case CABLE_TYPE_OTG:
	case CABLE_TYPE_JIG_UART_OFF:
	case CABLE_TYPE_MHL:
		value.intval = POWER_SUPPLY_TYPE_BATTERY;
		is_cable_attached = false;
		break;
	case CABLE_TYPE_USB:
	case CABLE_TYPE_JIG_USB_OFF:
	case CABLE_TYPE_JIG_USB_ON:
		value.intval = POWER_SUPPLY_TYPE_USB;
		is_cable_attached = true;
		break;
	case CABLE_TYPE_MHL_VB:
		value.intval = POWER_SUPPLY_TYPE_MISC;
		is_cable_attached = true;
		break;
	case CABLE_TYPE_TA:
	case CABLE_TYPE_CARDOCK:
	case CABLE_TYPE_DESKDOCK:
	case CABLE_TYPE_JIG_UART_OFF_VB:
		value.intval = POWER_SUPPLY_TYPE_MAINS;
		is_cable_attached = true;
		break;
	default:
		pr_err("%s: invalid type:%d\n", __func__, cable_type);
		return -EINVAL;
	}

	if (charging_cbs.tsp_set_charging_cable)
		charging_cbs.tsp_set_charging_cable(value.intval);

	return psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE, &value);
}

static void max8997_muic_usb_cb(u8 usb_mode)
{
	int prev_usb_mode = 0, ret;
	struct s3c_udc *udc = platform_get_drvdata(&s3c_device_usbgadget);
	struct sec_otghost_data *otg_data =
			dev_get_platdata(&s3c_device_usb_otghcd.dev);

	struct regulator *regulator;
	u32 lpcharging = __raw_readl(S5P_INFORM2);

	pr_info("%s: usb mode=%d\n", __func__, usb_mode);

	if (lpcharging == 1) {
		pr_info("%s: lpcharging: disable USB\n", __func__);
		ret = udc->change_usb_mode(USB_CABLE_DETACHED);
		if (ret < 0)
			pr_warn("%s: fail to change mode!!!\n", __func__);

		regulator = regulator_get(NULL, "safeout1");
		if (IS_ERR(regulator)) {
			pr_err("%s: fail to get regulator\n", __func__);
			return;
		}

		if (regulator_is_enabled(regulator))
			regulator_force_disable(regulator);
		regulator_put(regulator);

		return;
	}

#ifdef CONFIG_TARGET_LOCALE_KOR
	if (usb_access_lock)
		return;
#endif

	if (udc) {
		if (usb_mode == USB_OTGHOST_ATTACHED) {
			otg_data->set_pwr_cb(1);
			max8997_muic_charger_cb(CABLE_TYPE_OTG);
		}

		prev_usb_mode = udc->get_usb_mode();
		pr_info("%s: prev_usb_mode=%d\n", __func__, prev_usb_mode);

		ret = udc->change_usb_mode(usb_mode);
		if (ret < 0)
			pr_err("%s: fail to change mode!!!\n", __func__);

		if (usb_mode == USB_OTGHOST_DETACHED)
			otg_data->set_pwr_cb(0);
	}
}

extern void MHL_On(bool on);
static void max8997_muic_mhl_cb(int attached)
{
	pr_info("%s(%d)\n", __func__, attached);

	if (attached == MAX8997_MUIC_ATTACHED) {
		MHL_On(1);
	} else {
		MHL_On(0);
	}

}

static bool max8997_muic_is_mhl_attached(void)
{
	int val;

	gpio_request(GPIO_MHL_SEL, "MHL_SEL");
	val = gpio_get_value(GPIO_MHL_SEL);
	gpio_free(GPIO_MHL_SEL);

	return !!val;
}

static struct switch_dev switch_dock = {
	.name = "dock",
};

static void max8997_muic_deskdock_cb(bool attached)
{
	if (attached)
		switch_set_state(&switch_dock, 1);
	else
		switch_set_state(&switch_dock, 0);
}

static void max8997_muic_cardock_cb(bool attached)
{
	if (attached)
		switch_set_state(&switch_dock, 2);
	else
		switch_set_state(&switch_dock, 0);
}

static void max8997_muic_init_cb(void)
{
	int ret;

	/* for CarDock, DeskDock */
	ret = switch_dev_register(&switch_dock);
	if (ret < 0)
		pr_err("Failed to register dock switch. %d\n", ret);
}

static int max8997_muic_cfg_uart_gpio(void)
{
	int val, path;

	val = gpio_get_value(GPIO_UART_SEL);
	path =  val ? UART_PATH_AP : UART_PATH_CP;
#if 0
	/* Workaround
	 * Sometimes sleep current is 15 ~ 20mA if UART path was CP.
	 */
	if (path == UART_PATH_CP)
		gpio_set_value(GPIO_UART_SEL, GPIO_LEVEL_HIGH);
#endif
	pr_info("%s: path=%d\n", __func__, path);
	return path;
}

static void max8997_muic_jig_uart_cb(int path)
{
	int val;

	val = path == UART_PATH_AP ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW;
	gpio_set_value(GPIO_UART_SEL, val);
	pr_info("%s: val:%d\n", __func__, val);
}

static int max8997_muic_host_notify_cb(int enable)
{
	struct host_notify_dev *ndev = NULL;

	if (s3c_device_usbgadget.dev.platform_data)
		ndev = s3c_device_usbgadget.dev.platform_data;
	else {
		pr_err("%s: ndev is null.\n", __func__);
		return -1;
	}

	ndev->booster = enable ? NOTIFY_POWER_ON : NOTIFY_POWER_OFF;
	pr_info("%s: mode %d, enable %d\n", __func__, ndev->mode, enable);
	return ndev->mode;
}

static struct max8997_muic_data max8997_muic = {
	.usb_cb = max8997_muic_usb_cb,
	.charger_cb = max8997_muic_charger_cb,
	.mhl_cb = max8997_muic_mhl_cb,
	.is_mhl_attached = max8997_muic_is_mhl_attached,
#ifdef CONFIG_TARGET_LOCALE_NA
	.set_safeout = NULL,
#else
	.set_safeout = max8997_muic_set_safeout,
#endif /* CONFIG_TARGET_LOCALE_NA */
	.init_cb = max8997_muic_init_cb,
	.deskdock_cb = max8997_muic_deskdock_cb,
	.cardock_cb = max8997_muic_cardock_cb,
	.cfg_uart_gpio = max8997_muic_cfg_uart_gpio,
	.jig_uart_cb = max8997_muic_jig_uart_cb,
	.host_notify_cb = max8997_muic_host_notify_cb,
#if defined(CONFIG_TARGET_LOCALE_NA)
	.gpio_uart_sel = GPIO_UART_SEL,
#else
	.gpio_usb_sel = GPIO_USB_SEL,
#endif /* CONFIG_TARGET_LOCALE_NA */
};

static struct max8997_platform_data s5pv310_max8997_info = {
	.num_regulators = ARRAY_SIZE(max8997_regulators),
	.regulators	= &max8997_regulators[0],
	.irq_base	= IRQ_BOARD_START,
	.wakeup		= 1,
#ifdef __USB_GPIO_DVS__
	.buck1_max_voltage1 = 1200000,
	.buck1_max_voltage2 = 1100000,
	.buck1_max_voltage3 = 1000000,
	.buck1_max_voltage4 = 950000,
	.buck2_max_voltage1 = 1100000,
	.buck2_max_voltage2 = 1000000,
	.buck_set1 = GPIO_BUCK1_EN_A,
	.buck_set2 = GPIO_BUCK1_EN_B,
	.buck_set3 = GPIO_BUCK2_EN,
#endif /* __USE_GPIO_DVS__ */
	.buck1_ramp_en = true,
	.buck2_ramp_en = true,
	.buck_ramp_reg_val = 0x9,	/* b1001: 10.00mV /us (default) */
	.flash_cntl_val = 0x53,		/* Flash safety timer duration: 500msec,
					   Maximum timer mode */
	.power = &max8997_power,
	.muic = &max8997_muic,
};
#endif /* CONFIG_REGULATOR_MAX8997 */


static DEFINE_SPINLOCK(mic_bias_lock);
static bool mc1n2_mainmic_bias;
static bool mc1n2_submic_bias;

static void set_shared_mic_bias(void)
{
	if (system_rev >= 0x03)
		gpio_set_value(GPIO_MIC_BIAS_EN, mc1n2_mainmic_bias || mc1n2_submic_bias);
	else
		gpio_set_value(GPIO_EAR_MIC_BIAS_EN, mc1n2_mainmic_bias || mc1n2_submic_bias);
}

void sec_set_sub_mic_bias(bool on)
{
#ifdef CONFIG_SND_SOC_USE_EXTERNAL_MIC_BIAS
	if (system_rev < SYSTEM_REV_SND) {
		unsigned long flags;
		spin_lock_irqsave(&mic_bias_lock, flags);
		mc1n2_submic_bias = on;
		set_shared_mic_bias();
		spin_unlock_irqrestore(&mic_bias_lock, flags);
	} else
		gpio_set_value(GPIO_SUB_MIC_BIAS_EN, on);

#endif
}


void sec_set_main_mic_bias(bool on)
{
#ifdef CONFIG_SND_SOC_USE_EXTERNAL_MIC_BIAS
	if (system_rev < SYSTEM_REV_SND) {
		unsigned long flags;
		spin_lock_irqsave(&mic_bias_lock, flags);
		mc1n2_mainmic_bias = on;
		set_shared_mic_bias();
		spin_unlock_irqrestore(&mic_bias_lock, flags);
	} else
		gpio_set_value(GPIO_MIC_BIAS_EN, on);
#endif
}

void sec_set_ldo1_constraints(int disabled)
{
	/* VDD33_ADC */
	ldo1_init_data.constraints.state_mem.disabled = disabled;
	ldo1_init_data.constraints.state_mem.enabled = !disabled;
}

static struct mc1n2_platform_data mc1n2_pdata = {
	.set_main_mic_bias = sec_set_main_mic_bias,
	.set_sub_mic_bias = sec_set_sub_mic_bias,
	.set_adc_power_contraints = sec_set_ldo1_constraints,
};

static void c1_sound_init(void)
{
#ifdef CONFIG_SND_SOC_USE_EXTERNAL_MIC_BIAS
	int err;

	err = gpio_request(GPIO_MIC_BIAS_EN, "GPE1");
	if (err) {
		pr_err(KERN_ERR "MIC_BIAS_EN GPIO set error!\n");
		return;
	}
	gpio_direction_output(GPIO_MIC_BIAS_EN, 1);
#ifdef CONFIG_TARGET_LOCALE_NA
	s3c_gpio_setpull(GPIO_MIC_BIAS_EN, S3C_GPIO_PULL_NONE);
#endif /* CONFIG_TARGET_LOCALE_NA */
	gpio_set_value(GPIO_MIC_BIAS_EN, 0);
	gpio_free(GPIO_MIC_BIAS_EN);

	err = gpio_request(GPIO_EAR_MIC_BIAS_EN, "GPE2");
	if (err) {
		pr_err(KERN_ERR "EAR_MIC_BIAS_EN GPIO set error!\n");
		return;
	}
	gpio_direction_output(GPIO_EAR_MIC_BIAS_EN, 1);
#ifdef CONFIG_TARGET_LOCALE_NA
	s3c_gpio_setpull(GPIO_EAR_MIC_BIAS_EN, S3C_GPIO_PULL_NONE);
#endif /* CONFIG_TARGET_LOCALE_NA */
	gpio_set_value(GPIO_EAR_MIC_BIAS_EN, 0);
	gpio_free(GPIO_EAR_MIC_BIAS_EN);

	if (system_rev >= SYSTEM_REV_SND) {
		err = gpio_request(GPIO_SUB_MIC_BIAS_EN, "submic_bias");
		if (err) {
			pr_err(KERN_ERR "SUB_MIC_BIAS_EN GPIO set error!\n");
			return;
		}
		gpio_direction_output(GPIO_SUB_MIC_BIAS_EN, 0);
#ifdef CONFIG_TARGET_LOCALE_NA
		s3c_gpio_setpull(GPIO_SUB_MIC_BIAS_EN, S3C_GPIO_PULL_NONE);
#endif /* CONFIG_TARGET_LOCALE_NA */
		gpio_free(GPIO_SUB_MIC_BIAS_EN);
	}
#endif /* #ifdef CONFIG_SND_SOC_USE_EXTERNAL_MIC_BIAS */
}

#if defined(CONFIG_TOUCHSCREEN_QT602240) || defined(CONFIG_TOUCHSCREEN_MXT768E)
static void mxt224_power_on(void)
{
	s3c_gpio_cfgpin(GPIO_TSP_LDO_ON, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TSP_LDO_ON, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_TSP_LDO_ON, GPIO_LEVEL_HIGH);
	mdelay(70);
	s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_SFN(0xf));
	/*mdelay(40); */
	/* printk("mxt224_power_on is finished\n"); */
}

static void mxt224_power_off(void)
{
	s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_DOWN);

	s3c_gpio_cfgpin(GPIO_TSP_LDO_ON, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TSP_LDO_ON, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_TSP_LDO_ON, GPIO_LEVEL_LOW);
	/* printk("mxt224_power_off is finished\n"); */
}

static void mxt224_register_callback(void *function)
{
	charging_cbs.tsp_set_charging_cable = function;
}

static void mxt224_read_ta_status(bool *ta_status)
{
	*ta_status = is_cable_attached;
}

#if defined(CONFIG_MACH_C1Q1_REV02) || defined(CONFIG_MACH_P6_REV02)
#ifdef CONFIG_TOUCHSCREEN_MXT768E
static u8 t7_config[] = {GEN_POWERCONFIG_T7,
				64, 255, 20};
static u8 t8_config[] = {GEN_ACQUISITIONCONFIG_T8,
				64, 0, 20, 20, 0, 0, 20, 0, 50, 25};
static u8 t9_config[] = {TOUCH_MULTITOUCHSCREEN_T9,
				139, 0, 0, 16, 26, 0, 208, 50, 2, 6, 0, 5, 1,
				0, MXT224_MAX_MT_FINGERS, 10, 10, 5, 255, 3,
				255, 3, 0, 0, 0, 0, 136, 60, 136, 40, 40, 15, 0, 0};

static u8 t15_config[] = {TOUCH_KEYARRAY_T15,
				1, 16, 26, 1, 6, 0, 64, 255, 3, 0, 0};

static u8 t18_config[] = {SPT_COMCONFIG_T18,
				0, 0};

static u8 t40_config[] = {PROCI_GRIPSUPPRESSION_T40,
				0, 0, 0, 0, 0};

static u8 t42_config[] = {PROCI_TOUCHSUPPRESSION_T42,
				0, 0, 0, 0, 0, 0, 0, 0};

static u8 t43_config[] = {SPT_DIGITIZER_T43,
				0, 0, 0, 0};

static u8 t48_config[] = {PROCG_NOISESUPPRESSION_T48,
				1, 0, 65, 0, 12, 24, 36, 48, 8, 16, 11, 40, 0, 0,
				0, 0, 0};


static u8 t46_config[] = {SPT_CTECONFIG_T46,
				0, 0, 8, 32, 0, 0, 0, 0};
static u8 end_config[] = {RESERVED_T255};

static const u8 *mxt224_config[] = {
	t7_config,
	t8_config,
	t9_config,
	t15_config,
	t18_config,
	t40_config,
	t42_config,
	t43_config,
	t46_config,
	t48_config,
	end_config,
};
#else
static u8 t7_config[] = {GEN_POWERCONFIG_T7,
				64, 255, 20};
static u8 t8_config[] = {GEN_ACQUISITIONCONFIG_T8,
				36, 0, 20, 20, 0, 0, 10, 10, 50, 25};
static u8 t9_config[] = {TOUCH_MULTITOUCHSCREEN_T9,
				139, 0, 0, 18, 11, 0, 16, MXT224_THRESHOLD, 2, 1, 0, 3, 1,
				0, MXT224_MAX_MT_FINGERS, 10, 10, 10, 31, 3,
				223, 1, 0, 0, 0, 0, 0, 0, 0, 0, 10, 5, 5, 5};

static u8 t15_config[] = {TOUCH_KEYARRAY_T15,
				131, 16, 11, 2, 1, 0, 0, 40, 3, 0, 0};

static u8 t18_config[] = {SPT_COMCONFIG_T18,
				0, 0};

static u8 t48_config[] = {PROCG_NOISESUPPRESSION_T48,
				3, 0, 2, 10, 6, 12, 18, 24, 20, 30, 0, 0, 0, 0,
				0, 0, 0};


static u8 t46_config[] = {SPT_CTECONFIG_T46,
				0, 2, 0, 0, 0, 0, 0};
static u8 end_config[] = {RESERVED_T255};

static const u8 *mxt224_config[] = {
	t7_config,
	t8_config,
	t9_config,
	t15_config,
	t18_config,
	t46_config,
	t48_config,
	end_config,
};
#endif
#else
/*
	Configuration for MXT224
*/
static u8 t7_config[] = {GEN_POWERCONFIG_T7,
				48,		/* IDLEACQINT */
				255,	/* ACTVACQINT */
				25		/* ACTV2IDLETO: 25 * 200ms = 5s */};
static u8 t8_config[] = {GEN_ACQUISITIONCONFIG_T8,
				10, 0, 5, 1, 0, 0, 9, 30};/*byte 3: 0*/
static u8 t9_config[] = {TOUCH_MULTITOUCHSCREEN_T9,
				131, 0, 0, 19, 11, 0, 32, MXT224_THRESHOLD, 2, 1,
				0,
				15,		/* MOVHYSTI */
				1, 11, MXT224_MAX_MT_FINGERS, 5, 40, 10, 31, 3,
				223, 1, 0, 0, 0, 0, 143, 55, 143, 90, 18};

static u8 t18_config[] = {SPT_COMCONFIG_T18,
				0, 1};
static u8 t20_config[] = {PROCI_GRIPFACESUPPRESSION_T20,
				7, 0, 0, 0, 0, 0, 0, 30, 20, 4, 15, 10};
static u8 t22_config[] = {PROCG_NOISESUPPRESSION_T22,
				143, 0, 0, 0, 0, 0, 0, 3, 30, 0, 0,  29, 34, 39,
				49, 58, 3};
static u8 t28_config[] = {SPT_CTECONFIG_T28,
				0, 0, 3, 16, 19, 60};
static u8 end_config[] = {RESERVED_T255};

static const u8 *mxt224_config[] = {
	t7_config,
	t8_config,
	t9_config,
	t18_config,
	t20_config,
	t22_config,
	t28_config,
	end_config,
};

/*
	Configuration for MXT224-E
*/

#if defined(CONFIG_TARGET_LOCALE_NAATT)
static u8 t7_config_e[] = {GEN_POWERCONFIG_T7,
				32, 255, 50};
static u8 t8_config_e[] = {GEN_ACQUISITIONCONFIG_T8,
				27, 0, 5, 1, 0, 0, 8, 8, 40, 55};

/* MXT224E_0V5_CONFIG */
/* NEXTTCHDI added */
static u8 t9_config_e[] = {TOUCH_MULTITOUCHSCREEN_T9,
				139, 0, 0, 19, 11, 0, 16, 35, 2, 1,
				10, 3, 1, 0, MXT224_MAX_MT_FINGERS, 5, 40, 10, 31, 3,
				223, 1, 10, 10, 10, 10, 143, 40, 143, 80,
				18, 15, 50, 50, 2};

static u8 t15_config_e[] = {TOUCH_KEYARRAY_T15,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static u8 t18_config_e[] = {SPT_COMCONFIG_T18,
				0, 0};

static u8 t23_config_e[] = {TOUCH_PROXIMITY_T23,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static u8 t25_config_e[] = {SPT_SELFTEST_T25,
				0, 0, 3000, 0, 0, 0, 0, 0};

static u8 t40_config_e[] = {PROCI_GRIPSUPPRESSION_T40,
				0, 0, 0, 0, 0};

static u8 t42_config_e[] = {PROCI_TOUCHSUPPRESSION_T42,
				0, 0, 0, 0, 0, 0, 0, 0};

static u8 t46_config_e[] = {SPT_CTECONFIG_T46,
				0, 3, 16, 28, 0, 0, 1, 0, 0};

static u8 t47_config_e[] = {PROCI_STYLUS_T47,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/*MXT224E_0V5_CONFIG */
static u8 t48_config_e[] = {PROCG_NOISESUPPRESSION_T48,
				1, 12, 112, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 6, 6, 0, 0, 100, 4, 64,
				10, 0, 20, 5, 0, 38, 0, 20, 0, 0,
				0, 0, 0, 0, 0, 55, 2, 5, 2, 0,
				5, 10, 10, 0, 0, 16, 17, 146, 60, 149,
				68, 25, 15, 3};

#else

static u8 t7_config_e[] = {GEN_POWERCONFIG_T7,
				48,		/* IDLEACQINT */
				255,	/* ACTVACQINT */
				25		/* ACTV2IDLETO: 25 * 200ms = 5s */};
static u8 t8_config_e[] = {GEN_ACQUISITIONCONFIG_T8,
				22, 0, 5, 1, 0, 0, 4, 35, 40, 55};
#if 1 /* MXT224E_0V5_CONFIG */
/* NEXTTCHDI added */
static u8 t9_config_e[] = {TOUCH_MULTITOUCHSCREEN_T9,
				131, 0, 0, 19, 11, 0, 32, 50, 2, 1,
				10,
				15,		/* MOVHYSTI */
				1, 11, MXT224_MAX_MT_FINGERS, 5, 40, 10, 31, 3,
				223, 1, 10, 10, 10, 10, 143, 40, 143, 80,
				18, 15, 50, 50, 0};
#else
static u8 t9_config_e[] = {TOUCH_MULTITOUCHSCREEN_T9,
				139, 0, 0, 19, 11, 0, 16, MXT224_THRESHOLD, 2, 1, 10, 3, 1,
				0, MXT224_MAX_MT_FINGERS, 10, 40, 10, 31, 3,
				223, 1, 10, 10, 10, 10, 143, 40, 143, 80, 18, 15, 50, 50};
#endif

static u8 t15_config_e[] = {TOUCH_KEYARRAY_T15,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static u8 t18_config_e[] = {SPT_COMCONFIG_T18,
				0, 0};

static u8 t23_config_e[] = {TOUCH_PROXIMITY_T23,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static u8 t25_config_e[] = {SPT_SELFTEST_T25,
				0, 0, 0, 0, 0, 0, 0, 0};

static u8 t40_config_e[] = {PROCI_GRIPSUPPRESSION_T40,
				0, 0, 0, 0, 0};

static u8 t42_config_e[] = {PROCI_TOUCHSUPPRESSION_T42,
				0, 0, 0, 0, 0, 0, 0, 0};

static u8 t46_config_e[] = {SPT_CTECONFIG_T46,
				0, 3, 16, 40, 0, 0, 1, 0, 0};

static u8 t47_config_e[] = {PROCI_STYLUS_T47,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

#if 1 /*MXT224E_0V5_CONFIG */
#ifdef CONFIG_TARGET_LOCALE_NA
static u8 t48_config_e_ta[] = {PROCG_NOISESUPPRESSION_T48,
				3, 132, 0x50, 0, 0, 0, 0, 0, 10, 15,
				0, 0, 0, 6, 6, 0, 0, 100, 4, 64,
				10, 0, 20, 5, 0, 38, 0, 20, 0, 0,
				0, 0, 0, 0, 0, 40, 2,
				10,		/* MOVHYSTI */
				1, 15,
				10, 5, 40, 240, 245, 10, 10, 148, 50, 143,
				80, 18, 10, 0};
static u8 t48_config_e[] = {PROCG_NOISESUPPRESSION_T48,
				3, 132, 0x40, 0, 0, 0, 0, 0, 10, 15,
				0, 0, 0, 6, 6, 0, 0, 100, 4, 64,
				10, 0, 20, 5, 0, 38, 0, 5, 0, 0,  
				0, 0, 0, 0, 32, 50, 2,
				10,
				1, 46,
				MXT224_MAX_MT_FINGERS, 5, 40, 10, 0, 10, 10, 143, 40, 143,
				80, 18, 15, 0};
#else
static u8 t48_config_e_ta[] = {PROCG_NOISESUPPRESSION_T48,
				3, 132, 0x52, 0, 0, 0, 0, 0, 10, 15,
				0, 0, 0, 6, 6, 0, 0, 64, 4, 64,
				10, 0, 20, 5, 0, 38, 0, 20, 0, 0,
				0, 0, 0, 0, 0, 40, 2,
				15,		/* MOVHYSTI */
				1, 46,
				10, 5, 40, 235, 235, 10, 10, 160, 50, 143,
				80, 18, 10, 0};

static u8 t48_config_e[] = {PROCG_NOISESUPPRESSION_T48,
				3, 132, 0x40, 0, 0, 0, 0, 0, 10, 15,
				0, 0, 0, 6, 6, 0, 0, 64, 4, 64,
				10, 0, 20, 5, 0, 38, 0, 5, 0, 0,  
				0, 0, 0, 0, 32, 50, 2,
				15,
				1, 11,
				MXT224_MAX_MT_FINGERS, 5, 40, 10, 10, 10, 10, 143, 40, 143,
				80, 18, 15, 0};

#endif
#else
/*static u8 t48_config_e[] = {PROCG_NOISESUPPRESSION_T48,
				1, 0, 40, 0, 0, 0, 0, 0, 0, 0, 0, 60, 31, 6,
				50, 64, 100};*/
#endif
#endif

static u8 end_config_e[] = {RESERVED_T255};

static const u8 *mxt224e_config[] = {
	t7_config_e,
	t8_config_e,
	t9_config_e,
	t15_config_e,
	t18_config_e,
	t23_config_e,
	t25_config_e,
	t40_config_e,
	t42_config_e,
	t46_config_e,
	t47_config_e,
	t48_config_e,
	end_config_e,
};

#endif


static struct mxt224_platform_data mxt224_data = {
	.max_finger_touches = MXT224_MAX_MT_FINGERS,
	.gpio_read_done = GPIO_TSP_INT,
#if defined(CONFIG_MACH_C1Q1_REV02) || defined(CONFIG_MACH_P6_REV02)
	.config = mxt224_config,
#else
	.config = mxt224_config,
	.config_e = mxt224e_config,
	.t48_ta_cfg = t48_config_e_ta,
#endif
	.min_x = 0,
#ifdef CONFIG_TOUCHSCREEN_MXT768E
	.max_x = 1023,
#else
	.max_x = 480,
#endif
	.min_y = 0,
#ifdef CONFIG_TOUCHSCREEN_MXT768E
	.max_y = 1023,
#else
	.max_y = 800,
#endif
	.min_z = 0,
	.max_z = 255,
	.min_w = 0,
	.max_w = 30,
	.power_on = mxt224_power_on,
	.power_off = mxt224_power_off,
	.register_cb = mxt224_register_callback,
	.read_ta_status = mxt224_read_ta_status,
};

/*
static struct qt602240_platform_data qt602240_platform_data = {
	.x_line		  = 19,
	.y_line		  = 11,
	.x_size		  = 800,
	.y_size		  = 480,
	.blen			  = 0x11,
	.threshold		  = 0x28,
	.voltage		  = 2800000,
	.orient		  = QT602240_DIAGONAL,
};
*/
#endif

#ifdef CONFIG_I2C_S3C2410
/* I2C0 */
static struct i2c_board_info i2c_devs0[] __initdata = {
	{ I2C_BOARD_INFO("24c128", 0x50), },	 /* Samsung S524AD0XD1 */
	{ I2C_BOARD_INFO("24c128", 0x52), },	 /* Samsung S524AD0XD1 */
};
#ifdef CONFIG_S3C_DEV_I2C1
static struct k3dh_platform_data k3dh_data = {
	.gpio_acc_int = S5PV310_GPX3(0),
};
/* I2C1 */
static struct i2c_board_info i2c_devs1[] __initdata = {
	{
		I2C_BOARD_INFO("k3g", 0x69),
		.irq = IRQ_EINT(1),
	},
	{
		I2C_BOARD_INFO("k3dh", 0x19),
		.platform_data	= &k3dh_data,
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
#if defined(CONFIG_TOUCHSCREEN_QT602240) || defined(CONFIG_TOUCHSCREEN_MXT768E)
	{
#ifdef CONFIG_TOUCHSCREEN_MXT768E
		I2C_BOARD_INFO(MXT224_DEV_NAME, 0x4c),
#else
		I2C_BOARD_INFO(MXT224_DEV_NAME, 0x4a),
#endif
		.platform_data  = &mxt224_data,
	},
#endif
};
#endif
#ifdef CONFIG_EPEN_WACOM_G5SP
static void p6_digitizer_init_hw(void);
static void p6_register_wacom_callbacks(struct wacom_g5_callbacks *cb);

static struct wacom_g5_platform_data p6_digitizer_platform_data = {
	.x_invert = 1,
	.y_invert = 0,
	.xy_switch = 1,
	.init_platform_hw = p6_digitizer_init_hw,
	/* .exit_platform_hw = p6_touch_exit_hw, */
	/* .suspend_platform_hw = p6_touch_suspend_hw, */
	/* .resume_platform_hw = p6_touch_resume_hw, */
	.register_cb = p6_register_wacom_callbacks,
};
#endif /* CONFIG_EPEN_WACOM_G5SP */

#ifdef CONFIG_S3C_DEV_I2C4
/* I2C4 */
static struct i2c_board_info i2c_devs4[] __initdata = {
#if defined(CONFIG_WIMAX_CMC) && defined(CONFIG_TARGET_LOCALE_NA)
	{
		I2C_BOARD_INFO("max8893_wmx", 0x3E),
		.platform_data = NULL,
	},
#endif /* CONFIG_WIMAX_CMC */
#ifdef CONFIG_EPEN_WACOM_G5SP
	{
		I2C_BOARD_INFO("wacom_g5sp_i2c", 0x56),
		.platform_data = &p6_digitizer_platform_data,
	},
#endif /* CONFIG_EPEN_WACOM_G5SP */
};
#if defined(CONFIG_WIMAX_CMC) && defined(CONFIG_TARGET_LOCALE_NA)
static struct i2c_gpio_platform_data wmxeeprom_i2c_gpio_data __initdata = {
	.sda_pin  = GPIO_CMC_SDA_18V,
	.scl_pin  = GPIO_CMC_SCL_18V,
	.udelay = 2,
};
static struct platform_device wmxeeprom_i2c_gpio_device = {
	.name	= "i2c-gpio",
	.id	= 18,
	.dev	= {
		.platform_data  = &wmxeeprom_i2c_gpio_data,
	},
};
static struct i2c_board_info wmxeeprom_i2c_devices[] __initdata = {
{
	I2C_BOARD_INFO("wmxeeprom", 0x50),
}
};

#endif /* CONFIG_WIMAX_CMC */
#endif

#ifdef CONFIG_EPEN_WACOM_G5SP
static void p6_digitizer_init_hw(void)
{
	int gpio;
	int ret;

	gpio = GPIO_PEN_SLP;
	ret = gpio_request(gpio, "PEN_SLP");
	s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(0x1));
	s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP);
	gpio_direction_output(gpio, 0);

	gpio = GPIO_PEN_PDCT;
	ret = gpio_request(gpio, "PEN_PDCT");
	s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(0x0));
	s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP);
	gpio_direction_input(gpio);

	gpio = GPIO_PEN_IRQ;
	ret = gpio_request(gpio, "PEN_IRQ");
	s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	gpio_direction_input(gpio);
	i2c_devs4[0].irq = gpio_to_irq(gpio);
	set_irq_type(i2c_devs4[0].irq, IRQ_TYPE_LEVEL_HIGH);
	s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(0xf));

/*	printk(KERN_DEBUG "[E-PEN]:%s:registered irq num = %d\n",
			__func__, i2c_devs4[0].irq);
*/
}

static void p6_register_wacom_callbacks(struct wacom_g5_callbacks *cb)
{
	wacom_callbacks = cb;
};

static int __init p6_digitizer_init(void)
{
	p6_digitizer_init_hw();
	printk(KERN_INFO "[E-PEN] : initialized.\n");
	return 0;
}
#endif /* CONFIG_EPEN_WACOM_G5SP */
#ifdef CONFIG_S3C_DEV_I2C5
/* I2C5 */
static struct i2c_board_info i2c_devs5[] __initdata = {
#ifdef CONFIG_MFD_MAX8998
	{
		I2C_BOARD_INFO("lp3974", 0x66),
		.platform_data	= &s5pv310_max8998_info,
	},
#endif
#ifdef CONFIG_REGULATOR_MAX8649
	{
		I2C_BOARD_INFO("max8952", 0x60),
		.platform_data	= &s5pv310_max8952_info,
	},
#endif
#ifdef CONFIG_MFD_MAX8997
	{
		I2C_BOARD_INFO("max8997", (0xcc >> 1)),
		.platform_data	= &s5pv310_max8997_info,
	},
#endif
};
#endif
#ifdef CONFIG_S3C_DEV_I2C6
/* I2C6 */
static struct i2c_board_info i2c_devs6[] __initdata = {
	{
		I2C_BOARD_INFO("mc1n2", 0x3a),		/* MC1N2 */
		.platform_data = &mc1n2_pdata,
	},
};
#endif
#ifdef CONFIG_S3C_DEV_I2C7
static struct akm8975_platform_data akm8975_pdata = {
	.gpio_data_ready_int = GPIO_MSENSE_INT,
};
/* I2C7 */
static struct i2c_board_info i2c_devs7[] __initdata = {
	{
		I2C_BOARD_INFO("ak8975", 0x0C),
		.platform_data = &akm8975_pdata,
	},
#ifdef CONFIG_VIDEO_TVOUT
	{
		I2C_BOARD_INFO("s5p_ddc", (0x74 >> 1)),
	},
#endif
};
#endif

#ifdef CONFIG_S3C_DEV_I2C8_EMUL
static struct i2c_gpio_platform_data gpio_i2c_data8 = {
	.sda_pin = GPIO_3_TOUCH_SDA,
	.scl_pin = GPIO_3_TOUCH_SCL,
};

struct platform_device s3c_device_i2c8 = {
	.name = "i2c-gpio",
	.id = 8,
	.dev.platform_data = &gpio_i2c_data8,
};

/* I2C8 */
static struct i2c_board_info i2c_devs8_emul[] __initdata = {
	{
		I2C_BOARD_INFO("melfas_touchkey", 0x20),
	},
};
#endif

#ifdef CONFIG_S3C_DEV_I2C9_EMUL
static struct i2c_gpio_platform_data gpio_i2c_data9 = {
	.sda_pin = GPIO_FUEL_SDA,
	.scl_pin = GPIO_FUEL_SCL,
};

struct platform_device s3c_device_i2c9 = {
	.name = "i2c-gpio",
	.id = 9,
	.dev.platform_data = &gpio_i2c_data9,
};

#ifdef CONFIG_FUELGAUGE_MAX17042

struct max17042_reg_data max17042_init_data[] = {
	{ MAX17042_REG_CGAIN,		0x00,	0x00 },
	{ MAX17042_REG_MISCCFG,		0x03,	0x00 },
	{ MAX17042_REG_LEARNCFG,	0x07,	0x00 },
	/* RCOMP: 0x0050 2011.02.29 from MAXIM */
	{ MAX17042_REG_RCOMP,		0x50,	0x00 },
};

struct max17042_reg_data max17042_alert_init_data[] = {
	/* SALRT Threshold setting to 2% => 1% wake lock */
	{ MAX17042_REG_SALRT_TH,	0x02,	0xFF },
	/* VALRT Threshold setting (disable) */
	{ MAX17042_REG_VALRT_TH,	0x00,	0xFF },
	/* TALRT Threshold setting (disable) */
	{ MAX17042_REG_TALRT_TH,	0x80,	0x7F },
};

bool max17042_is_low_batt(void)
{
	struct power_supply *psy = power_supply_get_by_name("battery");
	union power_supply_propval value;

	if (!psy) {
		pr_err("%s: fail to get battery ps\n", __func__);
		return -ENODEV;
	}

	if (!(psy->get_property(psy, POWER_SUPPLY_PROP_CAPACITY, &value)))
		if (value.intval > SEC_BATTERY_SOC_3_6)
			return false;

	return true;
}
EXPORT_SYMBOL(max17042_is_low_batt);

static int max17042_low_batt_cb(void)
{
	struct power_supply *psy = power_supply_get_by_name("battery");
	union power_supply_propval value;

	if (!psy) {
		pr_err("%s: fail to get battery ps\n", __func__);
		return -ENODEV;
	}

	value.intval = POWER_SUPPLY_CAPACITY_LEVEL_CRITICAL;
	return psy->set_property(psy, POWER_SUPPLY_PROP_CAPACITY_LEVEL, &value);
}

#ifdef RECAL_SOC_FOR_MAXIM
static bool max17042_need_soc_recal(void)
{
	pr_info("%s: HW(0x%x)\n", __func__, system_rev);

	if (system_rev >= NO_NEED_RECAL_SOC_HW_REV)
		return false;
	else
		return true;
}
#endif

static struct max17042_platform_data s5pv310_max17042_info = {
	.low_batt_cb = max17042_low_batt_cb,
	.init = max17042_init_data,
	.init_size = sizeof(max17042_init_data),
	.alert_init = max17042_alert_init_data,
	.alert_init_size = sizeof(max17042_alert_init_data),
	.alert_gpio = GPIO_FUEL_ALERT,
	.alert_irq = 0,
	.enable_current_sense = false,
	.enable_gauging_temperature = true,
#ifdef RECAL_SOC_FOR_MAXIM
	.need_soc_recal = max17042_need_soc_recal,
#endif
};
#endif

/* I2C9 */
static struct i2c_board_info i2c_devs9_emul[] __initdata = {
#ifdef CONFIG_FUELGAUGE_MAX17042
	{
		I2C_BOARD_INFO("max17042", 0x36),
		.platform_data	= &s5pv310_max17042_info,
		.irq = IRQ_EINT(19),
	},
#endif
#ifdef CONFIG_BATTERY_MAX17040
	{
		I2C_BOARD_INFO("max17040", 0x36),
	},
#endif
};
#endif

#ifdef CONFIG_S3C_DEV_I2C10_EMUL
static struct i2c_gpio_platform_data gpio_i2c_data10 = {
	.sda_pin = GPIO_USB_SDA,
	.scl_pin = GPIO_USB_SCL,
};

struct platform_device s3c_device_i2c10 = {
	.name = "i2c-gpio",
	.id = 10,
	.dev.platform_data = &gpio_i2c_data10,
};

/* I2C10 */
static struct fsa9480_platform_data fsa9480_info = {
};

static struct i2c_board_info i2c_devs10_emul[] __initdata = {
	{
		I2C_BOARD_INFO("fsa9480", 0x25),
		.platform_data	= &fsa9480_info,
	},
};
#endif

#ifdef CONFIG_S3C_DEV_I2C11_EMUL

static struct i2c_gpio_platform_data gpio_i2c_data11 = {
	.sda_pin = GPIO_PS_ALS_SDA,
	.scl_pin = GPIO_PS_ALS_SCL,
};

struct platform_device s3c_device_i2c11 = {
	.name = "i2c-gpio",
	.id = 11,
	.dev.platform_data = &gpio_i2c_data11,
};

/* I2C11 */
static int cm3663_ldo(bool on)
{
	struct regulator *regulator;

	ldo15_init_data.constraints.state_mem.enabled = on;
	ldo15_init_data.constraints.state_mem.disabled = !on;

	if (on) {
		regulator = regulator_get(NULL, "vled");
		if (IS_ERR(regulator))
			return 0;
		regulator_enable(regulator);
		regulator_put(regulator);
	} else {
		regulator = regulator_get(NULL, "vled");
		if (IS_ERR(regulator))
			return 0;
		if (regulator_is_enabled(regulator))
			regulator_force_disable(regulator);
		regulator_put(regulator);
	}

	return 0;
}

#ifdef CONFIG_USBHUB_USB3803
int usb3803_hw_config(void)
{
	int i;
	int usb_gpio[] = {GPIO_USB_RESET_N, GPIO_USB_BYPASS_N, GPIO_USB_CLOCK_EN};

	for (i = 0; i < 3; i++) {
		s3c_gpio_cfgpin(usb_gpio[i], S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(usb_gpio[i], S3C_GPIO_PULL_NONE);
		gpio_set_value(usb_gpio[i], S3C_GPIO_SETPIN_ZERO);
		s5p_gpio_set_drvstr(usb_gpio[i], S5P_GPIO_DRVSTR_LV1); /* need to check drvstr 1 or 2 */
	}
	return 0;
}

int usb3803_reset_n(int val)
{
	gpio_set_value(GPIO_USB_RESET_N, !!val);
}

int usb3803_bypass_n(int val)
{
	gpio_set_value(GPIO_USB_BYPASS_N, !!val);
}

int usb3803_clock_en(int val)
{
	gpio_set_value(GPIO_USB_CLOCK_EN, !!val);
}
#endif /* CONFIG_USBHUB_USB3803 */

static struct cm3663_platform_data cm3663_pdata = {
	.proximity_power = cm3663_ldo,
};

static struct i2c_board_info i2c_devs11_emul[] __initdata = {
	{
		I2C_BOARD_INFO("cm3663", 0x20),
		.irq = GPIO_PS_ALS_INT,
		.platform_data = &cm3663_pdata,
	},
};

#endif

#if defined(CONFIG_VIDEO_S5K6AAFX) || defined(CONFIG_VIDEO_S5K5BAFX) || defined(CONFIG_VIDEO_S5K5BBGX)
static struct i2c_gpio_platform_data i2c12_platdata = {
	.sda_pin		= VT_CAM_SDA_18V,
	.scl_pin		= VT_CAM_SCL_18V,
	.udelay			= 2,	/* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device s3c_device_i2c12 = {
	.name = "i2c-gpio",
	.id = 12,
	.dev.platform_data = &i2c12_platdata,
};

/* I2C12 */
static struct i2c_board_info i2c_devs12_emul[] __initdata = {
	/* need to work here */
};
#endif

#ifdef CONFIG_SND_SOC_MIC_A1026
static struct i2c_gpio_platform_data gpio_i2c_data13 = {
	.sda_pin = GPIO_2MIC_SDA,
	.scl_pin = GPIO_2MIC_SCL,
};

struct platform_device s3c_device_i2c13 = {
	.name = "i2c-gpio",
	.id = 13,
	.dev.platform_data = &gpio_i2c_data13,
};

/* I2C13 */
static struct i2c_board_info i2c_devs13_emul[] __initdata = {
	{
		I2C_BOARD_INFO("a1026", 0x3E),
	},
};
#endif /* #ifdef CONFIG_SND_SOC_MIC_A1026 */

#ifdef CONFIG_PN544
static unsigned int nfc_gpio_table[][4] = {
	{GPIO_NFC_IRQ, S3C_GPIO_INPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN},
	{GPIO_NFC_EN, S3C_GPIO_OUTPUT, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{GPIO_NFC_FIRM, S3C_GPIO_OUTPUT, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
 /*     {GPIO_NFC_SCL, S3C_GPIO_INPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE}, */
 /*	  {GPIO_NFC_SDA, S3C_GPIO_INPUT, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE}, */
};

void nfc_setup_gpio(void)
{
	/* s3c_config_gpio_alive_table(ARRAY_SIZE(nfc_gpio_table), nfc_gpio_table); */
	int array_size = ARRAY_SIZE(nfc_gpio_table);
	u32 i, gpio;
	for (i = 0; i < array_size; i++) {
		gpio = nfc_gpio_table[i][0];
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(nfc_gpio_table[i][1]));
		s3c_gpio_setpull(gpio, nfc_gpio_table[i][3]);
		if (nfc_gpio_table[i][2] != GPIO_LEVEL_NONE)
			gpio_set_value(gpio, nfc_gpio_table[i][2]);
	}

	/* s3c_gpio_cfgpin(GPIO_NFC_IRQ, EINT_MODE); */
	/* s3c_gpio_setpull(GPIO_NFC_IRQ, S3C_GPIO_PULL_DOWN); */

}

static struct i2c_gpio_platform_data i2c14_platdata = {
	.sda_pin		= GPIO_NFC_SDA,
	.scl_pin		= GPIO_NFC_SCL,
	.udelay			= 2,
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device s3c_device_i2c14 = {
	.name			= "i2c-gpio",
	.id			= 14,
	.dev.platform_data	= &i2c14_platdata,
};

static struct pn544_i2c_platform_data pn544_pdata = {
	.irq_gpio = GPIO_NFC_IRQ,
	.ven_gpio = GPIO_NFC_EN,
	.firm_gpio = GPIO_NFC_FIRM,
};

static struct i2c_board_info i2c_devs14[] __initdata = {
	{
		I2C_BOARD_INFO("pn544", 0x2b),
		.irq = IRQ_EINT(15),
		.platform_data = &pn544_pdata,
	},
};
#endif

#ifdef CONFIG_VIDEO_MHL_V1
static struct i2c_board_info i2c_devs15[] __initdata = {
	{
		I2C_BOARD_INFO("SII9234", 0x72>>1),
	},
	{
		I2C_BOARD_INFO("SII9234A", 0x7A>>1),
	},
	{
		I2C_BOARD_INFO("SII9234B", 0x92>>1),
	},
	{
		I2C_BOARD_INFO("SII9234C", 0xC8>>1),
	},
};
#endif

#ifdef CONFIG_VIDEO_MHL_V1
/* i2c-gpio emulation platform_data */
static struct i2c_gpio_platform_data i2c15_platdata = {
	.sda_pin		= GPIO_AP_SDA_18V,
	.scl_pin		= GPIO_AP_SCL_18V,
	.udelay			= 2,	/* 250 kHz*/
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device s3c_device_i2c15 = {
	.name			= "i2c-gpio",
	.id			= 15,
	.dev.platform_data	= &i2c15_platdata,
};
#endif

#ifdef CONFIG_FM_SI4709_MODULE
static struct i2c_gpio_platform_data i2c16_platdata = {
	.sda_pin		= GPIO_FM_SDA_28V,
	.scl_pin		= GPIO_FM_SCL_28V,
	.udelay			= 2,	/* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device s3c_device_i2c16 = {
	.name					= "i2c-gpio",
	.id						= 16,
	.dev.platform_data	= &i2c16_platdata,
};

static struct i2c_board_info i2c_devs16[] __initdata = {
	{
		I2C_BOARD_INFO("Si4709", (0x20 >> 1)),
	},
};
#endif

#ifdef CONFIG_S3C_DEV_I2C17_EMUL
/* I2C17_EMUL */
static struct i2c_gpio_platform_data i2c17_platdata = {
	.sda_pin = GPIO_USB_I2C_SDA,
	.scl_pin = GPIO_USB_I2C_SCL,
};

struct platform_device s3c_device_i2c17 = {
	.name = "i2c-gpio",
	.id = 17,
	.dev.platform_data = &i2c17_platdata,
};
#endif /* CONFIG_S3C_DEV_I2C17_EMUL */

#ifdef CONFIG_USBHUB_USB3803
struct usb3803_platform_data usb3803_pdata = {
	.init_needed    =  1,
	.es_ver         = 1,
	.inital_mode    = USB_3803_MODE_STANDBY,
	.hw_config      = usb3803_hw_config,
	.reset_n        = usb3803_reset_n,
	.bypass_n       = usb3803_bypass_n,
	.clock_en       = usb3803_clock_en,
};

static struct i2c_board_info i2c_devs17_emul[] __initdata = {
	{
		I2C_BOARD_INFO(USB3803_I2C_NAME, 0x08),
		.platform_data  = &usb3803_pdata,
	},
};
#endif /* CONFIG_USBHUB_USB3803 */

#ifdef CONFIG_VIDEO_M5MO_USE_SWI2C
static struct i2c_gpio_platform_data i2c25_platdata = {
	.sda_pin		= S5PV310_GPD1(0),
	.scl_pin		= S5PV310_GPD1(1),
	.udelay			= 5,	/* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device s3c_device_i2c25 = {
	.name = "i2c-gpio",
	.id = 25,
	.dev.platform_data = &i2c25_platdata,
};

/* I2C25 */
static struct i2c_board_info i2c_devs25_emul[] __initdata = {
	/* need to work here */
};
#endif

#ifdef	CONFIG_ISDBT_FC8100
static struct	i2c_board_info i2c_devs17[]	__initdata = {
	{
		I2C_BOARD_INFO("isdbti2c", 0x77),
	},
};

static struct i2c_gpio_platform_data i2c17_platdata = {
	.sda_pin		= GPIO_ISDBT_SDA_28V,
	.scl_pin		= GPIO_ISDBT_SCL_28V,
	.udelay			= 3,	/*  kHz	*/
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only = 0,
};

static struct platform_device s3c_device_i2c17 = {
	.name			= "i2c-gpio",
	.id				= 17,
	.dev.platform_data	= &i2c17_platdata,
};
#endif

#endif

#ifdef CONFIG_S3C_DEV_HSMMC
static struct s3c_sdhci_platdata smdkc210_hsmmc0_pdata __initdata = {
	.cd_type		= S3C_SDHCI_CD_PERMANENT,
#if defined(CONFIG_S5PV310_SD_CH0_8BIT)
	.max_width		= 8,
	.host_caps		= MMC_CAP_8_BIT_DATA,
#endif
};
#endif
#ifdef CONFIG_S3C_DEV_HSMMC1
static struct s3c_sdhci_platdata smdkc210_hsmmc1_pdata __initdata = {
	.cd_type		= S3C_SDHCI_CD_GPIO,
};
#endif
#ifdef CONFIG_S3C_DEV_HSMMC2
static struct s3c_sdhci_platdata smdkc210_hsmmc2_pdata __initdata = {
	.cd_type		= S3C_SDHCI_CD_GPIO,
#ifndef CONFIG_MACH_C1_NA_SPR_REV05
	.ext_cd_gpio		= S5PV310_GPX3(4),
#endif
	.ext_cd_gpio_invert	= true,
#if defined(CONFIG_S5PV310_SD_CH2_8BIT)
	.max_width		= 4,
#endif
};
#endif
#ifdef CONFIG_S3C_DEV_HSMMC3
static struct s3c_sdhci_platdata smdkc210_hsmmc3_pdata __initdata = {
	.cd_type		= S3C_SDHCI_CD_PERMANENT,
};
#endif
#ifdef CONFIG_S5P_DEV_MSHC
static struct s3c_mshci_platdata smdkc210_mshc_pdata __initdata = {
	.cd_type		= S3C_MSHCI_CD_PERMANENT,
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

#ifdef CONFIG_FB_S3C_LD9040
#ifndef LCD_ON_FROM_BOOTLOADER
static int lcd_cfg_gpio(void)
{
	int i, f3_end = 4;

	for (i = 0; i < 8; i++) {
		/* set GPF0,1,2[0:7] for RGB Interface and Data lines (32bit) */
		s3c_gpio_cfgpin(S5PV310_GPF0(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PV310_GPF0(i), S3C_GPIO_PULL_NONE);

	}
	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PV310_GPF1(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PV310_GPF1(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PV310_GPF2(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PV310_GPF2(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < f3_end; i++) {
		s3c_gpio_cfgpin(S5PV310_GPF3(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PV310_GPF3(i), S3C_GPIO_PULL_NONE);
	}


#if MAX_DRVSTR
	/* drive strength to max[4X] */
	writel(0xffffffff, S5P_VA_GPIO + 0x18c);
	writel(0xffffffff, S5P_VA_GPIO + 0x1ac);
	writel(0xffffffff, S5P_VA_GPIO + 0x1cc);
	writel(readl(S5P_VA_GPIO + 0x1ec) | 0xffffff, S5P_VA_GPIO + 0x1ec);
#else
	/* drive strength to 2X */
	writel(0xaaaaaaaa, S5P_VA_GPIO + 0x18c);
	writel(0xaaaaaaaa, S5P_VA_GPIO + 0x1ac);
	writel(0xaaaaaaaa, S5P_VA_GPIO + 0x1cc);
	writel(readl(S5P_VA_GPIO + 0x1ec) | 0xaaaaaa, S5P_VA_GPIO + 0x1ec);
#endif
	/* MLCD_RST */
	s3c_gpio_cfgpin(S5PV310_GPY4(5), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV310_GPY4(5), S3C_GPIO_PULL_NONE);

	/* LCD_nCS */
	s3c_gpio_cfgpin(S5PV310_GPY4(3), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV310_GPY4(3), S3C_GPIO_PULL_NONE);
	/* LCD_SCLK */
	s3c_gpio_cfgpin(S5PV310_GPY3(1), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV310_GPY3(1), S3C_GPIO_PULL_NONE);
	/* LCD_SDI */
	s3c_gpio_cfgpin(S5PV310_GPY3(3), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV310_GPY3(3), S3C_GPIO_PULL_NONE);

	return 0;
}
#endif

static int lcd_power_on(struct lcd_device *ld, int enable)
{
	struct regulator *regulator;

	if (ld == NULL) {
		printk(KERN_ERR "lcd device object is NULL.\n");
		return 0;
	}

	if (enable) {
		regulator = regulator_get(NULL, "vlcd_3.0v");
		if (IS_ERR(regulator))
			return 0;
		regulator_enable(regulator);
		regulator_put(regulator);
		/* printk("lcd_power_on() is called : ON \n"); */
	} else {
		regulator = regulator_get(NULL, "vlcd_3.0v");
		if (IS_ERR(regulator))
			return 0;
		if (regulator_is_enabled(regulator)) {
			regulator_force_disable(regulator);
			/* printk("regulator_is_enabled() return SUCCESS\n"); */
		}
		regulator_put(regulator);
		/* printk("lcd_power_on() is called : OFF\n"); */
	}
	return 1;
}

static int reset_lcd(struct lcd_device *ld)
{
	int reset_gpio = -1;
	int err;

	reset_gpio = S5PV310_GPY4(5);

	err = gpio_request(reset_gpio, "MLCD_RST");
	if (err) {
		printk(KERN_ERR "failed to request MLCD_RST for "
				"lcd reset control\n");
		return err;
	}

	mdelay(10);
	gpio_direction_output(reset_gpio, 0);
	mdelay(10);
	gpio_direction_output(reset_gpio, 1);

	gpio_free(reset_gpio);

	return 1;
}

static int lcd_gpio_cfg_earlysuspend(struct lcd_device *ld)
{
	int reset_gpio = -1;
	int err;

	reset_gpio = S5PV310_GPY4(5);

	err = gpio_request(reset_gpio, "MLCD_RST");
	if (err) {
		printk(KERN_ERR "failed to request MLCD_RST for "
				"lcd reset control\n");
		return err;
	}

	mdelay(10);
	gpio_direction_output(reset_gpio, 0);

	gpio_free(reset_gpio);

#if 0
	/*
		Put all LCD GPIO pin into "INPUT-PULLDOWN" State to reduce sleep mode current
	*/

	/* MLCD_RST */
	gpio_direction_input(S5PV310_GPY4(5));
	s3c_gpio_setpull(S5PV310_GPY4(5), S3C_GPIO_PULL_DOWN);

	/* LCD_nCS */
	gpio_direction_input(S5PV310_GPY4(3));
	s3c_gpio_setpull(S5PV310_GPY4(3), S3C_GPIO_PULL_DOWN);

	/* LCD_SCLK */
	gpio_direction_input(S5PV310_GPY3(1));
	s3c_gpio_setpull(S5PV310_GPY3(1), S3C_GPIO_PULL_DOWN);

	/* LCD_SDI */
	gpio_direction_input(S5PV310_GPY3(3));
	s3c_gpio_setpull(S5PV310_GPY3(3), S3C_GPIO_PULL_DOWN);

#endif

	return 0;
}

static int lcd_gpio_cfg_lateresume(struct lcd_device *ld)
{
	/* MLCD_RST */
	s3c_gpio_cfgpin(S5PV310_GPY4(5), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV310_GPY4(5), S3C_GPIO_PULL_NONE);

	/* LCD_nCS */
	s3c_gpio_cfgpin(S5PV310_GPY4(3), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV310_GPY4(3), S3C_GPIO_PULL_NONE);
	/* LCD_SCLK */
	s3c_gpio_cfgpin(S5PV310_GPY3(1), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV310_GPY3(1), S3C_GPIO_PULL_NONE);
	/* LCD_SDI */
	s3c_gpio_cfgpin(S5PV310_GPY3(3), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV310_GPY3(3), S3C_GPIO_PULL_NONE);

	return 0;
}

static struct s3cfb_lcd ld9040_info = {
	.width = 480,
	.height = 800,
	.p_width = 56,
	.p_height = 93,
	.bpp = 24,

	.freq = 60,
	.timing = {
		.h_fp = 16,
		.h_bp = 14,
		.h_sw = 2,
		.v_fp = 10,
		.v_fpe = 1,
		.v_bp = 4,
		.v_bpe = 1,
		.v_sw = 2,
	},
	.polarity = {
		.rise_vclk = 1,
		.inv_hsync = 1,
		.inv_vsync = 1,
		.inv_vden = 1,
	},
};

static struct lcd_platform_data ld9040_platform_data = {
	.reset			= reset_lcd,
	.power_on		= lcd_power_on,
	.gpio_cfg_earlysuspend	= lcd_gpio_cfg_earlysuspend,
	.gpio_cfg_lateresume	= lcd_gpio_cfg_lateresume,

	/* it indicates whether lcd panel is enabled from u-boot. */
	.lcd_enabled		= 0,
/*
	.reset_delay		= 20,
	.power_on_delay		= 300,
	.power_off_delay	= 120,
*/
	.reset_delay		= 20,	/* 20ms */
	.power_on_delay		= 50,	/* 50ms */
	.power_off_delay	= 300,	/* 300ms */
	.sleep_in_delay		= 160,
	.pdata			= &c1_panel_data,
};

#define LCD_BUS_NUM	3
#define DISPLAY_CS	S5PV310_GPY4(3)
static struct spi_board_info spi_board_info[] __initdata = {
	{
		.max_speed_hz	= 1200000,
		.bus_num	= LCD_BUS_NUM,
		.chip_select	= 0,
		.mode		= SPI_MODE_3,
		.controller_data = (void *)DISPLAY_CS,
	},
};

#define DISPLAY_CLK	S5PV310_GPY3(1)
#define DISPLAY_SI	S5PV310_GPY3(3)
static struct spi_gpio_platform_data lcd_spi_gpio_data = {
	.sck			= DISPLAY_CLK,
	.mosi			= DISPLAY_SI,
	.miso			= SPI_GPIO_NO_MISO,
	.num_chipselect		= 1,
};

static struct platform_device ld9040_spi_gpio = {
	.name			= "spi_gpio",
	.id			= LCD_BUS_NUM,
	.dev			= {
		.parent		= &s3c_device_fb.dev,
		.platform_data	= &lcd_spi_gpio_data,
	},
};

static struct s3c_platform_fb fb_platform_data __initdata = {
	.hw_ver			= 0x70,
	.clk_name		= "fimd",
	.nr_wins		= 5,
#ifdef CONFIG_FB_S3C_DEFAULT_WINDOW
	.default_win		= CONFIG_FB_S3C_DEFAULT_WINDOW,
#else
	.default_win		= 0,
#endif
	.swap			= FB_SWAP_HWORD | FB_SWAP_WORD,
	.lcd = &ld9040_info,
};

static void __init ld9040_fb_init(void)
{
	strcpy(spi_board_info[0].modalias, "ld9040");
	spi_board_info[0].platform_data =
		(void *)&ld9040_platform_data;

	spi_register_board_info(spi_board_info,
		ARRAY_SIZE(spi_board_info));

#ifndef LCD_ON_FROM_BOOTLOADER
	lcd_cfg_gpio();
#endif
	s3cfb_set_platdata(&fb_platform_data);
}
#endif

#ifdef CONFIG_FB_S3C_MIPI_LCD
#ifdef CONFIG_FB_S3C_S6D6AA0
/* for Geminus based on MIPI-DSI interface */
static struct s3cfb_lcd s6d6aa0 = {
	.name = "s6d6aa0",
	.width = 800,
	.height = 1280,
	.p_width = 64,
	.p_height = 106,
	.bpp = 24,

	.freq = 60,

	/* minumun value is 0 except for wr_act time. */
	.cpu_timing = {
		.cs_setup = 0,
		.wr_setup = 0,
		.wr_act = 1,
		.wr_hold = 0,
	},

	.timing = {
		.h_fp = 10,
		.h_bp = 2,
		.h_sw = 8,
		.v_fp = 10,
		.v_fpe = 2,
		.v_bp = 2,
		.v_bpe = 1,
		.v_sw = 8,
		.cmd_allow_len = 2,
	},
	.polarity = {
		.rise_vclk = 0,
		.inv_hsync = 0,
		.inv_vsync = 0,
		.inv_vden = 0,
	},
};
#endif
#ifdef CONFIG_FB_S3C_S6E8AA0
/* for Geminus based on MIPI-DSI interface */
static struct s3cfb_lcd s6e8aa0 = {
       .name = "s6e8aa0",
       .width = 800,
       .height = 1280,
       .p_width = 64,
       .p_height = 106,
       .bpp = 24,

       .freq = 55,

       /* minumun value is 0 except for wr_act time. */
       .cpu_timing = {
		.cs_setup = 0,
		.wr_setup = 0,
		.wr_act = 1,
		.wr_hold = 0,
       },

       .timing = {
		.h_fp = 10,
		.h_bp = 10,
		.h_sw = 10,
		.v_fp = 13,
		.v_fpe = 1,
		.v_bp = 1,
		.v_bpe = 1,
		.v_sw = 2,
		.cmd_allow_len = 2,
       },

       .polarity = {
		.rise_vclk = 0,
		.inv_hsync = 0,
		.inv_vsync = 0,
		.inv_vden = 0,
       },
};
#endif
static const char *aquila_mipi_supply_names[] = {
	"VMIPI_1.8V", "VMIPI_1.1V",
};

static struct s3c_platform_fb fb_platform_data __initdata = {
	.hw_ver		= 0x70,
	.clk_name	= "fimd",
	.nr_wins	= 5,
#ifdef CONFIG_FB_S3C_DEFAULT_WINDOW
	.default_win	= CONFIG_FB_S3C_DEFAULT_WINDOW,
#else
	.default_win	= 0,
#endif
	.swap		= FB_SWAP_HWORD | FB_SWAP_WORD,
#ifdef CONFIG_FB_S3C_S6D6AA0
	.lcd		= &s6d6aa0,
#endif
#ifdef CONFIG_FB_S3C_S6E8AA0
	.lcd		= &s6e8aa0
#endif
};
#define AQUILA_MIPI_NUM_SUPPLIES	ARRAY_SIZE(aquila_mipi_supply_names)
static struct regulator_bulk_data aquila_mipi_supply[AQUILA_MIPI_NUM_SUPPLIES];
static void universal_mipi_power(struct device *dev, int en)
{
	int i, ret = 0;

	/*
	 * Aquila MIPI regulators
	 * VMIPI_1.8V, VMIPI_1.1V
	 */
	if (en) {
		for (i = 0; i < ARRAY_SIZE(aquila_mipi_supply); i++) {
			aquila_mipi_supply[i].supply =
				aquila_mipi_supply_names[i];
		}

		ret = regulator_bulk_get(dev, ARRAY_SIZE(aquila_mipi_supply),
				aquila_mipi_supply);
		if (ret != 0)
			printk(KERN_ERR "failed to get MIPI regulators\n");

		regulator_bulk_enable(ARRAY_SIZE(aquila_mipi_supply),
				aquila_mipi_supply);

		if (ret != 0) {
			printk(KERN_ERR "failed to enable supplies\n");
			regulator_bulk_free(ARRAY_SIZE(aquila_mipi_supply),
					aquila_mipi_supply);
		}
		printk(KERN_INFO "MIPI regulator on\n");
	} else {
		regulator_bulk_disable(ARRAY_SIZE(aquila_mipi_supply),
				aquila_mipi_supply);
		regulator_bulk_free(ARRAY_SIZE(aquila_mipi_supply),
				aquila_mipi_supply);
		printk(KERN_INFO "MIPI regulator off\n");
	}

}

#ifdef CONFIG_FB_S3C_S6D6AA0
static int reset_lcd(void)
{

	int reset_gpio = -1;

	reset_gpio = S5PV310_GPY4(7);

	gpio_request(reset_gpio, "MLCD_RST");

	mdelay(10);
	gpio_direction_output(reset_gpio, 0);
	mdelay(10);
	gpio_direction_output(reset_gpio, 1);

	gpio_free(reset_gpio);

	reset_gpio = S5PV310_GPY4(5);

	gpio_request(reset_gpio, "MLCD_RST");

	mdelay(10);
	gpio_direction_output(reset_gpio, 0);
	mdelay(10);
	gpio_direction_output(reset_gpio, 1);

	gpio_free(reset_gpio);

	/* Turn on LCD */
	reset_gpio = S5PV310_GPE1(4);

	gpio_request(reset_gpio, "MLCD_RST");

	gpio_direction_output(reset_gpio, 1);

	gpio_free(reset_gpio);
	return 1;
}
#endif
#ifdef CONFIG_FB_S3C_S6E8AA0
static int reset_lcd(void)
{
       int err;

       /* Set GPY4[7] OUTPUT HIGH */
       err = gpio_request(S5PV310_GPY4(7), "MLCD_RST");
       if (err) {
		printk(KERN_ERR "failed to request GPY4(7) for "
				"lcd reset control\n");
		return;
       }

       gpio_direction_output(S5PV310_GPY4(7), 1);
       msleep(10);
       gpio_set_value(S5PV310_GPY4(7), 0);
       msleep(10);
       gpio_set_value(S5PV310_GPY4(7), 1);
       msleep(10);
       gpio_free(S5PV310_GPY4(7));

       /* Set GPY4[5] OUTPUT HIGH */
       err = gpio_request(S5PV310_GPY4(5), "MLCD_RST");
       if (err) {
		printk(KERN_ERR "failed to request GPY4(5) for "
				"lcd reset control\n");
		return;
       }

       gpio_direction_output(S5PV310_GPY4(5), 1);
       msleep(10);
       gpio_set_value(S5PV310_GPY4(5), 0);
       msleep(10);
       gpio_set_value(S5PV310_GPY4(5), 1);
       msleep(10);
       gpio_free(S5PV310_GPY4(5));
}
#endif
static void lcd_cfg_gpio(void)
{
	int i, f3_end = 4;

	for (i = 0; i < 8; i++) {
		/* set GPF0,1,2[0:7] for RGB Interface and Data lines (32bit) */
		s3c_gpio_cfgpin(S5PV310_GPF0(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PV310_GPF0(i), S3C_GPIO_PULL_NONE);

	}
	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PV310_GPF1(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PV310_GPF1(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PV310_GPF2(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PV310_GPF2(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < f3_end; i++) {
		s3c_gpio_cfgpin(S5PV310_GPF3(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PV310_GPF3(i), S3C_GPIO_PULL_NONE);
	}

	/* drive strength to max */
	writel(0xffffffff, S5P_VA_GPIO + 0x18c);
	writel(0xffffffff, S5P_VA_GPIO + 0x1ac);
	writel(0xffffffff, S5P_VA_GPIO + 0x1cc);
	writel(readl(S5P_VA_GPIO + 0x1ec) | 0xffffff,
			S5P_VA_GPIO + 0x1ec);

	/* MLCD_RST */
	s3c_gpio_cfgpin(S5PV310_GPY4(5), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV310_GPY4(5), S3C_GPIO_PULL_NONE);

	/* LCD_nCS */
	s3c_gpio_cfgpin(S5PV310_GPY4(3), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV310_GPY4(3), S3C_GPIO_PULL_NONE);
	/* LCD_SCLK */
	s3c_gpio_cfgpin(S5PV310_GPY3(1), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV310_GPY3(1), S3C_GPIO_PULL_NONE);
	/* LCD_SDI */
	s3c_gpio_cfgpin(S5PV310_GPY3(3), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV310_GPY3(3), S3C_GPIO_PULL_NONE);

	return 0;
}

static int lcd_power_on(struct lcd_device *ld, int enable)
{
	struct regulator *regulator;

	if (ld == NULL) {
		printk(KERN_ERR "lcd device object is NULL.\n");
		return 0;
	}

	if (enable) {
		regulator = regulator_get(NULL, "vlcd_3.0v");
		if (IS_ERR(regulator))
			return 0;
		regulator_enable(regulator);
		regulator_put(regulator);
	} else {
		regulator = regulator_get(NULL, "vlcd_3.0v");
		if (IS_ERR(regulator))
			return 0;
		if (regulator_is_enabled(regulator))
			regulator_force_disable(regulator);
		regulator_put(regulator);
	}
	return 1;
}

extern struct platform_device s5p_device_dsim;

static void __init mipi_fb_init(void)
{
	struct s5p_platform_dsim *dsim_pd = NULL;
	struct mipi_ddi_platform_data *mipi_ddi_pd = NULL;
	struct dsim_lcd_config *dsim_lcd_info = NULL;
	int gpio;

	/* set platform data */

	/* gpio pad configuration for rgb and spi interface. */
	lcd_cfg_gpio();
	/*
	 * register lcd panel data.
	 */

	printk("fb_platform_data.hw_ver = 0x%x\n", fb_platform_data.hw_ver);
	/* geminus rev0.1 uses MIPI-DSI based ams397 lcd panel driver */

	fb_platform_data.mipi_is_enabled = 1;
	fb_platform_data.interface_mode = FIMD_CPU_INTERFACE;

	dsim_pd = (struct s5p_platform_dsim *)
		s5p_device_dsim.dev.platform_data;

#ifdef CONFIG_FB_S3C_S6D6AA0
	strcpy(dsim_pd->lcd_panel_name, "s6d6aa0");
#endif
#ifdef CONFIG_FB_S3C_S6E8AA0
	strcpy(dsim_pd->lcd_panel_name, "s6e8aa0");
#endif

	/* GPIO for TE Interrupt. */
/*	dsim_pd->te_irq = gpio_to_irq(S5PC11X_GPF0(1)); */

	/* geminus rev0.1 is based on evt1. */
#ifdef CONFIG_ARIES_VER_B0
	dsim_pd->platform_rev = 0;
#else
	dsim_pd->platform_rev = 1;
#endif

	dsim_lcd_info = dsim_pd->dsim_lcd_info;
#ifdef CONFIG_FB_S3C_S6D6AA0
	dsim_lcd_info->lcd_panel_info = (void *)&s6d6aa0;
#endif

#ifdef CONFIG_FB_S3C_S6E8AA0
	dsim_lcd_info->lcd_panel_info = (void *)&s6e8aa0;
#endif

	mipi_ddi_pd = (struct mipi_ddi_platform_data *)
		dsim_lcd_info->mipi_ddi_pd;
	mipi_ddi_pd->lcd_reset = reset_lcd;
	mipi_ddi_pd->lcd_power_on = lcd_power_on;

	platform_device_register(&s5p_device_dsim);

	s3cfb_set_platdata(&fb_platform_data);

	printk(KERN_INFO "platform data of %s lcd panel has been registered.\n",
		/* ((struct s3cfb_lcd *) fb_platform_data.lcd_data)->name); */
			dsim_pd->lcd_panel_name);
}
#endif
#ifdef CONFIG_ANDROID_PMEM
static struct android_pmem_platform_data pmem_pdata = {
	.name = "pmem",
	.no_allocator = 1,
#ifdef CONFIG_TARGET_LOCALE_KOR
	.cached = 1,
#else
	.cached = 0,
#endif

	.start = 0, /* will be set during proving pmem driver. */
	.size = 0 /* will be set during proving pmem driver. */
};

static struct android_pmem_platform_data pmem_gpu1_pdata = {
	.name = "pmem_gpu1",
	.no_allocator = 1,
	.cached = 0,
	/* .buffered = 1, */
	.start = 0,
	.size = 0,
};

static struct android_pmem_platform_data pmem_adsp_pdata = {
	.name = "pmem_adsp",
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

static struct platform_device pmem_adsp_device = {
	.name = "android_pmem",
	.id = 2,
	.dev = { .platform_data = &pmem_adsp_pdata },
};

static void __init android_pmem_set_platdata(void)
{
#if defined(CONFIG_S5P_MEM_CMA)
	pmem_pdata.size = CONFIG_ANDROID_PMEM_MEMSIZE_PMEM * SZ_1K;
	pmem_gpu1_pdata.size = CONFIG_ANDROID_PMEM_MEMSIZE_PMEM_GPU1 * SZ_1K;
	pmem_adsp_pdata.size = CONFIG_ANDROID_PMEM_MEMSIZE_PMEM_ADSP * SZ_1K;
#elif defined(CONFIG_S5P_MEM_BOOTMEM)
	pmem_pdata.start = (u32)s5p_get_media_memory_bank(S5P_MDEV_PMEM, 0);
	pmem_pdata.size = (u32)s5p_get_media_memsize_bank(S5P_MDEV_PMEM, 0);

	pmem_gpu1_pdata.start = (u32)s5p_get_media_memory_bank(S5P_MDEV_PMEM_GPU1, 0);
	pmem_gpu1_pdata.size = (u32)s5p_get_media_memsize_bank(S5P_MDEV_PMEM_GPU1, 0);

	pmem_adsp_pdata.start = (u32)s5p_get_media_memory_bank(S5P_MDEV_PMEM_ADSP, 0);
	pmem_adsp_pdata.size = (u32)s5p_get_media_memsize_bank(S5P_MDEV_PMEM_ADSP, 0);
#endif
}
#endif

#ifdef CONFIG_BATTERY_S3C
struct platform_device sec_fake_battery = {
	.name	= "sec-fake-battery",
	.id	= -1,
};
#endif

#ifdef CONFIG_BATTERY_SEC

#ifdef CONFIG_TARGET_LOCALE_KOR
static struct sec_bat_adc_table_data temper_table[] =  {
	{  264,	 500 },
	{  275,	 490 },
	{  286,	 480 },
	{  293,	 480 },
	{  299,	 470 },
	{  306,	 460 },
	{  324,	 450 },
	{  341,	 450 },
	{  354,	 440 },
	{  368,	 430 },
	{  381,	 420 },
	{  396,	 420 },
	{  411,	 410 },
	{  427,	 400 },
	{  442,	 390 },
	{  457,	 390 },
	{  472,	 380 },
	{  487,	 370 },
	{  503,	 370 },
	{  518,	 360 },
	{  533,	 350 },
	{  554,	 340 },
	{  574,	 330 },
	{  595,	 330 },
	{  615,	 320 },
	{  636,	 310 },
	{  656,	 310 },
	{  677,	 300 },
	{  697,	 290 },
	{  718,	 280 },
	{  738,	 270 },
	{  761,	 270 },
	{  784,	 260 },
	{  806,	 250 },
	{  829,	 240 },
	{  852,	 230 },
	{  875,	 220 },
	{  898,	 210 },
	{  920,	 200 },
	{  943,	 190 },
	{  966,	 180 },
	{  990,	 170 },
	{ 1013,	 160 },
	{ 1037,	 150 },
	{ 1060,	 140 },
	{ 1084,	 130 },
	{ 1108,	 120 },
	{ 1131,	 110 },
	{ 1155,	 100 },
	{ 1178,	  90 },
	{ 1202,	  80 },
	{ 1226,	  70 },
	{ 1251,	  60 },
	{ 1275,	  50 },
	{ 1299,	  40 },
	{ 1324,	  30 },
	{ 1348,	  20 },
	{ 1372,	  10 },
	{ 1396,	   0 },
	{ 1421,	 -10 },
	{ 1445,	 -20 },
	{ 1468,	 -30 },
	{ 1491,	 -40 },
	{ 1513,	 -50 },
	{ 1536,	 -60 },
	{ 1559,	 -70 },
	{ 1577,	 -80 },
	{ 1596,	 -90 },
	{ 1614,	 -100 },
	{ 1619,	 -110 },
	{ 1632,	 -120 },
	{ 1658,	 -130 },
	{ 1667,	 -140 },
};
#elif defined(CONFIG_TARGET_LOCALE_NTT)
/* temperature table for ADC 6 */
static struct sec_bat_adc_table_data temper_table[] =  {
	{  273,	 670 },
	{  289,	 660 },
	{  304,	 650 },
	{  314,	 640 },
	{  325,	 630 },
	{  337,	 620 },
	{  347,	 610 },
	{  361,	 600 },
	{  376,	 590 },
	{  391,	 580 },
	{  406,	 570 },
	{  417,	 560 },
	{  431,	 550 },
	{  447,	 540 },
	{  474,	 530 },
	{  491,	 520 },
	{  499,	 510 },
	{  511,	 500 },
	{  519,	 490 },
	{  547,	 480 },
	{  568,	 470 },
	{  585,	 460 },
	{  597,	 450 },
	{  614,	 440 },
	{  629,	 430 },
	{  647,	 420 },
	{  672,	 410 },
	{  690,	 400 },
	{  720,	 390 },
	{  735,	 380 },
	{  755,	 370 },
	{  775,	 360 },
	{  795,	 350 },
	{  818,	 340 },
	{  841,	 330 },
	{  864,	 320 },
	{  887,	 310 },
	{  909,	 300 },
	{  932,	 290 },
	{  954,	 280 },
	{  976,	 270 },
	{  999,	 260 },
	{ 1021,	 250 },
	{ 1051,	 240 },
	{ 1077,	 230 },
	{ 1103,	 220 },
	{ 1129,	 210 },
	{ 1155,	 200 },
	{ 1177,	 190 },
	{ 1199,	 180 },
	{ 1220,	 170 },
	{ 1242,	 160 },
	{ 1263,	 150 },
	{ 1284,	 140 },
	{ 1306,	 130 },
	{ 1326,	 120 },
	{ 1349,	 110 },
	{ 1369,	 100 },
	{ 1390,	  90 },
	{ 1411,	  80 },
	{ 1433,	  70 },
	{ 1454,	  60 },
	{ 1474,	  50 },
	{ 1486,	  40 },
	{ 1499,	  30 },
	{ 1512,	  20 },
	{ 1531,	  10 },
	{ 1548,	   0 },
	{ 1570,	 -10 },
	{ 1597,	 -20 },
	{ 1624,	 -30 },
	{ 1633,	 -40 },
	{ 1643,	 -50 },
	{ 1652,	 -60 },
	{ 1663,	 -70 },
};
/* temperature table for ADC 7 */
static struct sec_bat_adc_table_data temper_table_ADC7[] =  {
	{  300,	 670 },
	{  310,	 660 },
	{  324,	 650 },
	{  330,	 640 },
	{  340,	 630 },
	{  353,	 620 },
	{  368,	 610 },
	{  394,	 600 },
	{  394,	 590 },
	{  401,	 580 },
	{  418,	 570 },
	{  431,	 560 },
	{  445,	 550 },
	{  460,	 540 },
	{  478,	 530 },
	{  496,	 520 },
	{  507,	 510 },
	{  513,	 500 },
	{  531,	 490 },
	{  553,	 480 },
	{  571,	 470 },
	{  586,	 460 },
	{  604,	 450 },
	{  614,	 440 },
	{  640,	 430 },
	{  659,	 420 },
	{  669,	 410 },
	{  707,	 400 },
	{  722,	 390 },
	{  740,	 380 },
	{  769,	 370 },
	{  783,	 360 },
	{  816,	 350 },
	{  818,	 340 },
	{  845,	 330 },
	{  859,	 320 },
	{  889,	 310 },
	{  929,	 300 },
	{  942,	 290 },
	{  955,	 280 },
	{  972,	 270 },
	{  996,	 260 },
	{ 1040,	 250 },
	{ 1049,	 240 },
	{ 1073,	 230 },
	{ 1096,	 220 },
	{ 1114,	 210 },
	{ 1159,	 200 },
	{ 1165,	 190 },
	{ 1206,	 180 },
	{ 1214,	 170 },
	{ 1227,	 160 },
	{ 1256,	 150 },
	{ 1275,	 140 },
	{ 1301,	 130 },
	{ 1308,	 120 },
	{ 1357,	 110 },
	{ 1388,	 100 },
	{ 1396,	  90 },
	{ 1430,	  80 },
	{ 1448,	  70 },
	{ 1468,	  60 },
	{ 1499,	  50 },
	{ 1506,	  40 },
	{ 1522,	  30 },
	{ 1535,	  20 },
	{ 1561,	  10 },
	{ 1567,	   0 },
	{ 1595,	 -10 },
	{ 1620,	 -20 },
	{ 1637,	 -30 },
	{ 1640,	 -40 },
	{ 1668,	 -50 },
	{ 1669,	 -60 },
	{ 1688,	 -70 },
};
#else
/* temperature table for ADC 6 */
static struct sec_bat_adc_table_data temper_table[] =  {
	{  273,	 670 },
	{  289,	 660 },
	{  304,	 650 },
	{  314,	 640 },
	{  325,	 630 },
	{  337,	 620 },
	{  347,	 610 },
	{  361,	 600 },
	{  376,	 590 },
	{  391,	 580 },
	{  406,	 570 },
	{  417,	 560 },
	{  431,	 550 },
	{  447,	 540 },
	{  474,	 530 },
	{  491,	 520 },
	{  499,	 510 },
	{  511,	 500 },
	{  519,	 490 },
	{  547,	 480 },
	{  568,	 470 },
	{  585,	 460 },
	{  597,	 450 },
	{  614,	 440 },
	{  629,	 430 },
	{  647,	 420 },
	{  672,	 410 },
	{  690,	 400 },
	{  720,	 390 },
	{  735,	 380 },
	{  755,	 370 },
	{  775,	 360 },
	{  795,	 350 },
	{  818,	 340 },
	{  841,	 330 },
	{  864,	 320 },
	{  887,	 310 },
	{  909,	 300 },
	{  932,	 290 },
	{  954,	 280 },
	{  976,	 270 },
	{  999,	 260 },
	{ 1021,	 250 },
	{ 1051,	 240 },
	{ 1077,	 230 },
	{ 1103,	 220 },
	{ 1129,	 210 },
	{ 1155,	 200 },
	{ 1177,	 190 },
	{ 1199,	 180 },
	{ 1220,	 170 },
	{ 1242,	 160 },
	{ 1263,	 150 },
	{ 1284,	 140 },
	{ 1306,	 130 },
	{ 1326,	 120 },
	{ 1349,	 110 },
	{ 1369,	 100 },
	{ 1390,	  90 },
	{ 1411,	  80 },
	{ 1433,	  70 },
	{ 1454,	  60 },
	{ 1474,	  50 },
	{ 1486,	  40 },
	{ 1499,	  30 },
	{ 1512,	  20 },
	{ 1531,	  10 },
	{ 1548,	   0 },
	{ 1570,	 -10 },
	{ 1597,	 -20 },
	{ 1624,	 -30 },
	{ 1633,	 -40 },
	{ 1643,	 -50 },
	{ 1652,	 -60 },
	{ 1663,	 -70 },
};
#ifdef CONFIG_TARGET_LOCALE_NA
static struct sec_bat_adc_table_data  temper_table_ADC7[] =  {
	{  145,  670 },
	{  165,  660 },
	{  185,  650 },
	{  205,  640 },
	{  225,  630 },
	{  245,  620 },
	{  265,  610 },
	{  285,  600 },
	{  305,  590 },
	{  325,  580 },
	{  345,  570 },
	{  365,  560 },
	{  385,  550 },
	{  405,  540 },
	{  425,  530 },
	{  445,  520 },
	{  465,  510 },
	{  485,  500 },
	{  505,  490 },
	{  525,  480 },
	{  545,  470 },
	{  565,  460 },
	{  585,  450 },
	{  605,  440 },
	{  625,  430 },
	{  645,  420 },
	{  665,  410 },
	{  685,  400 },
	{  705,  390 },
	{  725,  380 },
	{  745,  370 },
	{  765,  360 },
	{  785,  350 },
	{  805,  340 },
	{  825,  330 },
	{  845,  320 },
	{  865,  310 },
	{  885,  300 },
	{  905,  290 },
	{  925,  280 },
	{  945,  270 },
	{  965,  260 },
	{  995,  250 },
	{ 1015,  240 },
	{ 1045,  230 },
	{ 1065,  220 },
	{ 1085,  210 },
	{ 1105,  200 },
	{ 1125,  190 },
	{ 1145,  180 },
	{ 1165,  170 },
	{ 1185,  160 },
	{ 1205,  150 },
	{ 1225,  140 },
	{ 1245,  130 },
	{ 1265,  120 },
	{ 1285,  110 },
	{ 1305,  100 },
	{ 1335,   90 },
	{ 1365,   80 },
	{ 1395,   70 },
	{ 1425,   60 },
	{ 1455,   50 },
	{ 1475,   40 },
	{ 1495,   30 },
	{ 1515,   20 },
	{ 1535,   10 },
	{ 1545,    0 },
	{ 1555,  -10 },
	{ 1565,  -20 },
	{ 1575,  -30 },
	{ 1585,  -40 },
	{ 1595,  -50 },
	{ 1605,  -60 },
	{ 1615,  -70 },
	{ 1625,  -80 },
	{ 1635,  -90 },
	{ 1645,  -100 },
	{ 1655,  -110 },
	{ 1665,  -120 },
	{ 1675,  -130 },
	{ 1685,  -140 },
};
#else
/* temperature table for ADC 7 */
static struct sec_bat_adc_table_data temper_table_ADC7[] =  {
	{  289,	 670 },
	{  304,	 660 },
	{  314,	 650 },
	{  325,	 640 },
	{  337,	 630 },
	{  347,	 620 },
	{  358,	 610 },
	{  371,	 600 },
	{  384,	 590 },
	{  396,	 580 },
	{  407,	 570 },
	{  419,	 560 },
	{  431,	 550 },
	{  447,	 540 },
	{  474,	 530 },
	{  491,	 520 },
	{  499,	 510 },
	{  511,	 500 },
	{  529,	 490 },
	{  547,	 480 },
	{  568,	 470 },
	{  585,	 460 },
	{  597,	 450 },
	{  611,	 440 },
	{  626,	 430 },
	{  643,	 420 },
	{  665,	 410 },
	{  680,	 400 },
	{  703,	 390 },
	{  720,	 380 },
	{  743,	 370 },
	{  765,	 360 },
	{  789,	 350 },
	{  813,	 340 },
	{  841,	 330 },
	{  864,	 320 },
	{  887,	 310 },
	{  909,	 300 },
	{  932,	 290 },
	{  954,	 280 },
	{  976,	 270 },
	{  999,	 260 },
	{ 1021,	 250 },
	{ 1051,	 240 },
	{ 1077,	 230 },
	{ 1103,	 220 },
	{ 1129,	 210 },
	{ 1155,	 200 },
	{ 1177,	 190 },
	{ 1199,	 180 },
	{ 1220,	 170 },
	{ 1242,	 160 },
	{ 1263,	 150 },
	{ 1284,	 140 },
	{ 1306,	 130 },
	{ 1326,	 120 },
	{ 1349,	 110 },
	{ 1369,	 100 },
	{ 1390,	  90 },
	{ 1411,	  80 },
	{ 1433,	  70 },
	{ 1454,	  60 },
	{ 1474,	  50 },
	{ 1486,	  40 },
	{ 1499,	  30 },
	{ 1512,	  20 },
	{ 1531,	  10 },
	{ 1548,	   0 },
	{ 1570,	 -10 },
	{ 1597,	 -20 },
	{ 1624,	 -30 },
	{ 1633,	 -40 },
	{ 1643,	 -50 },
	{ 1652,	 -60 },
	{ 1663,	 -70 },
};
#endif
#endif
#define ADC_CH_TEMPERATURE_PMIC	6
#define ADC_CH_TEMPERATURE_LCD	7
#if defined(CONFIG_TARGET_LOCALE_NAATT)
#define ADC_CH_VF_BATT	2
#endif

static unsigned int sec_bat_get_lpcharging_state(void)
{
	u32 val = __raw_readl(S5P_INFORM2);
	struct power_supply *psy = power_supply_get_by_name("max8997-charger");
	union power_supply_propval value;

	BUG_ON(!psy);

	if (val == 1) {
		psy->get_property(psy, POWER_SUPPLY_PROP_STATUS, &value);
		pr_info("%s: charging status: %d\n", __func__, value.intval);
		if (value.intval == POWER_SUPPLY_STATUS_DISCHARGING)
			pr_warn("%s: DISCHARGING\n", __func__);
	}

	pr_info("%s: LP charging:%d\n", __func__, val);
	return val;
}

/*  : Fix me regarding ADC kernel Panic
static void sec_bat_get_adc_value_cb(int value)
{
	if (sec_battery_cbs.lcd_set_adc_value) {
		sec_battery_cbs.lcd_set_adc_value(value);
	}
}
*/

static struct sec_bat_platform_data sec_bat_pdata = {
	.fuel_gauge_name	= "fuelgauge",
	.charger_name		= "max8997-charger",
#ifdef CONFIG_MAX8922_CHARGER
	.sub_charger_name	= "max8922-charger",
#endif
	/* TODO: should provide temperature table */
	.adc_arr_size		= ARRAY_SIZE(temper_table),
	.adc_table			= temper_table,
	.adc_channel		= ADC_CH_TEMPERATURE_PMIC,
#ifndef CONFIG_TARGET_LOCALE_KOR
	.adc_sub_arr_size	= ARRAY_SIZE(temper_table_ADC7),
	.adc_sub_table		= temper_table_ADC7,
	.adc_sub_channel	= ADC_CH_TEMPERATURE_LCD,
#endif
#if defined(CONFIG_TARGET_LOCALE_NAATT)
	.adc_vf_channel		= ADC_CH_VF_BATT,
#endif
	.get_lpcharging_state	= sec_bat_get_lpcharging_state,
};

static struct platform_device sec_device_battery = {
	.name = "sec-battery",
	.id = -1,
	.dev.platform_data = &sec_bat_pdata,
};
#endif /* CONFIG_BATTERY_SEC */

#ifdef CONFIG_LEDS_GPIO
struct gpio_led leds_gpio[] = {
	{
		.name = "red",
		.default_trigger = NULL,        /* "default-on", // Turn ON RED LED at boot time ! */
		.gpio = GPIO_SVC_LED_RED,
		.active_low = 0,
	},
	{
		.name = "blue",
		.default_trigger = NULL,	/* "default-on", // Turn ON RED LED at boot time ! */
		.gpio = GPIO_SVC_LED_BLUE,
		.active_low = 0,
	}
};


struct gpio_led_platform_data leds_gpio_platform_data = {
	.num_leds = ARRAY_SIZE(leds_gpio),
	.leds = leds_gpio,
};


struct platform_device sec_device_leds_gpio = {
	.name   = "leds-gpio",
	.id     = -1,
	.dev = { .platform_data = &leds_gpio_platform_data },
};
#endif /* CONFIG_LEDS_GPIO */

static struct platform_device c1_regulator_consumer = {
	.name = "c1-regulator-consumer",
	.id = -1,
};

#ifdef CONFIG_MAX8922_CHARGER
static int max8922_cfg_gpio(void)
{
	if (system_rev < HWREV_FOR_BATTERY)
		return -ENODEV;

	s3c_gpio_cfgpin(GPIO_CHG_EN, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_CHG_EN, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_CHG_EN, GPIO_LEVEL_LOW);

	s3c_gpio_cfgpin(GPIO_CHG_ING_N, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_CHG_ING_N, S3C_GPIO_PULL_NONE);

	s3c_gpio_cfgpin(GPIO_TA_nCONNECTED, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_TA_nCONNECTED, S3C_GPIO_PULL_NONE);

	return 0;
}

static struct max8922_platform_data max8922_pdata = {
	.topoff_cb = c1_charger_topoff_cb,
	.cfg_gpio = max8922_cfg_gpio,
	.gpio_chg_en = GPIO_CHG_EN,
	.gpio_chg_ing = GPIO_CHG_ING_N,
	.gpio_ta_nconnected = GPIO_TA_nCONNECTED,
};

static struct platform_device max8922_device_charger = {
	.name = "max8922-charger",
	.id = -1,
	.dev.platform_data = &max8922_pdata,
};
#endif /* CONFIG_MAX8922_CHARGER */

static struct platform_device sec_device_rfkill = {
	.name = "bt_rfkill",
	.id = -1,
};

struct gpio_keys_button c1_buttons[] = {
	{
		.code = KEY_VOLUMEUP,
		.gpio = GPIO_VOL_UP,
		.active_low = 1,
		.type = EV_KEY,
		.wakeup = 1,
		.can_disable = 1,
		.isr_hook = sec_debug_check_crash_key,
	}, {
		.code = KEY_VOLUMEDOWN,
		.gpio = GPIO_VOL_DOWN,
		.active_low = 1,
		.type = EV_KEY,
		.wakeup = 1,
		.can_disable = 1,
		.isr_hook = sec_debug_check_crash_key,
	}, {
#if defined(GPIO_CAM_HALF)
		.code = 113,
		.gpio = GPIO_CAM_HALF,
		.active_low = 1,
		.type = EV_KEY,
	}, {
#endif
#if defined(GPIO_CAM_FULL)
		.code = 112,
		.gpio = GPIO_CAM_FULL,
		.active_low = 1,
		.type = EV_KEY,
	}, {
#endif
#if defined(CONFIG_TARGET_LOCALE_NAATT)
		.code = KEY_POWER,
		.gpio = GPIO_nPOWER,
		.active_low = 1,
		.type = EV_KEY,
		.wakeup = 1,
		.isr_hook = sec_debug_check_crash_key,
	}
#else
		.code = KEY_POWER,
		.gpio = GPIO_nPOWER,
		.active_low = 1,
		.type = EV_KEY,
		.wakeup = 1,
#if !defined(CONFIG_TARGET_LOCALE_NA)
	}, {
		.code = KEY_HOME,
		.gpio = GPIO_OK_KEY,
		.active_low = 1,
		.type = EV_KEY,
		.wakeup = 1,
		.isr_hook = sec_debug_check_crash_key,
#endif
	}
#endif
};

struct gpio_keys_platform_data c1_keypad_platform_data = {
	c1_buttons,
	ARRAY_SIZE(c1_buttons),
};

struct platform_device c1_keypad = {
	.name	= "sec_key",
	.dev.platform_data = &c1_keypad_platform_data,
};

#ifdef CONFIG_SEC_DEV_JACK
static void sec_set_jack_micbias(bool on)
{
#ifdef CONFIG_SND_SOC_USE_EXTERNAL_MIC_BIAS
	if (system_rev >= 3)
		gpio_set_value(GPIO_EAR_MIC_BIAS_EN, on);
	else
		gpio_set_value(GPIO_MIC_BIAS_EN, on);
#endif

	return;
}

static struct sec_jack_zone sec_jack_zones[] = {
	{
		/* adc == 0, unstable zone, default to 3pole if it stays
		 * in this range for 300ms (15ms delays, 20 samples)
		 */
		.adc_high = 0,
		.delay_ms = 15,
		.check_count = 20,
		.jack_type = SEC_HEADSET_3POLE,
	},
	{
		/* 0 < adc <= 1200, unstable zone, default to 3pole if it stays
		 * in this range for 300ms (15ms delays, 20 samples)
		 */
		.adc_high = 1200,
		.delay_ms = 10,
		.check_count = 80,
		.jack_type = SEC_HEADSET_3POLE,
	},
	{
		/* 950 < adc <= 2600, unstable zone, default to 4pole if it
		 * stays in this range for 800ms (10ms delays, 80 samples)
		 */
		.adc_high = 2600,
		.delay_ms = 10,
		.check_count = 10,
		.jack_type = SEC_HEADSET_4POLE,
	},
	{
		/* 2600 < adc <= 3400, 3 pole zone, default to 3pole if it
		 * stays in this range for 100ms (10ms delays, 10 samples)
		 */
		.adc_high = 3800,
		.delay_ms = 10,
		.check_count = 5,
		.jack_type = SEC_HEADSET_4POLE,
	},
	{
		/* adc > 3400, unstable zone, default to 3pole if it stays
		 * in this range for two seconds (10ms delays, 200 samples)
		 */
		.adc_high = 0x7fffffff,
		.delay_ms = 10,
		.check_count = 200,
		.jack_type = SEC_HEADSET_3POLE,
	},
};

/* To support 3-buttons earjack */
static struct sec_jack_buttons_zone sec_jack_buttons_zones[] = {
	{
		/* 0 <= adc <=150, stable zone */
		.code		= KEY_MEDIA,
		.adc_low	= 0,
		.adc_high	= 145,
	},
	{
		/* 190 <= adc <= 320, stable zone */
		.code		= KEY_PREVIOUSSONG,
		.adc_low	= 150,
		.adc_high	= 370,
	},
	{
		/* 420 <= adc <= 800, stable zone */
		.code		= KEY_NEXTSONG,
		.adc_low	= 400,
		.adc_high	= 820,
	},
};

static struct sec_jack_platform_data sec_jack_data = {
	.set_micbias_state	= sec_set_jack_micbias,
	.zones			= sec_jack_zones,
	.num_zones		= ARRAY_SIZE(sec_jack_zones),
	.buttons_zones		= sec_jack_buttons_zones,
	.num_buttons_zones	= ARRAY_SIZE(sec_jack_buttons_zones),
	.det_gpio		= GPIO_DET_35,
	.send_end_gpio		= GPIO_EAR_SEND_END,
};

static struct platform_device sec_device_jack = {
	.name			= "sec_jack",
	.id			= 1, /* will be used also for gpio_event id */
	.dev.platform_data	= &sec_jack_data,
};
#endif	/* #ifdef CONFIG_SEC_DEV_JACK */

#if defined (CONFIG_SAMSUNG_PHONE_TTY)
struct platform_device sec_device_dpram = {
	.name = "dpram-device",
	.id   = -1,
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

#ifdef CONFIG_S5PV310_WATCHDOG_RESET
static struct platform_device watchdog_reset_device = {
	.name = "watchdog-reset",
	.id = -1,
};
#endif

static struct platform_device *smdkc210_devices[] __initdata = {
#ifdef CONFIG_S5PV310_WATCHDOG_RESET
	&watchdog_reset_device,
#endif
#ifdef CONFIG_S5PV310_DEV_PD
	&s5pv310_device_pd[PD_MFC],
	&s5pv310_device_pd[PD_G3D],
	&s5pv310_device_pd[PD_LCD0],
	&s5pv310_device_pd[PD_LCD1],
	&s5pv310_device_pd[PD_CAM],
#ifndef CONFIG_TARGET_LOCALE_NA
	&s5pv310_device_pd[PD_GPS],
#endif /* CONFIG_TARGET_LOCALE_NA */
	&s5pv310_device_pd[PD_TV],
	/* &s5pv310_device_pd[PD_MAUDIO], */
#endif

	&smdkc210_smsc911x,
#ifdef CONFIG_BATTERY_S3C
	&sec_fake_battery,
#endif
#ifdef CONFIG_BATTERY_SEC
	&sec_device_battery,
#endif
#ifdef	CONFIG_LEDS_GPIO
	&sec_device_leds_gpio,
#endif
#ifdef CONFIG_MAX8922_CHARGER
	&max8922_device_charger,
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
#if defined(CONFIG_S3C_DEV_I2C8_EMUL)
	&s3c_device_i2c8,
#endif
#if defined(CONFIG_S3C_DEV_I2C9_EMUL)
	&s3c_device_i2c9,
#endif
#if defined(CONFIG_S3C_DEV_I2C10_EMUL)
	&s3c_device_i2c10,
#endif
#if defined(CONFIG_S3C_DEV_I2C11_EMUL)
	&s3c_device_i2c11,
#endif
#if defined(CONFIG_VIDEO_S5K6AAFX) || defined(CONFIG_VIDEO_S5K5BAFX) || defined(CONFIG_VIDEO_S5K5BBGX)
	&s3c_device_i2c12,
#endif
#ifdef CONFIG_SND_SOC_MIC_A1026
	&s3c_device_i2c13,
#endif
#ifdef CONFIG_PN544
	&s3c_device_i2c14,	/* NFC Sensor */
#endif
#ifdef CONFIG_VIDEO_MHL_V1
	&s3c_device_i2c15,	/* MHL */
#endif
#ifdef CONFIG_FM_SI4709_MODULE
	&s3c_device_i2c16,	/* Si4709 */
#endif
#ifdef CONFIG_VIDEO_M5MO_USE_SWI2C
	&s3c_device_i2c25,
#endif
#if defined(CONFIG_S3C_DEV_I2C17_EMUL)
	&s3c_device_i2c17,	/* USB HUB */
#endif
#ifdef	CONFIG_ISDBT_FC8100
	&s3c_device_i2c17,	/* ISDBT */
#endif
#if defined(CONFIG_WIMAX_CMC) && defined(CONFIG_TARGET_LOCALE_NA)
	&wmxeeprom_i2c_gpio_device,
#endif
#endif
	/* consumer driver should resume after resuming i2c drivers */
	&c1_regulator_consumer,
#if defined(CONFIG_SND_S3C64XX_SOC_I2S_V4)
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

#ifdef CONFIG_S5P_DEV_MSHC
	&s3c_device_mshci,
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

#ifdef CONFIG_ANDROID_PMEM
	&pmem_device,
	&pmem_gpu1_device,
	&pmem_adsp_device,
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
#ifdef CONFIG_USB_S3C_OTG_HOST
	&s3c_device_usb_otghcd,
#endif
#if defined(CONFIG_S3C64XX_DEV_SPI) && !defined(CONFIG_EPEN_WACOM_G5SP)
	&s5pv310_device_spi0,
#endif
#ifdef CONFIG_FB_S3C_LD9040
	&ld9040_spi_gpio,
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
#ifdef CONFIG_SATA_AHCI_PLATFORM
	&s5pv310_device_sata,
#endif
	&c1_keypad,
#ifdef CONFIG_SEC_DEV_JACK
	&sec_device_jack,
#endif
	&sec_device_rfkill,

#ifdef CONFIG_HAVE_PWM
	&s3c_device_timer[0],
	&s3c_device_timer[1],
	&s3c_device_timer[2],
	&s3c_device_timer[3],
#endif
#ifdef CONFIG_VIDEO_TSI
	&s3c_device_tsi,
#endif
#ifdef CONFIG_SAMSUNG_PHONE_TTY
	&sec_device_dpram,
#endif
	&pmu_device,

#ifdef CONFIG_DEV_THERMAL
	&s5p_device_tmu,
#endif

#if defined(CONFIG_WIMAX_CMC) && defined(CONFIG_TARGET_LOCALE_NA)
	&s3c_device_cmc732,
#endif

};

#ifdef CONFIG_VIDEO_TVOUT
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

	__raw_writel(0xff1ffff1, S5P_SROM_BC1);

	tmp = __raw_readl(S5P_VA_GPIO + 0x120);
	tmp &= ~(0xffffff);
	tmp |= (0x221121);
	__raw_writel(tmp, (S5P_VA_GPIO + 0x120));

	__raw_writel(0x22222222, (S5P_VA_GPIO + 0x180));
	__raw_writel(0x22222222, (S5P_VA_GPIO + 0x1a0));
	__raw_writel(0x22222222, (S5P_VA_GPIO + 0x1c0));
	__raw_writel(0x22222222, (S5P_VA_GPIO + 0x1e0));
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
	sec_getlog_supply_meminfo(meminfo.bank[0].size, meminfo.bank[0].start,
				  meminfo.bank[1].size, meminfo.bank[1].start);

	/* as soon as INFORM3 is visible, sec_debug is ready to run */
	sec_debug_init();
}

#ifdef CONFIG_S3C_ADC
static struct s3c_adc_mach_info s3c_adc_platform __initdata = {
	/*    s5pc100 supports 12-bit resolution */
	.delay		= 10000,
	.presc		= 49,
	.resolution	= 12,
};
#endif

static void smdkc210_power_off(void)
{
	int poweroff_try = 0;
	pr_emerg("%s\n", __func__);

	while (1) {
		/* Check reboot charging */
		if (is_cable_attached || (poweroff_try >= 5)) {
			pr_emerg("%s: charger connected(%d) or power off failed(%d), reboot!\n",
				 __func__, is_cable_attached, poweroff_try);
			writel(0x0, S5P_INFORM2); /* To enter LP charging */

			flush_cache_all();
			outer_flush_all();
			arch_reset(0, 0);

			pr_emerg("%s: waiting for reboot\n", __func__);
			while (1)
			;
		}

		/* wait for power button release */
		if (gpio_get_value(GPIO_nPOWER)) {
			pr_emerg("%s: set PS_HOLD low\n", __func__);

			/* power off code
			 * PS_HOLD Out/High -->
			 * Low PS_HOLD_CONTROL, R/W, 0x1002_330C
			 */
			writel(readl(S5P_PS_HOLD_CONTROL) & 0xFFFFFEFF,
					S5P_PS_HOLD_CONTROL);

			++poweroff_try;
			pr_emerg("%s: Should not reach here! (poweroff_try:%d)\n",
				 __func__, poweroff_try);
		} else {
			/* if power button is not released, wait and check TA again */
			pr_info("%s: PowerButton is not released.\n", __func__);
		}
		mdelay(1000);
	}
}

#define REBOOT_PREFIX		0x12345670
#define REBOOT_MODE_NONE	0
#define REBOOT_MODE_DOWNLOAD	1
#define REBOOT_MODE_UPLOAD	2
#define REBOOT_MODE_CHARGING	3
#define REBOOT_MODE_RECOVERY	4
#define REBOOT_MODE_ARM11_FOTA	5

#if defined(CONFIG_TARGET_LOCALE_NA)
#define REBOOT_MODE_ARM9_FOTA	6
#endif

static void c1_reboot(char str, const char *cmd)
{
	pr_emerg("%s (%d, %s)\n", __func__, str, cmd ? cmd : "(null)");

	writel(0x12345678, S5P_INFORM2);	/* Don't enter lpm mode */

	if (!cmd) {
		writel(REBOOT_PREFIX | REBOOT_MODE_NONE, S5P_INFORM3);
	} else {
		if (!strcmp(cmd, "arm11_fota"))
			writel(REBOOT_PREFIX | REBOOT_MODE_ARM11_FOTA,
			       S5P_INFORM3);
#if defined(CONFIG_TARGET_LOCALE_NA)
		else if (!strcmp(cmd, "arm9_fota"))
			writel(REBOOT_PREFIX | REBOOT_MODE_ARM9_FOTA,
			       S5P_INFORM3);
#endif
		else if (!strcmp(cmd, "recovery"))
			writel(REBOOT_PREFIX | REBOOT_MODE_RECOVERY,
			       S5P_INFORM3);
		else if (!strcmp(cmd, "download"))
			writel(REBOOT_PREFIX | REBOOT_MODE_DOWNLOAD,
			       S5P_INFORM3);
		else if (!strcmp(cmd, "upload"))
			writel(REBOOT_PREFIX | REBOOT_MODE_UPLOAD,
			       S5P_INFORM3);
		else
			writel(REBOOT_PREFIX | REBOOT_MODE_NONE,
			       S5P_INFORM3);
	}

	flush_cache_all();
	outer_flush_all();
	arch_reset(0, 0);

	pr_emerg("%s: waiting for reboot\n", __func__);
	while (1)
	;
}

static void __init universal_tsp_init(void)
{
	int gpio;

	/* TSP_LDO_ON: XMDMADDR_11 */
	gpio = GPIO_TSP_LDO_ON;
	gpio_request(gpio, "TSP_LDO_ON");
	gpio_direction_output(gpio, 1);
	s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_NONE);
	gpio_export(gpio, 0);

	/* TSP_INT: XMDMADDR_7 */
	gpio = GPIO_TSP_INT;
	gpio_request(gpio, "TSP_INT");
	s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(0xf));
	/* s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP); */
	s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	i2c_devs3[0].irq = gpio_to_irq(gpio);

	printk("%s touch : %d\n", __func__, i2c_devs3[0].irq);
}

#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
/* Changes value of nluns in order to use external storage */
static void __init usb_device_init(void)
{
	struct usb_mass_storage_platform_data *ums_pdata =
	    s3c_device_usb_mass_storage.dev.platform_data;
	if (ums_pdata) {
		printk(KERN_DEBUG "%s: default luns=%d, system_rev=%d\n",
		       __func__, ums_pdata->nluns, system_rev);
		if (system_rev >= 0x3) {
			printk(KERN_DEBUG "nluns is changed to %d from %d.\n",
			       2, ums_pdata->nluns);
			ums_pdata->nluns = 2;
		}
	} else {
		printk(KERN_DEBUG "I can't find s3c_device_usb_mass_storage\n");
	}
}
#endif

static void __init smdkc210_machine_init(void)
{
#if defined(CONFIG_S3C64XX_DEV_SPI)
	struct clk *sclk = NULL;
	struct clk *prnt = NULL;
	struct device *spi0_dev = &s5pv310_device_spi0.dev;
#endif

	/* to support system shut down */
	pm_power_off = smdkc210_power_off;
	arm_pm_restart = c1_reboot;

	/* initialise the gpios */
	c1_config_gpio_table();
	s3c_config_sleep_gpio_table = c1_config_sleep_gpio_table;

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
#ifndef CONFIG_TARGET_LOCALE_NA
	s5pv310_pd_enable(&s5pv310_device_pd[PD_GPS].dev);
#endif /* CONFIG_TARGET_LOCALE_NA */
#endif

	sromc_setup();

	s3c_gpio_cfgpin(GPIO_MASSMEM_EN, S3C_GPIO_OUTPUT);
	gpio_set_value(GPIO_MASSMEM_EN, GPIO_MASSMEM_EN_LEVEL);

	/* 400 kHz for initialization of MMC Card  */
	__raw_writel((__raw_readl(S5P_CLKDIV_FSYS3) & 0xfffffff0)
		     | 0x9, S5P_CLKDIV_FSYS3);
	__raw_writel((__raw_readl(S5P_CLKDIV_FSYS2) & 0xfff0fff0)
		     | 0x80008, S5P_CLKDIV_FSYS2);
	__raw_writel((__raw_readl(S5P_CLKDIV_FSYS1) & 0xfff0fff0)
		     | 0x90009, S5P_CLKDIV_FSYS1);


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
	universal_tsp_init();
	s3c_i2c3_set_platdata(NULL);
	i2c_register_board_info(3, i2c_devs3, ARRAY_SIZE(i2c_devs3));
#endif
#ifdef CONFIG_S3C_DEV_I2C4
#ifdef CONFIG_EPEN_WACOM_G5SP
	p6_digitizer_init();
#endif /* CONFIG_EPEN_WACOM_G5SP */
	s3c_i2c4_set_platdata(NULL);
	i2c_register_board_info(4, i2c_devs4, ARRAY_SIZE(i2c_devs4));
#endif /* CONFIG_S3C_DEV_I2C4 */
#ifdef CONFIG_S3C_DEV_I2C5
	s3c_i2c5_set_platdata(NULL);
	s3c_gpio_setpull(GPIO_PMIC_IRQ, S3C_GPIO_PULL_NONE);
	i2c_devs5[0].irq = gpio_to_irq(GPIO_PMIC_IRQ);
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
#ifdef CONFIG_S3C_DEV_I2C8_EMUL
	i2c_register_board_info(8, i2c_devs8_emul, ARRAY_SIZE(i2c_devs8_emul));
#endif
#ifdef CONFIG_S3C_DEV_I2C9_EMUL
	i2c_register_board_info(9, i2c_devs9_emul, ARRAY_SIZE(i2c_devs9_emul));
#endif
#ifdef CONFIG_S3C_DEV_I2C10_EMUL
	i2c_register_board_info(10, i2c_devs10_emul, ARRAY_SIZE(i2c_devs10_emul));
#endif
#ifdef CONFIG_S3C_DEV_I2C11_EMUL
	s3c_gpio_setpull(GPIO_PS_ALS_INT, S3C_GPIO_PULL_NONE);
	i2c_register_board_info(11, i2c_devs11_emul, ARRAY_SIZE(i2c_devs11_emul));
#endif
#if defined(CONFIG_VIDEO_S5K6AAFX) || defined(CONFIG_VIDEO_S5K5BAFX) || defined(CONFIG_VIDEO_S5K5BBGX)
	i2c_register_board_info(12, i2c_devs12_emul, ARRAY_SIZE(i2c_devs12_emul));
#endif
#ifdef CONFIG_SND_SOC_MIC_A1026
	i2c_register_board_info(13, i2c_devs13_emul, ARRAY_SIZE(i2c_devs13_emul));
#endif
#ifdef CONFIG_PN544
	/* NFC sensor */
	nfc_setup_gpio();
	i2c_register_board_info(14, i2c_devs14, ARRAY_SIZE(i2c_devs14));
#endif
#ifdef CONFIG_VIDEO_MHL_V1
	i2c_register_board_info(15, i2c_devs15, ARRAY_SIZE(i2c_devs15));
#endif
#ifdef CONFIG_FM_SI4709_MODULE
	i2c_register_board_info(16, i2c_devs16, ARRAY_SIZE(i2c_devs16));
#endif
#ifdef CONFIG_VIDEO_M5MO_USE_SWI2C
	/* i2c_register_board_info(25, i2c_devs25_emul,
				ARRAY_SIZE(i2c_devs25_emul)); */
#endif
#ifdef	CONFIG_ISDBT_FC8100
	i2c_register_board_info(17, i2c_devs17, ARRAY_SIZE(i2c_devs17));
#endif
#endif

#ifdef CONFIG_S3C_DEV_I2C17_EMUL
	i2c_register_board_info(17, i2c_devs17_emul, ARRAY_SIZE(i2c_devs17_emul));
#endif

#if defined(CONFIG_WIMAX_CMC) && defined(CONFIG_TARGET_LOCALE_NA)
	i2c_register_board_info(18, wmxeeprom_i2c_devices, ARRAY_SIZE(wmxeeprom_i2c_devices));
#endif


#ifdef CONFIG_FB_S3C
	s3cfb_set_platdata(NULL);
#endif

#ifdef CONFIG_VIDEO_FIMC
	/* fimc */
	s3c_fimc0_set_platdata(&fimc_plat);
	s3c_fimc1_set_platdata(&fimc_plat);
	s3c_fimc2_set_platdata(&fimc_plat);
	s3c_fimc3_set_platdata(&fimc_plat);
#ifdef CONFIG_ITU_A
#endif
#ifdef CONFIG_ITU_B
	smdkv310_cam1_reset(1);
#endif
#ifdef CONFIG_VIDEO_FIMC_MIPI
	s3c_csis0_set_platdata(NULL);
	s3c_csis1_set_platdata(NULL);
#endif
#endif

#ifdef CONFIG_VIDEO_MFC5X
#ifdef CONFIG_S5PV310_DEV_PD
	s5p_device_mfc.dev.parent = &s5pv310_device_pd[PD_MFC].dev;
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

#ifdef CONFIG_S3C_ADC
	s3c_adc_set_platdata(&s3c_adc_platform);
#endif

#ifdef CONFIG_DEV_THERMAL
	s5p_tmu_set_platdata(NULL);
#endif

#ifdef CONFIG_VIDEO_TVOUT
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
#ifdef CONFIG_VIDEO_TVOUT
	s5p_device_tvout.dev.parent = &s5pv310_device_pd[PD_TV].dev;
#endif
#endif
#ifdef CONFIG_VIDEO_JPEG
	s5p_device_jpeg.dev.parent = &s5pv310_device_pd[PD_CAM].dev;
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


#ifdef CONFIG_FB_S3C_LD9040
	ld9040_fb_init();
#endif
#ifdef CONFIG_FB_S3C_MIPI_LCD
	mipi_fb_init();
#endif

	c1_sec_switch_init();
	uart_switch_init();
	c1_sound_init();

#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
/* soonyong.cho : This is for setting unique serial number */
	s3c_usb_set_serial();
/* Changes value of nluns in order to use external storage */
	usb_device_init();
#endif

/* klaatu: semaphore logging code - for debug  */
	debug_semaphore_init();
	debug_rwsemaphore_init();
/* --------------------------------------- */

	BUG_ON(!sec_class);
	gps_dev = device_create(sec_class, NULL, 0, NULL, "gps");
	if (IS_ERR(gps_dev))
		pr_err("Failed to create device(gps)!\n");
}

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
#ifdef CONFIG_ANDROID_PMEM_MEMSIZE_PMEM_ADSP
		{
			.name = "pmem_adsp",
			.size = CONFIG_ANDROID_PMEM_MEMSIZE_PMEM_ADSP * SZ_1K,
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
		"android_pmem.0=pmem;android_pmem.1=pmem_gpu1;android_pmem.2=pmem_adsp;"
		"s3cfb.0=fimd;"
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

MACHINE_START(C1, "SMDKC210")
	/* Maintainer: Kukjin Kim <kgene.kim@samsung.com> */
	.phys_io	= S3C_PA_UART & 0xfff00000,
	.io_pg_offst	= (((u32)S3C_VA_UART) >> 18) & 0xfffc,
	.boot_params	= S5P_PA_SDRAM + 0x100,
	.init_irq	= s5pv310_init_irq,
	.map_io		= smdkc210_map_io,
	.init_machine	= smdkc210_machine_init,
	.timer		= &s5pv310_timer,
MACHINE_END

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
