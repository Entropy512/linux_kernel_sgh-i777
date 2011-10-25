/* linux/arch/arm/mach-s5pv310/cpuidle.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/cpuidle.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>

#include <asm/proc-fns.h>
#include <asm/cacheflush.h>
#include <asm/hardware/cache-l2x0.h>

#include <plat/pm.h>
#include <plat/cpu.h>
#include <plat/gpio-cfg.h>
#include <plat/s5pv310.h>

#include <mach/regs-clock.h>
#include <mach/regs-gpio.h>
#include <mach/regs-pmu.h>
#include <plat/regs-otg.h>
#include <mach/gpio.h>
#include <plat/devs.h>

#include <mach/ext-gic.h>

/* enable AFTR/LPA feature */
static enum { ENABLE_IDLE = 0, ENABLE_AFTR = 1, ENABLE_LPA = 2 } enable_mask =
    ENABLE_IDLE | ENABLE_LPA;
module_param_named(enable_mask, enable_mask, uint, 0644);

static unsigned long *regs_save;
static dma_addr_t phy_regs_save;

/* #define AFTR_DEBUG */
#define L2_FLUSH_ALL_AFTR
#define MAX_CHK_DEV     0xf

/*
 * Specific device list for checking before entering
 * didle(LPA) mode
 */
#ifdef CONFIG_MTD_ONENAND
static void __iomem *onenandctl_vbase;
#endif

enum hc_type {
	HC_SDHC,
	HC_MSHC,
};


#define REG_DIRECTGO_ADDR	(s5pv310_subrev() == 0 ?\
				(S5P_VA_SYSRAM + 0x24) : S5P_INFORM7)
#define REG_DIRECTGO_FLAG	(s5pv310_subrev() == 0 ?\
				(S5P_VA_SYSRAM + 0x20) : S5P_INFORM6)

struct check_device_op {
	void __iomem		*base;
	struct platform_device	*pdev;
	enum hc_type type;
};

static unsigned int cpu_core;
static unsigned int old_div;
static DEFINE_SPINLOCK(idle_lock);

/* Array of checking devices list */
static struct check_device_op chk_dev_op[] = {
#if defined(CONFIG_S5P_DEV_MSHC)
	{.base = 0, .pdev = &s3c_device_mshci, .type = HC_MSHC},
#endif
#if defined(CONFIG_S3C_DEV_HSMMC)
	{.base = 0, .pdev = &s3c_device_hsmmc0, .type = HC_SDHC},
#endif
#if defined(CONFIG_S3C_DEV_HSMMC1)
	{.base = 0, .pdev = &s3c_device_hsmmc1, .type = HC_SDHC},
#endif
#if defined(CONFIG_S3C_DEV_HSMMC2)
	{.base = 0, .pdev = &s3c_device_hsmmc2, .type = HC_SDHC},
#endif
#if defined(CONFIG_S3C_DEV_HSMMC3)
	{.base = 0, .pdev = &s3c_device_hsmmc3, .type = HC_SDHC},
#endif
	{.base = 0, .pdev = NULL},
};

#define S3C_HSMMC_PRNSTS	(0x24)
#define S3C_HSMMC_CLKCON	(0x2c)
#define S3C_HSMMC_CMD_INHIBIT	0x00000001
#define S3C_HSMMC_DATA_INHIBIT	0x00000002
#define S3C_HSMMC_CLOCK_CARD_EN	0x0004

#define MSHCI_CLKENA	(0x10)	/* Clock enable */
#define MSHCI_STATUS	(0x48)	/* Status */
#define MSHCI_DATA_BUSY	(0x1<<9)
#define MSHCI_ENCLK	(0x1)


static struct sleep_save s5pv310_aftr_save[] = {
	/* GIC side */
#ifndef CONFIG_USE_EXT_GIC
	SAVE_ITEM(S5P_VA_GIC_CPU + 0x000),
	SAVE_ITEM(S5P_VA_GIC_CPU + 0x004),
	SAVE_ITEM(S5P_VA_GIC_CPU + 0x008),
	SAVE_ITEM(S5P_VA_GIC_CPU + 0x014),
	SAVE_ITEM(S5P_VA_GIC_CPU + 0x018),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x000),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x004),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x100),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x104),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x108),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x300),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x304),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x308),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x400),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x404),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x408),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x40C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x410),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x414),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x418),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x41C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x420),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x424),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x428),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x42C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x430),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x434),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x438),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x43C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x440),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x444),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x448),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x44C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x450),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x454),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x458),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x45C),

	SAVE_ITEM(S5P_VA_GIC_DIST + 0x800),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x804),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x808),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x80C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x810),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x814),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x818),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x81C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x820),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x824),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x828),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x82C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x830),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x834),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x838),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x83C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x840),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x844),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x848),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x84C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x850),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x854),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x858),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x85C),

	SAVE_ITEM(S5P_VA_GIC_DIST + 0xC00),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0xC04),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0xC08),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0xC0C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0xC10),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0xC14),

	SAVE_ITEM(S5P_VA_COMBINER_BASE + 0x000),
	SAVE_ITEM(S5P_VA_COMBINER_BASE + 0x010),
	SAVE_ITEM(S5P_VA_COMBINER_BASE + 0x020),
	SAVE_ITEM(S5P_VA_COMBINER_BASE + 0x030),
	SAVE_ITEM(S5P_VA_COMBINER_BASE + 0x040),
	SAVE_ITEM(S5P_VA_COMBINER_BASE + 0x050),
	SAVE_ITEM(S5P_VA_COMBINER_BASE + 0x060),
	SAVE_ITEM(S5P_VA_COMBINER_BASE + 0x070),
	SAVE_ITEM(S5P_VA_COMBINER_BASE + 0x080),
	SAVE_ITEM(S5P_VA_COMBINER_BASE + 0x090),
#ifdef CONFIG_CPU_S5PV310_EVT1
	SAVE_ITEM(S5P_VA_COMBINER_BASE + 0x0C0),
	SAVE_ITEM(S5P_VA_COMBINER_BASE + 0x0D0),
#endif
#endif
};

static struct sleep_save s5pv310_lpa_save[] = {
	/* GIC side */
#ifndef CONFIG_USE_EXT_GIC
	SAVE_ITEM(S5P_VA_GIC_CPU + 0x000),
	SAVE_ITEM(S5P_VA_GIC_CPU + 0x004),
	SAVE_ITEM(S5P_VA_GIC_CPU + 0x008),
	SAVE_ITEM(S5P_VA_GIC_CPU + 0x014),
	SAVE_ITEM(S5P_VA_GIC_CPU + 0x018),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x000),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x004),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x100),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x104),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x108),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x300),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x304),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x308),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x400),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x404),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x408),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x40C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x410),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x414),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x418),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x41C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x420),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x424),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x428),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x42C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x430),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x434),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x438),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x43C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x440),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x444),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x448),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x44C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x450),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x454),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x458),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x45C),

	SAVE_ITEM(S5P_VA_GIC_DIST + 0x800),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x804),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x808),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x80C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x810),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x814),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x818),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x81C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x820),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x824),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x828),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x82C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x830),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x834),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x838),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x83C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x840),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x844),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x848),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x84C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x850),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x854),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x858),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0x85C),

	SAVE_ITEM(S5P_VA_GIC_DIST + 0xC00),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0xC04),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0xC08),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0xC0C),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0xC10),
	SAVE_ITEM(S5P_VA_GIC_DIST + 0xC14),

	SAVE_ITEM(S5P_VA_COMBINER_BASE + 0x000),
	SAVE_ITEM(S5P_VA_COMBINER_BASE + 0x010),
	SAVE_ITEM(S5P_VA_COMBINER_BASE + 0x020),
	SAVE_ITEM(S5P_VA_COMBINER_BASE + 0x030),
	SAVE_ITEM(S5P_VA_COMBINER_BASE + 0x040),
	SAVE_ITEM(S5P_VA_COMBINER_BASE + 0x050),
	SAVE_ITEM(S5P_VA_COMBINER_BASE + 0x060),
	SAVE_ITEM(S5P_VA_COMBINER_BASE + 0x070),
	SAVE_ITEM(S5P_VA_COMBINER_BASE + 0x080),
	SAVE_ITEM(S5P_VA_COMBINER_BASE + 0x090),
