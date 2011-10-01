/* linux/arch/arm/mach-s5pv310/pm.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * S5PV310 - Power Management support
 *
 * Based on arch/arm/mach-s3c2410/pm.c
 * Copyright (c) 2006 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/suspend.h>
#include <linux/io.h>
#include <linux/regulator/machine.h>

#include <plat/cpu.h>
#include <plat/pm.h>
#include <plat/regs-timer.h>

#include <asm/cacheflush.h>
#include <asm/hardware/cache-l2x0.h>

#include <mach/regs-irq.h>
#include <mach/regs-gpio.h>
#include <mach/regs-clock.h>
#include <mach/regs-systimer.h>
#include <mach/pm-core.h>
#include <mach/regs-mem.h>

void (*s3c_config_sleep_gpio_table)(void);

extern int s5p_l2x0_cache_init(void);

static struct sleep_save s5pv310_sleep[] = {
	{ .reg = S5P_ARM_CORE0_LOWPWR			, .val = 0x2, },
	{ .reg = S5P_DIS_IRQ_CORE0			, .val = 0x0, },
	{ .reg = S5P_DIS_IRQ_CENTRAL0			, .val = 0x0, },
	{ .reg = S5P_ARM_CORE1_LOWPWR			, .val = 0x2, },
	{ .reg = S5P_DIS_IRQ_CORE1			, .val = 0x0, },
	{ .reg = S5P_DIS_IRQ_CENTRAL1			, .val = 0x0, },
	{ .reg = S5P_ARM_COMMON_LOWPWR			, .val = 0x2, },
	{ .reg = S5P_L2_0_LOWPWR			, .val = 0x3, },
	{ .reg = S5P_L2_1_LOWPWR			, .val = 0x3, },
	{ .reg = S5P_CMU_ACLKSTOP_LOWPWR		, .val = 0x0, },
	{ .reg = S5P_CMU_SCLKSTOP_LOWPWR		, .val = 0x0, },
	{ .reg = S5P_CMU_RESET_LOWPWR			, .val = 0x0, },
	{ .reg = S5P_APLL_SYSCLK_LOWPWR			, .val = 0x0, },
	{ .reg = S5P_MPLL_SYSCLK_LOWPWR			, .val = 0x0, },
	{ .reg = S5P_VPLL_SYSCLK_LOWPWR			, .val = 0x0, },
	{ .reg = S5P_EPLL_SYSCLK_LOWPWR			, .val = 0x0, },
	{ .reg = S5P_CMU_CLKSTOP_GPS_ALIVE_LOWPWR	, .val = 0x0, },
	{ .reg = S5P_CMU_RESET_GPSALIVE_LOWPWR		, .val = 0x0, },
	{ .reg = S5P_CMU_CLKSTOP_CAM_LOWPWR		, .val = 0x0, },
	{ .reg = S5P_CMU_CLKSTOP_TV_LOWPWR		, .val = 0x0, },
	{ .reg = S5P_CMU_CLKSTOP_MFC_LOWPWR		, .val = 0x0, },
	{ .reg = S5P_CMU_CLKSTOP_G3D_LOWPWR		, .val = 0x0, },
	{ .reg = S5P_CMU_CLKSTOP_LCD0_LOWPWR		, .val = 0x0, },
	{ .reg = S5P_CMU_CLKSTOP_LCD1_LOWPWR		, .val = 0x0, },
	{ .reg = S5P_CMU_CLKSTOP_MAUDIO_LOWPWR		, .val = 0x0, },
	{ .reg = S5P_CMU_CLKSTOP_GPS_LOWPWR		, .val = 0x0, },
	{ .reg = S5P_CMU_RESET_CAM_LOWPWR		, .val = 0x0, },
	{ .reg = S5P_CMU_RESET_TV_LOWPWR		, .val = 0x0, },
	{ .reg = S5P_CMU_RESET_MFC_LOWPWR		, .val = 0x0, },
	{ .reg = S5P_CMU_RESET_G3D_LOWPWR		, .val = 0x0, },
	{ .reg = S5P_CMU_RESET_LCD0_LOWPWR		, .val = 0x0, },
	{ .reg = S5P_CMU_RESET_LCD1_LOWPWR		, .val = 0x0, },
	{ .reg = S5P_CMU_RESET_MAUDIO_LOWPWR		, .val = 0x0, },
	{ .reg = S5P_CMU_RESET_GPS_LOWPWR		, .val = 0x0, },
	{ .reg = S5P_TOP_BUS_LOWPWR			, .val = 0x0, },
	{ .reg = S5P_TOP_RETENTION_LOWPWR		, .val = 0x1, },
	{ .reg = S5P_TOP_PWR_LOWPWR			, .val = 0x3, },
	{ .reg = S5P_LOGIC_RESET_LOWPWR			, .val = 0x0, },
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
	{ .reg = S5P_XXTI_LOWPWR			, .val = 0x0, },
	{ .reg = S5P_EXT_REGULATOR_LOWPWR		, .val = 0x0, },
	{ .reg = S5P_GPIO_MODE_LOWPWR			, .val = 0x0, },
	{ .reg = S5P_GPIO_MODE_MAUDIO_LOWPWR		, .val = 0x0, },
	{ .reg = S5P_CAM_LOWPWR				, .val = 0x0, },
	{ .reg = S5P_TV_LOWPWR				, .val = 0x0, },
	{ .reg = S5P_MFC_LOWPWR				, .val = 0x0, },
	{ .reg = S5P_G3D_LOWPWR				, .val = 0x0, },
	{ .reg = S5P_LCD0_LOWPWR			, .val = 0x0, },
	{ .reg = S5P_LCD1_LOWPWR			, .val = 0x0, },
	{ .reg = S5P_MAUDIO_LOWPWR			, .val = 0x0, },
	{ .reg = S5P_GPS_LOWPWR				, .val = 0x0, },
	{ .reg = S5P_GPS_ALIVE_LOWPWR			, .val = 0x0, },
	{ .reg = S5P_LCD1_LOWPWR			, .val = 0x0, },
	{ .reg = S5P_MAUDIO_LOWPWR			, .val = 0x0, },
	{ .reg = S5P_GPS_LOWPWR				, .val = 0x0, },
	{ .reg = S5P_GPS_ALIVE_LOWPWR			, .val = 0x0, },
};

static struct sleep_save s5pv310_set_clksrc[] = {
#ifndef CONFIG_CPU_S5PV310_EVT1
	{ .reg = S5P_CLKSRC_DMC				, .val = 0x00010000, },
	{ .reg = S5P_CLKSRC_CAM				, .val = 0x11111111, },
	{ .reg = S5P_CLKSRC_LCD0			, .val = 0x00001111, },
	{ .reg = S5P_CLKSRC_LCD1			, .val = 0x00001111, },
	{ .reg = S5P_CLKSRC_FSYS			, .val = 0x00011111, },
	{ .reg = S5P_CLKSRC_PERIL0			, .val = 0x01111111, },
	{ .reg = S5P_CLKSRC_PERIL1			, .val = 0x01110055, },
	{ .reg = S5P_CLKSRC_MAUDIO			, .val = 0x00000006, },
#endif
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
};

static struct sleep_save s5pv310_core_save[] = {
	SAVE_ITEM(S5P_CLKDIV_LEFTBUS),
	SAVE_ITEM(S5P_CLKGATE_IP_LEFTBUS),
	SAVE_ITEM(S5P_CLKDIV_RIGHTBUS),
	SAVE_ITEM(S5P_CLKGATE_IP_RIGHTBUS),
	SAVE_ITEM(S5P_VPLL_CON0),
	SAVE_ITEM(S5P_VPLL_CON1),
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
	SAVE_ITEM(S5P_CLKDIV_TV),
	SAVE_ITEM(S5P_CLKDIV_MFC),
	SAVE_ITEM(S5P_CLKDIV_G3D),
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
	SAVE_ITEM(S5P_CLKGATE_SCLKCAM),
	SAVE_ITEM(S5P_CLKGATE_IP_CAM),
	SAVE_ITEM(S5P_CLKGATE_IP_TV),
	SAVE_ITEM(S5P_CLKGATE_IP_MFC),
	SAVE_ITEM(S5P_CLKGATE_IP_G3D),
	SAVE_ITEM(S5P_CLKGATE_IP_IMAGE),
	SAVE_ITEM(S5P_CLKGATE_IP_LCD0),
	SAVE_ITEM(S5P_CLKGATE_IP_LCD1),
	SAVE_ITEM(S5P_CLKGATE_IP_FSYS),
	SAVE_ITEM(S5P_CLKGATE_IP_GPS),
	SAVE_ITEM(S5P_CLKGATE_IP_PERIL),
	SAVE_ITEM(S5P_CLKGATE_IP_PERIR),
	SAVE_ITEM(S5P_CLKGATE_BLOCK),
	SAVE_ITEM(S5P_CLKSRC_MASK_DMC),
	SAVE_ITEM(S5P_CLKSRC_DMC),
	SAVE_ITEM(S5P_CLKDIV_DMC0),
	SAVE_ITEM(S5P_CLKDIV_DMC1),
	SAVE_ITEM(S5P_CLKGATE_IP_DMC),
	SAVE_ITEM(S5P_CLKSRC_CPU),
	SAVE_ITEM(S5P_CLKDIV_CPU),
	SAVE_ITEM(S5P_CLKGATE_SCLKCPU),
	SAVE_ITEM(S5P_CLKGATE_IP_CPU),
#ifndef CONFIG_CPU_S5PV310_EVT1
	/* System Timer side  */
	SAVE_ITEM(S5P_VA_SYSTIMER + (0x00)),
	SAVE_ITEM(S5P_VA_SYSTIMER + (0x04)),
	SAVE_ITEM(S5P_VA_SYSTIMER + (0x08)),
	SAVE_ITEM(S5P_VA_SYSTIMER + (0x10)),
	SAVE_ITEM(S5P_VA_SYSTIMER + (0x18)),
	SAVE_ITEM(S5P_VA_SYSTIMER + (0x24)),
	SAVE_ITEM(S5P_VA_SYSTIMER + (0x28)),