#ifdef CONFIG_CPU_S5PV310_EVT1
	SAVE_ITEM(S5P_VA_COMBINER_BASE + 0x0C0),
	SAVE_ITEM(S5P_VA_COMBINER_BASE + 0x0D0),
#endif
#endif

#ifdef CONFIG_CPU_S5PV310_EVT1
	/* CMU side */
	SAVE_ITEM(S5P_CLKSRC_MASK_CAM),
	SAVE_ITEM(S5P_CLKSRC_MASK_TV),
	SAVE_ITEM(S5P_CLKSRC_MASK_LCD0),
	SAVE_ITEM(S5P_CLKSRC_MASK_LCD1),
	SAVE_ITEM(S5P_CLKSRC_MASK_MAUDIO),
	SAVE_ITEM(S5P_CLKSRC_MASK_FSYS),
	SAVE_ITEM(S5P_CLKSRC_MASK_PERIL0),
	SAVE_ITEM(S5P_CLKSRC_MASK_PERIL1),
	SAVE_ITEM(S5P_CLKSRC_MASK_DMC),
#else
	/* CMU side */
	SAVE_ITEM(S5P_CLKDIV_LEFTBUS),
	SAVE_ITEM(S5P_CLKDIV_RIGHTBUS),
	SAVE_ITEM(S5P_EPLL_CON0),
	SAVE_ITEM(S5P_EPLL_CON1),
	SAVE_ITEM(S5P_CLKSRC_TOP0),
	SAVE_ITEM(S5P_CLKSRC_TOP1),
	SAVE_ITEM(S5P_CLKSRC_CAM),
	SAVE_ITEM(S5P_CLKSRC_MFC),
	SAVE_ITEM(S5P_CLKSRC_IMAGE),
	SAVE_ITEM(S5P_CLKSRC_LCD0),
	SAVE_ITEM(S5P_CLKSRC_LCD1),
	SAVE_ITEM(S5P_CLKSRC_MAUDIO),
	SAVE_ITEM(S5P_CLKSRC_FSYS),
	SAVE_ITEM(S5P_CLKSRC_PERIL0),
	SAVE_ITEM(S5P_CLKSRC_PERIL1),
	SAVE_ITEM(S5P_CLKDIV_CAM),
	SAVE_ITEM(S5P_CLKDIV_IMAGE),
	SAVE_ITEM(S5P_CLKDIV_LCD0),
	SAVE_ITEM(S5P_CLKDIV_LCD1),
	SAVE_ITEM(S5P_CLKDIV_MAUDIO),
	SAVE_ITEM(S5P_CLKDIV_FSYS0),
	SAVE_ITEM(S5P_CLKDIV_FSYS1),
	SAVE_ITEM(S5P_CLKDIV_FSYS2),
	SAVE_ITEM(S5P_CLKDIV_FSYS3),
	SAVE_ITEM(S5P_CLKDIV_PERIL0),
	SAVE_ITEM(S5P_CLKDIV_PERIL1),
	SAVE_ITEM(S5P_CLKDIV_PERIL2),
	SAVE_ITEM(S5P_CLKDIV_PERIL3),
	SAVE_ITEM(S5P_CLKDIV_PERIL4),
	SAVE_ITEM(S5P_CLKDIV_PERIL5),
	SAVE_ITEM(S5P_CLKDIV_TOP),
	SAVE_ITEM(S5P_CLKSRC_MASK_CAM),
	SAVE_ITEM(S5P_CLKSRC_MASK_TV),
	SAVE_ITEM(S5P_CLKSRC_MASK_LCD0),
	SAVE_ITEM(S5P_CLKSRC_MASK_LCD1),
	SAVE_ITEM(S5P_CLKSRC_MASK_MAUDIO),
	SAVE_ITEM(S5P_CLKSRC_MASK_FSYS),
	SAVE_ITEM(S5P_CLKSRC_MASK_PERIL0),
	SAVE_ITEM(S5P_CLKSRC_MASK_PERIL1),
	SAVE_ITEM(S5P_CLKGATE_IP_CAM),
	SAVE_ITEM(S5P_CLKGATE_IP_MFC),
	SAVE_ITEM(S5P_CLKGATE_IP_IMAGE),
	SAVE_ITEM(S5P_CLKGATE_IP_LCD0),
	SAVE_ITEM(S5P_CLKGATE_IP_LCD1),
	SAVE_ITEM(S5P_CLKGATE_IP_FSYS),
	SAVE_ITEM(S5P_CLKGATE_IP_PERIL),
	SAVE_ITEM(S5P_CLKGATE_IP_PERIR),
	SAVE_ITEM(S5P_CLKSRC_MASK_DMC),
	SAVE_ITEM(S5P_CLKSRC_DMC),
	SAVE_ITEM(S5P_CLKDIV_DMC0),
	SAVE_ITEM(S5P_CLKSRC_CPU),
	SAVE_ITEM(S5P_CLKDIV_CPU),
	SAVE_ITEM(S5P_CLKGATE_SCLKCPU),
#endif
};

static struct sleep_save s5pv310_aftr[] = {
	{ .reg = S5P_ARM_CORE0_LOWPWR			, .val = 0x0, },
	{ .reg = S5P_DIS_IRQ_CORE0			, .val = 0x0, },
	{ .reg = S5P_DIS_IRQ_CENTRAL0			, .val = 0x0, },
	{ .reg = S5P_ARM_CORE1_LOWPWR			, .val = 0x0, },
	{ .reg = S5P_DIS_IRQ_CORE1			, .val = 0x0, },
	{ .reg = S5P_DIS_IRQ_CENTRAL1			, .val = 0x0, },
	{ .reg = S5P_ARM_COMMON_LOWPWR			, .val = 0x0, },
	{ .reg = S5P_L2_0_LOWPWR			, .val = 0x2, },
	{ .reg = S5P_L2_1_LOWPWR			, .val = 0x2, },
	{ .reg = S5P_CMU_ACLKSTOP_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_CMU_SCLKSTOP_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_CMU_RESET_LOWPWR			, .val = 0x1, },
	{ .reg = S5P_APLL_SYSCLK_LOWPWR			, .val = 0x1, },
	{ .reg = S5P_MPLL_SYSCLK_LOWPWR			, .val = 0x1, },
	{ .reg = S5P_VPLL_SYSCLK_LOWPWR			, .val = 0x1, },
	{ .reg = S5P_EPLL_SYSCLK_LOWPWR			, .val = 0x1, },
	{ .reg = S5P_CMU_CLKSTOP_GPS_ALIVE_LOWPWR	, .val = 0x1, },
	{ .reg = S5P_CMU_RESET_GPSALIVE_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_CMU_CLKSTOP_CAM_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_CMU_CLKSTOP_TV_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_CMU_CLKSTOP_MFC_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_CMU_CLKSTOP_G3D_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_CMU_CLKSTOP_LCD0_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_CMU_CLKSTOP_LCD1_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_CMU_CLKSTOP_MAUDIO_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_CMU_CLKSTOP_GPS_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_CMU_RESET_CAM_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_CMU_RESET_TV_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_CMU_RESET_MFC_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_CMU_RESET_G3D_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_CMU_RESET_LCD0_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_CMU_RESET_LCD1_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_CMU_RESET_MAUDIO_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_CMU_RESET_GPS_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_TOP_BUS_LOWPWR			, .val = 0x3, },
	{ .reg = S5P_TOP_RETENTION_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_TOP_PWR_LOWPWR			, .val = 0x3, },
	{ .reg = S5P_LOGIC_RESET_LOWPWR			, .val = 0x1, },
	{ .reg = S5P_ONENAND_MEM_LOWPWR			, .val = 0x3, },
	{ .reg = S5P_MODIMIF_MEM_LOWPWR			, .val = 0x3, },
	{ .reg = S5P_G2D_ACP_MEM_LOWPWR			, .val = 0x3, },
	{ .reg = S5P_USBOTG_MEM_LOWPWR			, .val = 0x3, },
	{ .reg = S5P_HSMMC_MEM_LOWPWR			, .val = 0x3, },
	{ .reg = S5P_CSSYS_MEM_LOWPWR			, .val = 0x3, },
	{ .reg = S5P_SECSS_MEM_LOWPWR			, .val = 0x3, },
	{ .reg = S5P_PCIE_MEM_LOWPWR			, .val = 0x3, },
	{ .reg = S5P_SATA_MEM_LOWPWR			, .val = 0x3, },
	{ .reg = S5P_PAD_RETENTION_DRAM_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_PAD_RETENTION_MAUDIO_LOWPWR	, .val = 0x1, },
	{ .reg = S5P_PAD_RETENTION_GPIO_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_PAD_RETENTION_UART_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_PAD_RETENTION_MMCA_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_PAD_RETENTION_MMCB_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_PAD_RETENTION_EBIA_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_PAD_RETENTION_EBIB_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_PAD_RETENTION_ISOLATION_LOWPWR	, .val = 0x1, },
	{ .reg = S5P_PAD_RETENTION_ALV_SEL_LOWPWR	, .val = 0x1, },
	{ .reg = S5P_XUSBXTI_LOWPWR			, .val = 0x1, },
	{ .reg = S5P_XXTI_LOWPWR			, .val = 0x1, },
	{ .reg = S5P_EXT_REGULATOR_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_GPIO_MODE_LOWPWR			, .val = 0x1, },
	{ .reg = S5P_GPIO_MODE_MAUDIO_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_CAM_LOWPWR				, .val = 0x7, },
	{ .reg = S5P_TV_LOWPWR				, .val = 0x7, },
	{ .reg = S5P_MFC_LOWPWR				, .val = 0x7, },
	{ .reg = S5P_G3D_LOWPWR				, .val = 0x7, },
	{ .reg = S5P_LCD0_LOWPWR			, .val = 0x7, },
	{ .reg = S5P_LCD1_LOWPWR			, .val = 0x7, },
	{ .reg = S5P_MAUDIO_LOWPWR			, .val = 0x7, },
	{ .reg = S5P_GPS_LOWPWR				, .val = 0x7, },
	{ .reg = S5P_GPS_ALIVE_LOWPWR			, .val = 0x7, },
};

static struct sleep_save s5pv310_lpa[] = {
	{ .reg = S5P_ARM_CORE0_LOWPWR			, .val = 0x0, },
	{ .reg = S5P_DIS_IRQ_CORE0			, .val = 0x0, },
	{ .reg = S5P_DIS_IRQ_CENTRAL0			, .val = 0x0, },
	{ .reg = S5P_ARM_CORE1_LOWPWR			, .val = 0x0, },
	{ .reg = S5P_DIS_IRQ_CORE1			, .val = 0x0, },
	{ .reg = S5P_DIS_IRQ_CENTRAL1			, .val = 0x0, },
	{ .reg = S5P_ARM_COMMON_LOWPWR			, .val = 0x0, },
	{ .reg = S5P_L2_0_LOWPWR			, .val = 0x2, },
	{ .reg = S5P_L2_1_LOWPWR			, .val = 0x2, },
	{ .reg = S5P_CMU_ACLKSTOP_LOWPWR		, .val = 0x0, },
	{ .reg = S5P_CMU_SCLKSTOP_LOWPWR		, .val = 0x0, },
	{ .reg = S5P_CMU_RESET_LOWPWR			, .val = 0x1, },
	{ .reg = S5P_APLL_SYSCLK_LOWPWR			, .val = 0x0, },
	{ .reg = S5P_MPLL_SYSCLK_LOWPWR			, .val = 0x0, },
	{ .reg = S5P_VPLL_SYSCLK_LOWPWR			, .val = 0x0, },
	{ .reg = S5P_EPLL_SYSCLK_LOWPWR			, .val = 0x1, },
	{ .reg = S5P_CMU_CLKSTOP_GPS_ALIVE_LOWPWR	, .val = 0x1, },
	{ .reg = S5P_CMU_RESET_GPSALIVE_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_CMU_CLKSTOP_CAM_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_CMU_CLKSTOP_TV_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_CMU_CLKSTOP_MFC_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_CMU_CLKSTOP_G3D_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_CMU_CLKSTOP_LCD0_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_CMU_CLKSTOP_LCD1_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_CMU_CLKSTOP_MAUDIO_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_CMU_CLKSTOP_GPS_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_CMU_RESET_CAM_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_CMU_RESET_TV_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_CMU_RESET_MFC_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_CMU_RESET_G3D_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_CMU_RESET_LCD0_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_CMU_RESET_LCD1_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_CMU_RESET_MAUDIO_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_CMU_RESET_GPS_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_TOP_BUS_LOWPWR			, .val = 0x0, },
#ifdef CONFIG_CPU_S5PV310_EVT1
	{ .reg = S5P_TOP_RETENTION_LOWPWR		, .val = 0x0, },
	{ .reg = S5P_TOP_PWR_LOWPWR			, .val = 0x0, },
#else
	{ .reg = S5P_TOP_RETENTION_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_TOP_PWR_LOWPWR			, .val = 0x3, },
#endif
	{ .reg = S5P_LOGIC_RESET_LOWPWR			, .val = 0x1, },
	{ .reg = S5P_ONENAND_MEM_LOWPWR			, .val = 0x0, },
	{ .reg = S5P_MODIMIF_MEM_LOWPWR			, .val = 0x0, },
	{ .reg = S5P_G2D_ACP_MEM_LOWPWR			, .val = 0x0, },
	{ .reg = S5P_USBOTG_MEM_LOWPWR			, .val = 0x0, },
	{ .reg = S5P_HSMMC_MEM_LOWPWR			, .val = 0x0, },
	{ .reg = S5P_CSSYS_MEM_LOWPWR			, .val = 0x0, },
	{ .reg = S5P_SECSS_MEM_LOWPWR			, .val = 0x0, },
	{ .reg = S5P_PCIE_MEM_LOWPWR			, .val = 0x0, },
	{ .reg = S5P_SATA_MEM_LOWPWR			, .val = 0x0, },
	{ .reg = S5P_PAD_RETENTION_DRAM_LOWPWR		, .val = 0x0, },
	{ .reg = S5P_PAD_RETENTION_MAUDIO_LOWPWR	, .val = 0x1, },
	{ .reg = S5P_PAD_RETENTION_GPIO_LOWPWR		, .val = 0x0, },
	{ .reg = S5P_PAD_RETENTION_UART_LOWPWR		, .val = 0x0, },
	{ .reg = S5P_PAD_RETENTION_MMCA_LOWPWR		, .val = 0x0, },
	{ .reg = S5P_PAD_RETENTION_MMCB_LOWPWR		, .val = 0x0, },
	{ .reg = S5P_PAD_RETENTION_EBIA_LOWPWR		, .val = 0x0, },
	{ .reg = S5P_PAD_RETENTION_EBIB_LOWPWR		, .val = 0x0, },
	{ .reg = S5P_PAD_RETENTION_ISOLATION_LOWPWR	, .val = 0x0, },
	{ .reg = S5P_PAD_RETENTION_ALV_SEL_LOWPWR	, .val = 0x0, },
	{ .reg = S5P_XUSBXTI_LOWPWR			, .val = 0x1, },
	{ .reg = S5P_XXTI_LOWPWR			, .val = 0x1, },
	{ .reg = S5P_EXT_REGULATOR_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_GPIO_MODE_LOWPWR			, .val = 0x0, },
	{ .reg = S5P_GPIO_MODE_MAUDIO_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_CAM_LOWPWR				, .val = 0x0, },
	{ .reg = S5P_TV_LOWPWR				, .val = 0x0, },
	{ .reg = S5P_MFC_LOWPWR				, .val = 0x0, },
	{ .reg = S5P_G3D_LOWPWR				, .val = 0x0, },
	{ .reg = S5P_LCD0_LOWPWR			, .val = 0x0, },
	{ .reg = S5P_LCD1_LOWPWR			, .val = 0x0, },
	{ .reg = S5P_MAUDIO_LOWPWR			, .val = 0x7, },
	{ .reg = S5P_GPS_LOWPWR				, .val = 0x0, },
	{ .reg = S5P_GPS_ALIVE_LOWPWR			, .val = 0x0, },
};