#endif
#ifndef CONFIG_USE_EXT_GIC
	/* GIC side */
	SAVE_ITEM(S5P_VA_GIC_CPU + 0x000),
	SAVE_ITEM(S5P_VA_GIC_CPU + 0x004),
	SAVE_ITEM(S5P_VA_GIC_CPU + 0x008),
	SAVE_ITEM(S5P_VA_GIC_CPU + 0x00C),
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
#else
	SAVE_ITEM(S5P_VA_EXTGIC_CPU + 0x000),
	SAVE_ITEM(S5P_VA_EXTGIC_CPU + 0x004),
	SAVE_ITEM(S5P_VA_EXTGIC_CPU + 0x008),
	SAVE_ITEM(S5P_VA_EXTGIC_CPU + 0x00C),
	SAVE_ITEM(S5P_VA_EXTGIC_CPU + 0x014),
	SAVE_ITEM(S5P_VA_EXTGIC_CPU + 0x018),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x000),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x004),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x100),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x104),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x108),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x10C),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x110),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x300),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x304),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x308),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x30C),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x310),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x400),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x404),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x408),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x40C),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x410),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x414),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x418),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x41C),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x420),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x424),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x428),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x42C),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x430),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x434),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x438),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x43C),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x440),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x444),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x448),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x44C),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x450),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x454),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x458),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x45C),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x460),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x464),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x468),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x46C),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x470),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x474),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x478),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x47C),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x480),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x484),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x488),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x48C),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x490),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x494),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x498),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x49C),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x800),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x804),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x808),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x80C),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x810),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x814),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x818),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x81C),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x820),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x824),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x828),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x82C),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x830),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x834),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x838),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x83C),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x840),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x844),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x848),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x84C),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x850),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x854),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x858),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x85C),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x860),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x864),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x868),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x86C),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x870),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x874),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x878),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x87C),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x880),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x884),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x888),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x88C),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x890),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x894),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x898),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0x89C),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0xC00),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0xC04),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0xC08),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0xC0C),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0xC10),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0xC14),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0xC18),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0xC1C),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0xC20),
	SAVE_ITEM(S5P_VA_EXTGIC_DIST + 0xC24),
	SAVE_ITEM(S5P_VA_EXTCOMBINER_BASE + 0x000),
	SAVE_ITEM(S5P_VA_EXTCOMBINER_BASE + 0x010),
	SAVE_ITEM(S5P_VA_EXTCOMBINER_BASE + 0x020),
	SAVE_ITEM(S5P_VA_EXTCOMBINER_BASE + 0x030),