static struct sleep_save s5pv310_set_clksrc[] = {
#ifdef CONFIG_CPU_S5PV310_EVT1
	{ .reg = S5P_CLKSRC_MASK_TOP			, .val = 0x00000001, },
	{ .reg = S5P_CLKSRC_MASK_CAM			, .val = 0x11111111, },
	{ .reg = S5P_CLKSRC_MASK_TV			, .val = 0x00000111, },
	{ .reg = S5P_CLKSRC_MASK_LCD0			, .val = 0x00001111, },
	{ .reg = S5P_CLKSRC_MASK_LCD1			, .val = 0x00001111, },
	{ .reg = S5P_CLKSRC_MASK_MAUDIO			, .val = 0x00000001, },
	{ .reg = S5P_CLKSRC_MASK_FSYS			, .val = 0x01011111, },
	{ .reg = S5P_CLKSRC_MASK_PERIL0			, .val = 0x01111111, },
	{ .reg = S5P_CLKSRC_MASK_PERIL1			, .val = 0x01110111, },
	{ .reg = S5P_CLKSRC_MASK_DMC			, .val = 0x00010000, },
#else
	{ .reg = S5P_CLKSRC_DMC				, .val = 0x00010000, },
	{ .reg = S5P_CLKSRC_CAM				, .val = 0x11111111, },
	{ .reg = S5P_CLKSRC_LCD0			, .val = 0x00001111, },
	{ .reg = S5P_CLKSRC_LCD1			, .val = 0x00001111, },
	{ .reg = S5P_CLKSRC_FSYS			, .val = 0x00011111, },
	{ .reg = S5P_CLKSRC_PERIL0			, .val = 0x01111111, },
	{ .reg = S5P_CLKSRC_PERIL1			, .val = 0x01110055, },
	{ .reg = S5P_CLKSRC_MAUDIO			, .val = 0x00000006, },

	{ .reg = S5P_CLKSRC_MASK_TOP			, .val = 0x00000001, },
	{ .reg = S5P_CLKSRC_MASK_CAM			, .val = 0x11111111, },
	{ .reg = S5P_CLKSRC_MASK_TV			, .val = 0x00000111, },
	{ .reg = S5P_CLKSRC_MASK_LCD0			, .val = 0x00001111, },
	{ .reg = S5P_CLKSRC_MASK_LCD1			, .val = 0x00001111, },
	{ .reg = S5P_CLKSRC_MASK_MAUDIO			, .val = 0x00000001, },
	{ .reg = S5P_CLKSRC_MASK_FSYS			, .val = 0x01011111, },
	{ .reg = S5P_CLKSRC_MASK_PERIL0			, .val = 0x01111111, },
	{ .reg = S5P_CLKSRC_MASK_PERIL1			, .val = 0x01110111, },
	{ .reg = S5P_CLKSRC_MASK_DMC			, .val = 0x00010000, },
#endif
};

/*
 * This function is called by s3c_cpu_save() in sleep.S. The contents of
 * the SVC mode stack area and s3c_sleep_save_phys variable should be
 * updated to physical memory until entering AFTR mode. The memory contents
 * are used by s3c_cpu_resume() and resume_with_mmu before enabling L2 cache.
 */
void s5p_aftr_cache_clean(unsigned long stack_addr)
{
	unsigned long tmp;

	/* SVC mode stack area is cleaned from L1 cache */
	clean_dcache_area(stack_addr, 0x100);
	/* The variable is cleaned from L1 cache */
	clean_dcache_area(&s3c_sleep_save_phys, 0x4);

#ifdef CONFIG_OUTER_CACHE
#ifdef L2_FLUSH_ALL_AFTR
	/* Temporally add to avoid abort */
	dsb();
	/* outer_nolock_flush_all() */
	/* use clean_all to reduce power consumption */
	outer_nolock_clean_all();
#else
	dsb();
	/* SVC mode stack area is cleaned from L2 cache */
	tmp = virt_to_phys(stack_addr);
	outer_cache.clean_range(tmp, tmp + 0x100);

	/* The variable is cleaned from L2 cache */
	tmp = virt_to_phys(&s3c_sleep_save_phys);
	outer_cache.clean_range(tmp, tmp + 0x4);
#endif
#endif
}

enum gic_loc {
	INT_GIC,
	EXT_GIC,
	END_GIC,

};

#ifndef CONFIG_USE_EXT_GIC
static void s5pv310_gic_ctrl(enum gic_loc gic, unsigned int ctrl)
{
	if (ctrl > 1 || gic >= END_GIC) {
		printk(KERN_ERR "Invalid input argument: %s, %d\n",
		       __func__, __LINE__);
		return;
	}

	switch (gic) {
	case INT_GIC:
		__raw_writel(ctrl, S5P_VA_GIC_DIST + 0x00);
		__raw_writel(ctrl, S5P_VA_GIC_CPU + 0x00);
		break;
	case EXT_GIC:
		__raw_writel(ctrl, S5P_VA_EXTGIC_DIST + 0x00);
		__raw_writel(ctrl, S5P_VA_EXTGIC_CPU + 0x00);
		break;
	default:
		break;
	}
}
#endif

static int s5pv310_enter_idle(struct cpuidle_device *dev,
			      struct cpuidle_state *state);

static int s5pv310_enter_lowpower(struct cpuidle_device *dev,
			      struct cpuidle_state *state);

static struct cpuidle_state s5pv310_cpuidle_set[] = {
	[0] = {
		.enter			= s5pv310_enter_idle,
		.exit_latency		= 1,
		.target_residency	= 40000,
		.flags			= CPUIDLE_FLAG_TIME_VALID,
		.name			= "IDLE",
		.desc			= "ARM clock gating(WFI)",
	},
	[1] = {
		.enter			= s5pv310_enter_lowpower,
		.exit_latency		= 10,
		.target_residency	= 40000,
		.flags			= CPUIDLE_FLAG_TIME_VALID,
		.name			= "LOW_POWER",
		.desc			= "ARM power down",
	},
};

static DEFINE_PER_CPU(struct cpuidle_device, s5pv310_cpuidle_device);

static struct cpuidle_driver s5pv310_idle_driver = {
	.name		= "s5pv310_idle",
	.owner		= THIS_MODULE,
};

void s5pv310_set_core0_pwroff(void)
{
	unsigned long tmp;

	/*
	 * Setting Central Sequence Register for power down mode
	 */
	tmp = __raw_readl(S5P_CENTRAL_SEQ_CONFIGURATION);
	tmp &= ~(S5P_CENTRAL_LOWPWR_CFG);
	__raw_writel(tmp, S5P_CENTRAL_SEQ_CONFIGURATION);

	/* flush L1 cache all for data sync */
	flush_cache_all();

	cpu_do_idle();
}

static void s5pv310_l2x0_resume(void __iomem *l2x0_base, __u32 aux_val,
							__u32 aux_mask)
{
	unsigned int aux;
	unsigned int cache_id;

	/* Data, TAG Latency is 2cycle */
	__raw_writel(0x110, l2x0_base + L2X0_TAG_LATENCY_CTRL);
	__raw_writel(0x110, l2x0_base + L2X0_DATA_LATENCY_CTRL);

	/* L2 cache Prefetch Control Register setting */
	__raw_writel(0x30000007, l2x0_base + L2X0_PREFETCH_CTRL);

	/* Power control register setting */
	__raw_writel(L2X0_DYNAMIC_CLK_GATING_EN | L2X0_STNDBY_MODE_EN,
		     l2x0_base + L2X0_POWER_CTRL);

	cache_id = readl_relaxed(l2x0_base + L2X0_CACHE_ID);
	aux = readl_relaxed(l2x0_base + L2X0_AUX_CTRL);

	aux &= aux_mask;
	aux |= aux_val;

	/*
	 * Check if l2x0 controller is already enabled.
	 * If you are booting from non-secure mode
	 * accessing the below registers will fault.
	 */
	if (!(readl_relaxed(l2x0_base + L2X0_CTRL) & 1)) {

		/* l2x0 controller is disabled */
		writel_relaxed(aux, l2x0_base + L2X0_AUX_CTRL);

		/* enable L2X0 */
		writel_relaxed(1, l2x0_base + L2X0_CTRL);
	}

}

#ifndef CONFIG_USE_EXT_GIC
static int s5pv310_check_gic_pending(enum gic_loc gic)
{
	unsigned long tmp;

	if (gic >= END_GIC) {
		printk(KERN_ERR "Invalid input argument: %s, %d\n",
		       __func__, __LINE__);
		return 0;
	}
	if (gic  == INT_GIC)
		tmp = __raw_readl(S5P_VA_GIC_CPU + 0x0c);
	else
		tmp = __raw_readl(S5P_VA_EXTGIC_CPU + 0x0c);

	if (tmp != 0x3ff) {
		if (gic  == INT_GIC)
			__raw_writel(tmp, S5P_VA_GIC_CPU + 0x10);
		else
			__raw_writel(tmp, S5P_VA_EXTGIC_CPU + 0x10);

		return 1;
	} else
		return 0;
}
#endif

#define GPIO_OFFSET		0x20
#define GPIO_PUD_OFFSET		0x08
#define GPIO_CON_PDN_OFFSET	0x10
#define GPIO_PUD_PDN_OFFSET	0x14
#define GPIO_END_OFFSET		0x200

static void s5pv310_gpio_conpdn_reg(void)
{
	void __iomem *gpio_base = S5P_VA_GPIO;
	unsigned int val;

	do {
		/* Keep the previous state in didle mode */
		__raw_writel(0xffff, gpio_base + GPIO_CON_PDN_OFFSET);

		/* Pull up-down state in didle is same as normal */
		val = __raw_readl(gpio_base + GPIO_PUD_OFFSET);
		__raw_writel(val, gpio_base + GPIO_PUD_PDN_OFFSET);

		gpio_base += GPIO_OFFSET;

		if (gpio_base == S5P_VA_GPIO + GPIO_END_OFFSET)
			gpio_base = S5PV310_VA_GPIO2;

	} while (gpio_base <= S5PV310_VA_GPIO2 + GPIO_END_OFFSET);

	/* set the GPZ */
	gpio_base = S5PV310_VA_GPIO3;
	__raw_writel(0xffff, gpio_base + GPIO_CON_PDN_OFFSET);

	val = __raw_readl(gpio_base + GPIO_PUD_OFFSET);
	__raw_writel(val, gpio_base + GPIO_PUD_PDN_OFFSET);
}

static void s5pv310_set_wakeupmask(void)
{
	__raw_writel(0x0000ff3e, S5P_WAKEUP_MASK);
}

static int s5pv310_enter_core0_aftr(struct cpuidle_device *dev,
				    struct cpuidle_state *state)
{
	struct timeval before, after;
	int idle_time;
	unsigned long tmp;

	pr_info("++%s\n", __func__);
#ifdef AFTR_DEBUG
	/* ON */
	gpio_set_value(S5PV310_GPX1(6), 0);
	gpio_set_value(S5PV310_GPX1(7), 0);
#endif

	s3c_pm_do_save(s5pv310_aftr_save, ARRAY_SIZE(s5pv310_aftr_save));

	s3c_sleep_save_phys = phy_regs_save;

	local_irq_disable();
	do_gettimeofday(&before);

#ifdef CONFIG_CPU_S5PV310_EVT1
	s5pv310_set_wakeupmask();
#else
	/* disable wakeup event monitoring logic */
	__raw_writel(0x0, S5P_WAKEUP_MASK);

	/*
	 * Unmasking all wakeup source and enable
	 * wakeup event monitoring logic
	 */
	__raw_writel(0x80000000, S5P_WAKEUP_MASK);
#endif

#ifndef CONFIG_USE_EXT_GIC
	if (s5pv310_check_gic_pending(INT_GIC))
		goto early_wakeup;

#ifdef CONFIG_CPU_S5PV310_EVT1
	remap_ext_gic();
	s5pv310_gic_ctrl(INT_GIC, 0);
	s5pv310_gic_ctrl(EXT_GIC, 1);
#else
	/*
	 * Disable internal GIC to avoid unexpected early wakeup condition.
	 */
	s5pv310_gic_ctrl(INT_GIC, 0);

	remap_ext_gic();
#endif
#endif
	/* ensure at least INFORM0 has the resume address */
	__raw_writel(virt_to_phys(s3c_cpu_resume), S5P_INFORM0);
#ifdef CONFIG_CPU_S5PV310_EVT1
	__raw_writel(virt_to_phys(s3c_cpu_resume), REG_DIRECTGO_ADDR);
	__raw_writel(0xfcba0d10, REG_DIRECTGO_FLAG);