#endif
	/* SROM side */
	SAVE_ITEM(S5P_SROM_BW),
	SAVE_ITEM(S5P_SROM_BC0),
	SAVE_ITEM(S5P_SROM_BC1),
	SAVE_ITEM(S5P_SROM_BC2),
	SAVE_ITEM(S5P_SROM_BC3),
	SAVE_ITEM(S5P_VA_GPIO + 0x700),
	SAVE_ITEM(S5P_VA_GPIO + 0x704),
	SAVE_ITEM(S5P_VA_GPIO + 0x708),
	SAVE_ITEM(S5P_VA_GPIO + 0x70C),
	SAVE_ITEM(S5P_VA_GPIO + 0x710),
	SAVE_ITEM(S5P_VA_GPIO + 0x714),
	SAVE_ITEM(S5P_VA_GPIO + 0x718),
	SAVE_ITEM(S5P_VA_GPIO + 0x71C),
	SAVE_ITEM(S5P_VA_GPIO + 0x720),
	SAVE_ITEM(S5P_VA_GPIO + 0x724),
	SAVE_ITEM(S5P_VA_GPIO + 0x728),
	SAVE_ITEM(S5P_VA_GPIO + 0x72C),
	SAVE_ITEM(S5P_VA_GPIO + 0x730),
	SAVE_ITEM(S5P_VA_GPIO + 0x734),
	SAVE_ITEM(S5P_VA_GPIO + 0x738),
	SAVE_ITEM(S5P_VA_GPIO + 0x73C),
	SAVE_ITEM(S5P_VA_GPIO + 0x900),
	SAVE_ITEM(S5P_VA_GPIO + 0x904),
	SAVE_ITEM(S5P_VA_GPIO + 0x908),
	SAVE_ITEM(S5P_VA_GPIO + 0x90C),
	SAVE_ITEM(S5P_VA_GPIO + 0x910),
	SAVE_ITEM(S5P_VA_GPIO + 0x914),
	SAVE_ITEM(S5P_VA_GPIO + 0x918),
	SAVE_ITEM(S5P_VA_GPIO + 0x91C),
	SAVE_ITEM(S5P_VA_GPIO + 0x920),
	SAVE_ITEM(S5P_VA_GPIO + 0x924),
	SAVE_ITEM(S5P_VA_GPIO + 0x928),
	SAVE_ITEM(S5P_VA_GPIO + 0x92C),
	SAVE_ITEM(S5P_VA_GPIO + 0x930),
	SAVE_ITEM(S5P_VA_GPIO + 0x934),
	SAVE_ITEM(S5P_VA_GPIO + 0x938),
	SAVE_ITEM(S5P_VA_GPIO + 0x93C),
	SAVE_ITEM(S5PV310_VA_GPIO2 + 0x700),
	SAVE_ITEM(S5PV310_VA_GPIO2 + 0x704),
	SAVE_ITEM(S5PV310_VA_GPIO2 + 0x708),
	SAVE_ITEM(S5PV310_VA_GPIO2 + 0x70C),
	SAVE_ITEM(S5PV310_VA_GPIO2 + 0x710),
	SAVE_ITEM(S5PV310_VA_GPIO2 + 0x714),
	SAVE_ITEM(S5PV310_VA_GPIO2 + 0x718),
	SAVE_ITEM(S5PV310_VA_GPIO2 + 0x71C),
	SAVE_ITEM(S5PV310_VA_GPIO2 + 0x720),
	SAVE_ITEM(S5PV310_VA_GPIO2 + 0x900),
	SAVE_ITEM(S5PV310_VA_GPIO2 + 0x904),
	SAVE_ITEM(S5PV310_VA_GPIO2 + 0x908),
	SAVE_ITEM(S5PV310_VA_GPIO2 + 0x90C),
	SAVE_ITEM(S5PV310_VA_GPIO2 + 0x910),
	SAVE_ITEM(S5PV310_VA_GPIO2 + 0x914),
	SAVE_ITEM(S5PV310_VA_GPIO2 + 0x918),
	SAVE_ITEM(S5PV310_VA_GPIO2 + 0x91C),
	SAVE_ITEM(S5PV310_VA_GPIO2 + 0x920),
};

void s5pv310_cpu_suspend(void)
{
	unsigned long tmp;

	/* emmc power off delay
	 * 0x10020988 => 0: 300ms, 1: 6ms
	 */
	__raw_writel(1, S5P_PMU(0x0988));

	/*
	 * Before enter central sequence mode, clock src register have to set
	 */
	s3c_pm_do_restore_core(s5pv310_set_clksrc,
				ARRAY_SIZE(s5pv310_set_clksrc));

#ifndef CONFIG_CPU_S5PV310_EVT1
	/*
	 * Setting PMU spare 1 register to inform wakeup from sleep mode
	 */
	tmp = __raw_readl(S5P_PMU_SPARE1);
	tmp |= ((1 << 31) | (1 << 0) | (1 << 4));
	__raw_writel(tmp, S5P_PMU_SPARE1);
#endif

	/*
	 * Setting Central Sequence Register for power down mode
	 */
	tmp = __raw_readl(S5P_CENTRAL_SEQ_CONFIGURATION);
	tmp &= ~(S5P_CENTRAL_LOWPWR_CFG);
	__raw_writel(tmp, S5P_CENTRAL_SEQ_CONFIGURATION);

	/* issue the standby signal into the pm unit. */
	cpu_do_idle();

	tmp = __raw_readl(S5P_CENTRAL_SEQ_CONFIGURATION);
	tmp |= (S5P_CENTRAL_LOWPWR_CFG);
	__raw_writel(tmp, S5P_CENTRAL_SEQ_CONFIGURATION);
}
EXPORT_SYMBOL(s5pv310_cpu_suspend);