	/* Set value of power down register for aftr mode */
	s3c_pm_do_restore(s5pv310_aftr, ARRAY_SIZE(s5pv310_aftr));

	__raw_writel(S5P_CHECK_DIDLE, S5P_INFORM1);
#else
	tmp = __raw_readl(S5P_INFORM1);

	if (tmp != S5P_CHECK_DIDLE) {
		/* Set value of power down register for aftr mode */
		s3c_pm_do_restore(s5pv310_aftr, ARRAY_SIZE(s5pv310_aftr));
		__raw_writel(S5P_CHECK_DIDLE, S5P_INFORM1);
	}
#endif
	if (s3c_cpu_save(regs_save) == 0) {

		/*
		 * Clear Central Sequence Register in exiting early wakeup
		 */
		tmp = __raw_readl(S5P_CENTRAL_SEQ_CONFIGURATION);
		tmp |= (S5P_CENTRAL_LOWPWR_CFG);
		__raw_writel(tmp, S5P_CENTRAL_SEQ_CONFIGURATION);
#ifndef CONFIG_USE_EXT_GIC
		/* Clear Ext GIC interrupt pending */
		tmp = s5pv310_check_gic_pending(EXT_GIC);

		/* Disable external GIC */
		s5pv310_gic_ctrl(EXT_GIC, 0);

		/* Enable internal GIC */
		s5pv310_gic_ctrl(INT_GIC, 1);
#endif
		goto early_wakeup;
	}
	flush_cache_all();

	tmp = __raw_readl(S5P_VA_SCU);
	tmp |= ((0x1 << 5)|(0x1 << 0));
	__raw_writel(tmp, S5P_VA_SCU);

#ifdef CONFIG_OUTER_CACHE
	s5pv310_l2x0_resume(S5P_VA_L2CC, 0x7C470001, 0xC200ffff);
#endif
	cpu_init();
#ifndef CONFIG_USE_EXT_GIC
	/* Disable external GIC */
	s5pv310_gic_ctrl(EXT_GIC, 0);
#endif
	s3c_pm_do_restore_core(s5pv310_aftr_save,
				       ARRAY_SIZE(s5pv310_aftr_save));
early_wakeup:

	/* Clear wakeup state register */
	__raw_writel(0x0, S5P_WAKEUP_STAT);
#ifdef AFTR_DEBUG
	/* OFF */
	gpio_set_value(S5PV310_GPX1(7), 1);
#endif
	do_gettimeofday(&after);

	local_irq_enable();
	idle_time = (after.tv_sec - before.tv_sec) * USEC_PER_SEC +
		    (after.tv_usec - before.tv_usec);

	pr_info("--%s\n", __func__);
	return idle_time;
}

static void s5pv310_check_enter(void)
{
	unsigned int check = 0;
	unsigned int val;

	/* Check UART for console is empty */
	val = __raw_readl(S5P_VA_UART(CONFIG_S3C_LOWLEVEL_UART_PORT) + 0x18);

	while (check)
		check = ((val >> 16) & 0xff);
}

extern void bt_uart_rts_ctrl(int flag);

static int s5pv310_enter_core0_lpa(struct cpuidle_device *dev,
				   struct cpuidle_state *state)
{
	struct timeval before, after;
	int idle_time;
	unsigned long tmp;

	pr_info("++%s\n", __func__);
#ifdef AFTR_DEBUG
	/* ON */
	gpio_set_value(S5PV310_GPX1(6), 0);
	gpio_set_value(S5PV310_GPX1(7), 0);
#endif
#ifdef CONFIG_SAMSUNG_PHONE_TTY
	gpio_set_value(GPIO_PDA_ACTIVE, 0);
#endif

	s3c_pm_do_save(s5pv310_lpa_save, ARRAY_SIZE(s5pv310_lpa_save));

	/*
	 * Before enter central sequence mode, clock src register have to set
	 */
	s3c_pm_do_restore_core(s5pv310_set_clksrc,
				ARRAY_SIZE(s5pv310_set_clksrc));

	s3c_sleep_save_phys = phy_regs_save;

#ifdef CONFIG_RFKILL
	/* BT-UART RTS Control (RTS High) */
	bt_uart_rts_ctrl(1);
#endif

	local_irq_disable();
	do_gettimeofday(&before);

	/*
	 * Unmasking all wakeup source.
	 */
	__raw_writel(0x0, S5P_WAKEUP_MASK);

	/* Configure GPIO Power down control register */
	s5pv310_gpio_conpdn_reg();

	/* ensure at least INFORM0 has the resume address */
	__raw_writel(virt_to_phys(s3c_cpu_resume), S5P_INFORM0);

#ifdef CONFIG_CPU_S5PV310_EVT1
	__raw_writel(virt_to_phys(s3c_cpu_resume), REG_DIRECTGO_ADDR);
	__raw_writel(0xfcba0d10, REG_DIRECTGO_FLAG);

	__raw_writel(S5P_CHECK_LPA, S5P_INFORM1);
	s3c_pm_do_restore(s5pv310_lpa, ARRAY_SIZE(s5pv310_lpa));

	__raw_writel(0xfcba0d10, S5P_VA_SYSRAM + 0x20);

#else
	tmp = __raw_readl(S5P_INFORM1);

	if (tmp != S5P_CHECK_LPA) {
		s3c_pm_do_restore(s5pv310_lpa, ARRAY_SIZE(s5pv310_lpa));
		__raw_writel(S5P_CHECK_LPA, S5P_INFORM1);
	}
#endif

	s5pv310_check_enter();

	if (s3c_cpu_save(regs_save) == 0) {

		tmp = __raw_readl(S5P_CENTRAL_SEQ_CONFIGURATION);
		tmp |= (S5P_CENTRAL_LOWPWR_CFG);
		__raw_writel(tmp, S5P_CENTRAL_SEQ_CONFIGURATION);

#ifdef AFTR_DEBUG
		gpio_set_value(S5PV310_GPX1(6), 1);
#endif
		goto early_wakeup;
	}

	flush_cache_all();

	tmp = __raw_readl(S5P_VA_SCU);
	tmp |= ((0x1 << 5)|(0x1 << 0));
	__raw_writel(tmp, S5P_VA_SCU);

#ifdef CONFIG_OUTER_CACHE
	s5pv310_l2x0_resume(S5P_VA_L2CC, 0x7C470001, 0xC200ffff);
#endif
	cpu_init();

	s3c_pm_do_restore_core(s5pv310_lpa_save,
			       ARRAY_SIZE(s5pv310_lpa_save));

	/* For release retention */
	__raw_writel((1 << 28), S5P_PAD_RET_GPIO_OPTION);
	__raw_writel((1 << 28), S5P_PAD_RET_UART_OPTION);
	__raw_writel((1 << 28), S5P_PAD_RET_MMCA_OPTION);
	__raw_writel((1 << 28), S5P_PAD_RET_MMCB_OPTION);
	__raw_writel((1 << 28), S5P_PAD_RET_EBIA_OPTION);
	__raw_writel((1 << 28), S5P_PAD_RET_EBIB_OPTION);

early_wakeup:

	/* Clear wakeup state register */
	__raw_writel(0x0, S5P_WAKEUP_STAT);

	__raw_writel(0x0, S5P_WAKEUP_MASK);

	do_gettimeofday(&after);

#ifdef AFTR_DEBUG
	/* OFF */
	gpio_set_value(S5PV310_GPX1(7), 1);
#endif
#ifdef CONFIG_SAMSUNG_PHONE_TTY
	gpio_set_value(GPIO_PDA_ACTIVE, 1);
#endif

	local_irq_enable();
	idle_time = (after.tv_sec - before.tv_sec) * USEC_PER_SEC +
		    (after.tv_usec - before.tv_usec);

#ifdef CONFIG_RFKILL
	/* BT-UART RTS Control (RTS Low) */
	bt_uart_rts_ctrl(0);
#endif

	pr_info("--%s\n", __func__);
	return idle_time;
}

static int s5pv310_enter_idle(struct cpuidle_device *dev,
			      struct cpuidle_state *state)
{
	struct timeval before, after;
	int idle_time;
	int cpu;
	unsigned int tmp, nr_cores;

	/* ON */
#ifdef AFTR_DEBUG
	gpio_set_value(S5PV310_GPX1(5), 0);
#endif
	local_irq_disable();
	do_gettimeofday(&before);

	cpu = get_cpu();

	spin_lock(&idle_lock);
	cpu_core |= (1 << cpu);

	if (cpu_online(1) == 1)
	    nr_cores = 0x3;
	else
	    nr_cores = 0x1;

	if (cpu_core == nr_cores) {
		old_div = __raw_readl(S5P_CLKDIV_CPU);
		tmp = old_div;
		tmp |= ((0x7 << 28) | (0x7 << 0));
		__raw_writel(tmp , S5P_CLKDIV_CPU);
		do {
			tmp = __raw_readl(S5P_CLKDIV_STATCPU);
		} while (tmp & 0x10000001);
	}

	spin_unlock(&idle_lock);

	cpu_do_idle();

	spin_lock(&idle_lock);

	if (cpu_core == nr_cores) {
		__raw_writel(old_div, S5P_CLKDIV_CPU);
		do {
			tmp = __raw_readl(S5P_CLKDIV_STATCPU);
		} while (tmp & 0x10000001);
	}

	cpu_core &= ~(1 << cpu);
	spin_unlock(&idle_lock);

	put_cpu();

	do_gettimeofday(&after);
	local_irq_enable();
	idle_time = (after.tv_sec - before.tv_sec) * USEC_PER_SEC +
		    (after.tv_usec - before.tv_usec);

	/* OFF */
#ifdef AFTR_DEBUG
	gpio_set_value(S5PV310_GPX1(5), 1);
#endif
	return idle_time;
}

static int check_power_domain(void)
{
	unsigned long tmp;

	tmp = __raw_readl(S5P_PMU_LCD0_CONF);
	if ((tmp & S5P_INT_LOCAL_PWR_EN) == S5P_INT_LOCAL_PWR_EN)
		return 1;

	tmp = __raw_readl(S5P_PMU_MFC_CONF);
	if ((tmp & S5P_INT_LOCAL_PWR_EN) == S5P_INT_LOCAL_PWR_EN)
		return 1;

	tmp = __raw_readl(S5P_PMU_G3D_CONF);
	if ((tmp & S5P_INT_LOCAL_PWR_EN) == S5P_INT_LOCAL_PWR_EN)
		return 1;

	tmp = __raw_readl(S5P_PMU_CAM_CONF);
	if ((tmp & S5P_INT_LOCAL_PWR_EN) == S5P_INT_LOCAL_PWR_EN)
		return 1;

	tmp = __raw_readl(S5P_PMU_TV_CONF);
	if ((tmp & S5P_INT_LOCAL_PWR_EN) == S5P_INT_LOCAL_PWR_EN)
		return 1;

	tmp = __raw_readl(S5P_PMU_GPS_CONF);
	if ((tmp & S5P_INT_LOCAL_PWR_EN) == S5P_INT_LOCAL_PWR_EN)
		return 1;

	return 0;
}

static int check_clock_gating(void)
{
	unsigned long tmp;

	tmp = __raw_readl(S5P_CLKGATE_IP_IMAGE);
	if (tmp & (S5P_CLKGATE_IP_IMAGE_MDMA | S5P_CLKGATE_IP_IMAGE_SMMUMDMA
		| S5P_CLKGATE_IP_IMAGE_QEMDMA))
		return 1;

	tmp = __raw_readl(S5P_CLKGATE_IP_FSYS);
	if (tmp & (S5P_CLKGATE_IP_FSYS_PDMA0 | S5P_CLKGATE_IP_FSYS_PDMA1))
		return 1;

	tmp = __raw_readl(S5P_CLKGATE_IP_PERIL);
	if (tmp & S5P_CLKGATE_IP_PERIL_I2C0_8)
		return 1;

	return 0;
}

#ifdef CONFIG_MTD_ONENAND
static int check_onenand_op(void)
{
	unsigned int val;
	int ret = 0;

	val = __raw_readl(onenandctl_vbase + 0x10C);
	if (val & 0x1) {
		printk(KERN_ERR "onenand IF status = %x", val);
		return 1;
	}

	val = __raw_readl(onenandctl_vbase + 0x41C);
	if (val & 0x70000) {
		printk(KERN_ERR "onenand dma status = %x\n", val);
		ret = 1;
	}

	val = __raw_readl(onenandctl_vbase + 0x1064);
	if (val & 0x1010000) {
		printk(KERN_ERR "onenand intc dma status = %x\n", val);
		ret = 1;
	}

	return ret;
}
#endif

static int sdmmc_dev_num;
/* If SD/MMC interface is working: return = 1 or not 0 */
static int check_sdmmc_op(unsigned int ch)
{
	unsigned int reg1, reg2;
	void __iomem *base_addr;

	if (unlikely(ch > sdmmc_dev_num)) {
		printk(KERN_ERR "Invalid ch[%d] for SD/MMC\n", ch);
		return 0;
	}

	if (chk_dev_op[ch].type == HC_SDHC) {
		base_addr = chk_dev_op[ch].base;
		/* Check CMDINHDAT[1] and CMDINHCMD [0] */
		reg1 = readl(base_addr + S3C_HSMMC_PRNSTS);
		/* Check CLKCON [2]: ENSDCLK */
		reg2 = readl(base_addr + S3C_HSMMC_CLKCON);

		return !!(reg1 & (S3C_HSMMC_CMD_INHIBIT | S3C_HSMMC_DATA_INHIBIT)) ||
		       (reg2 & (S3C_HSMMC_CLOCK_CARD_EN));
	} else if (chk_dev_op[ch].type == HC_MSHC) {
		base_addr = chk_dev_op[ch].base;
		/* Check STATUS [9] for data busy */
		reg1 = readl(base_addr + MSHCI_STATUS);
		/* Check CLKENA [0] for clock on/off */
		reg2 = readl(base_addr + MSHCI_CLKENA);

		return (reg1 & (MSHCI_DATA_BUSY)) ||
		       (reg2 & (MSHCI_ENCLK));

	}
	/* should not be here */
	return 0;
}

/* Check all sdmmc controller */
static int loop_sdmmc_check(void)
{
	unsigned int iter;

	for (iter = 0; iter < sdmmc_dev_num + 1; iter++) {
		if (check_sdmmc_op(iter))
			return 1;
	}
	return 0;
}

/*
 * Check USBOTG is working or not
 * GOTGCTL(0xEC000000)
 * BSesVld (Indicates the Device mode transceiver status)
 * BSesVld =	1b : B-session is valid
 *		0b : B-session is not valid
 */