static void s5pv310_cpu_prepare(void)
{
	s3c_pm_do_save(s5pv310_core_save, ARRAY_SIZE(s5pv310_core_save));

	/* Set value of power down register for sleep mode */
	s3c_pm_do_restore_core(s5pv310_sleep, ARRAY_SIZE(s5pv310_sleep));
	__raw_writel(S5P_CHECK_SLEEP, S5P_INFORM1);

	/* ensure at least INFORM0 has the resume address */
	__raw_writel(virt_to_phys(s3c_cpu_resume), S5P_INFORM0);
}

static int s5pv310_pm_begin(void)
{
	return 0;
}

static int s5pv310_pm_prepare(void)
{
	int ret = 0;

	if (s3c_config_sleep_gpio_table)
		s3c_config_sleep_gpio_table();

#if defined(CONFIG_REGULATOR)
	ret = regulator_suspend_prepare(PM_SUSPEND_MEM);
#endif

	return ret;
}

static void s5pv310_pm_end(void)
{
}

static int s5pv310_pm_add(struct sys_device *sysdev)
{
	pm_cpu_prep = s5pv310_cpu_prepare;
	pm_cpu_sleep = s5pv310_cpu_suspend;

	pm_begin = s5pv310_pm_begin;
	pm_prepare = s5pv310_pm_prepare;
	pm_end = s5pv310_pm_end;
	return 0;
}

/*
 * This function copy from linux/arch/arm/kernel/smp_scu.c
 */