static int check_usbotg_op(void)
{
	unsigned int val;

	val = __raw_readl(S3C_UDC_OTG_GOTGCTL);

	return val & (A_SESSION_VALID | B_SESSION_VALID);
}

#ifdef CONFIG_USB_EHCI_HCD
static int check_usb_host_op(void)
{
	extern int is_usb_host_phy_suspend(void);

	if (is_usb_host_phy_suspend())
		return 0;

	return 1;
}
#endif

#ifdef CONFIG_SND_S5P_RP
extern int s5p_rp_get_op_level(void);	/* By srp driver */
extern volatile int s5p_rp_is_running;
#endif

#ifdef CONFIG_RFKILL
extern volatile int bt_is_running;
#endif

#ifdef CONFIG_SAMSUNG_PHONE_TTY
static int is_dpram_in_use(void)
{
	/* This pin is high when CP might be accessing dpram */
	/* return !!gpio_get_value(GPIO_CP_DUMP_INT); */
	int x1_2 = __raw_readl(S5PV310_VA_GPIO2 + 0xC24) & 4; /* GPX1(2) */
	pr_err("%s x1_2 is %s\n", __func__, x1_2 ? "high" : "low");
	return x1_2;
}
#endif

static int s5pv310_check_operation(void)
{
	if (check_power_domain())
		return 1;

	if (check_clock_gating())
		return 1;

#ifdef CONFIG_MTD_ONENAND
	if (check_onenand_op())
		return 1;
#endif

	if (loop_sdmmc_check() || check_usbotg_op())
		return 1;

#ifdef CONFIG_SND_S5P_RP
	if (s5p_rp_get_op_level())
		return 1;
#endif

	if (!s5p_rp_is_running)
		return 1;

#ifdef CONFIG_USB_EHCI_HCD
	if (check_usb_host_op())
		return 1;
#endif

#ifdef CONFIG_RFKILL
	if (bt_is_running)
		return 1;
#endif

#ifdef CONFIG_SAMSUNG_PHONE_TTY
	if (is_dpram_in_use())
		return 1;
#endif

	return 0;
}

static int s5pv310_enter_lowpower(struct cpuidle_device *dev,
				  struct cpuidle_state *state)
{
	struct cpuidle_state *new_state = state;

	/* This mode only can be entered when Core1 is offline */
	if (cpu_online(1)) {
		BUG_ON(!dev->safe_state);
		new_state = dev->safe_state;
	}
	dev->last_state = new_state;

	if (new_state == &dev->states[0])
		return s5pv310_enter_idle(dev, new_state);

	if (s5pv310_check_operation())
		return (enable_mask & ENABLE_AFTR)
			? s5pv310_enter_core0_aftr(dev, new_state)
			: s5pv310_enter_idle(dev, new_state);

	return (enable_mask & ENABLE_LPA)
		? s5pv310_enter_core0_lpa(dev, new_state)
		: s5pv310_enter_idle(dev, new_state);
}

static int s5pv310_init_cpuidle(void)
{
	int i, max_cpuidle_state, cpu_id, ret;
	struct cpuidle_device *device;
	struct platform_device *pdev;
	struct resource *res;
#ifdef AFTR_DEBUG
	int err;
#endif

#ifdef CONFIG_MTD_ONENAND
	static phys_addr_t onenand_pbase;
	static int onenand_psize;
#endif

	cpuidle_register_driver(&s5pv310_idle_driver);

	for_each_cpu(cpu_id, cpu_online_mask) {
		device = &per_cpu(s5pv310_cpuidle_device, cpu_id);
		device->cpu = cpu_id;

		if (cpu_id == 0)
			device->state_count = (sizeof(s5pv310_cpuidle_set) /
					       sizeof(struct cpuidle_state));
		else
			device->state_count = 1;	/* Support IDLE only */

		max_cpuidle_state = device->state_count;

		for (i = 0; i < max_cpuidle_state; i++) {
			memcpy(&device->states[i], &s5pv310_cpuidle_set[i],
					sizeof(struct cpuidle_state));
		}

		device->safe_state = &device->states[0];

		if (cpuidle_register_device(device)) {
			printk(KERN_ERR "CPUidle register device failed\n,");
			return -EIO;
		}
	}

	regs_save = dma_alloc_coherent(NULL, 4096, &phy_regs_save, GFP_KERNEL);
	if (regs_save == NULL) {
		printk(KERN_ERR "Alloc memory for CPUIDLE failed\n");
		return -1;
	}

#ifdef AFTR_DEBUG
	/*err = gpio_request(S5PV310_GPX1(4), "LED4");
	if (err)
		printk(KERN_ERR "failed to request LED4\n");

	gpio_direction_output(S5PV310_GPX1(4), 1);
	gpio_set_value(S5PV310_GPX1(4), 1);
	*/
	err = gpio_request(S5PV310_GPX1(5), "LED5");
	if (err)
		printk(KERN_ERR "failed to request LED5\n");

	gpio_direction_output(S5PV310_GPX1(5), 1);
	gpio_set_value(S5PV310_GPX1(5), 1);

	err = gpio_request(S5PV310_GPX1(6), "LED6");
	if (err)
		printk(KERN_ERR "failed to request LED6\n");

	gpio_direction_output(S5PV310_GPX1(6), 1);
	gpio_set_value(S5PV310_GPX1(6), 1);

	err = gpio_request(S5PV310_GPX1(7), "LED7");
	if (err)
		printk(KERN_ERR "failed to request LED7\n");

	gpio_direction_output(S5PV310_GPX1(7), 1);
	gpio_set_value(S5PV310_GPX1(7), 1);
#endif
	/* Allocate memory region to access IP's directly */
	for (i = 0 ; i < MAX_CHK_DEV ; i++) {

		pdev = chk_dev_op[i].pdev;

		if (pdev == NULL) {
			sdmmc_dev_num = i - 1;
			break;
		}

		res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		if (!res) {
			printk(KERN_ERR "%s: failed to get io memory region\n",
					__func__);
			ret = -EINVAL;
			goto err_alloc;
		}
		chk_dev_op[i].base = ioremap_nocache(res->start, 4096);

		if (!chk_dev_op[i].base) {
			printk(KERN_ERR "failed to remap io region\n");
			ret = -EINVAL;
			goto err_alloc;
		}
	}
	regs_save = dma_alloc_coherent(NULL, 4096, &phy_regs_save, GFP_KERNEL);
	if (regs_save == NULL) {
		printk(KERN_ERR "Alloc memory for CPUIDLE failed\n");
		return -1;
	}

#ifdef CONFIG_MTD_ONENAND
	onenand_pbase = S5P_PA_ONENAND_CTL;
	onenand_psize = S5P_SZ_ONENAND_CTL;
	onenandctl_vbase = ioremap(onenand_pbase, onenand_psize);
	if (!onenandctl_vbase) {
		printk(KERN_ERR "failed to remap io region\n");
		ret = -EINVAL;
		goto err_alloc;
	}
#endif

#ifndef CONFIG_USE_EXT_GIC
	ext_gic_init();
#endif
	return 0;

err_alloc:
	while (--i >= 0)
		iounmap(chk_dev_op[i].base);

#ifdef CONFIG_MTD_ONENAND
	iounmap(onenandctl_vbase);
#endif
	return ret;
}

device_initcall(s5pv310_init_cpuidle);