void s5pv310_scu_enable(void __iomem *scu_base)
{
	u32 scu_ctrl;

	scu_ctrl = __raw_readl(scu_base);
	/* already enabled? */
	if (scu_ctrl & 1)
		return;

	scu_ctrl |= ((0x1 << 5)|(0x1 << 0));
	__raw_writel(scu_ctrl, scu_base);

	/*
	 * Ensure that the data accessed by CPU0 before the SCU was
	 * initialised is visible to the other CPUs.
	 */
	flush_cache_all();
}

static int s5pv310_pm_resume(struct sys_device *dev)
{
	unsigned int tmp;

	/* check either sleep wakeup or early wake */
	tmp = __raw_readl(S5P_WAKEUP_STAT);
	/* clear for next wakeup */
	__raw_writel(0x0, S5P_WAKEUP_STAT);

	if (tmp & (0x1 << 31)) {
		printk(KERN_DEBUG "Wakeup from sleep, 0x%08x\n", tmp);

		/* For release retention */
#ifdef CONFIG_CPU_S5PV310_EVT1
		__raw_writel((1 << 28), S5P_PAD_RET_MAUDIO_OPTION);
#endif
		__raw_writel((1 << 28), S5P_PAD_RET_GPIO_OPTION);
		__raw_writel((1 << 28), S5P_PAD_RET_UART_OPTION);
		__raw_writel((1 << 28), S5P_PAD_RET_MMCA_OPTION);
		__raw_writel((1 << 28), S5P_PAD_RET_MMCB_OPTION);
		__raw_writel((1 << 28), S5P_PAD_RET_EBIA_OPTION);
		__raw_writel((1 << 28), S5P_PAD_RET_EBIB_OPTION);

		s3c_pm_do_restore_core(s5pv310_core_save,
					ARRAY_SIZE(s5pv310_core_save));

#ifndef CONFIG_CPU_S5PV310_EVT1
		tmp = __raw_readl(S5PV310_INT_CSTAT);
		tmp |= (S5PV310_INT_TICK_EN | S5PV310_INT_EN);
		__raw_writel(tmp, S5PV310_INT_CSTAT);
#endif

		/* Clear External Interrupt Pending */
#if 0
		/* Do not clear external interrupt: we lost it */
		__raw_writel(0xFFFFFFFF, S5P_EINT_PEND(0));
		__raw_writel(0xFFFFFFFF, S5P_EINT_PEND(1));
		__raw_writel(0xFFFFFFFF, S5P_EINT_PEND(2));
		__raw_writel(0xFFFFFFFF, S5P_EINT_PEND(3));
#endif
		s5pv310_scu_enable(S5P_VA_SCU);

#ifdef CONFIG_CACHE_L2X0
		s5p_l2x0_cache_init();
#endif
	} else {
		printk(KERN_DEBUG "Early_wake up!. 0x%08x\n", tmp);
		s3c_pm_do_restore_core(s5pv310_core_save,
					ARRAY_SIZE(s5pv310_core_save));
	}

	return 0;
}

static struct sysdev_driver s5pv310_pm_driver = {
	.add		= s5pv310_pm_add,
	.resume		= s5pv310_pm_resume,
};

static __init int s5pv310_pm_drvinit(void)
{
	unsigned int tmp;

	/* All wakeup disable */
	tmp = __raw_readl(S5P_WAKEUP_MASK);
	tmp |= ((0xFF << 8) | (0x1F << 1));
	__raw_writel(tmp, S5P_WAKEUP_MASK);

	/* Disable XXTI pad in system level normal mode */
	__raw_writel(0x0, S5P_XXTI_CONFIGURATION);

	/* Enable ARMCLK down feature with both ARM cores in IDLE mode */
	tmp = __raw_readl(S5P_PWR_CTRL);
	tmp |= ARMCLK_DOWN_ENABLE;
	__raw_writel(tmp, S5P_PWR_CTRL);

	return sysdev_driver_register(&s5pv310_sysclass, &s5pv310_pm_driver);
}
arch_initcall(s5pv310_pm_drvinit);
