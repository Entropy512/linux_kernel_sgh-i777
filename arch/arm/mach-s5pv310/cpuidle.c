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
	ENABLE_IDLE | ENABLE_AFTR | ENABLE_LPA;
module_param_named(enable_mask, enable_mask, uint, 0644);

static unsigned long *regs_save;
static dma_addr_t phy_regs_save;

static unsigned int cpu_core;
static unsigned int old_div;
static DEFINE_SPINLOCK(idle_lock);

#ifdef CONFIG_MTD_ONENAND
static void __iomem *onenandctl_vbase;
#endif

#define REG_DIRECTGO_ADDR	(s5pv310_subrev() == 0 ?\
				(S5P_VA_SYSRAM + 0x24) : S5P_INFORM7)
#define REG_DIRECTGO_FLAG	(s5pv310_subrev() == 0 ?\
				(S5P_VA_SYSRAM + 0x20) : S5P_INFORM6)

#define L2_CLEAN_ALL
#define MAX_CHK_DEV     0xf
#define DEV_NAME_LEN	0xf

static int check_power_domain(void);
static int check_clock_gating(void);
static int loop_sdmmc_check(void);
static int check_usbotg_op(void);
#ifdef CONFIG_USB_EHCI_HCD
static int check_usb_host_op(void);
#endif
#ifdef CONFIG_RFKILL
static int check_bt_op(void);
#endif
#ifdef CONFIG_SND_S5P_RP
static int check_audio_rp_op(void);
#endif
#ifdef CONFIG_MTD_ONENAND
static int check_onenand_op(void);
#endif
#ifdef CONFIG_MACH_C1
static int check_gps_uart_op(void);
#endif /* CONFIG_MACH_C1 */
#ifdef CONFIG_SAMSUNG_LTE
static int check_idpram_op(void);
#endif
static int check_mfc_op(void);

enum op_state {
	NO_OP = 0,
	IS_OP,
};

/*
 * Device list for checking before entering
 * LPA mode
 */
struct check_device_op {
	char name[DEV_NAME_LEN];
	int (*check_operation) (void);
};

/* Array of checking devices list */
struct check_device_op chk_device_op[] = {
	{.name = "powergating",	.check_operation = check_power_domain},
	{.name = "clockgating",	.check_operation = check_clock_gating},
	{.name = "mmc",		.check_operation = loop_sdmmc_check},
	{.name = "usb device",	.check_operation = check_usbotg_op},
#ifdef CONFIG_RFKILL
	{.name = "bluetooth",	.check_operation = check_bt_op},
#endif
#ifdef CONFIG_SND_S5P_RP
	{.name = "audio rp",	.check_operation = check_audio_rp_op},
#endif
#ifdef CONFIG_MTD_ONENAND
	{.name = "onenand",	.check_operation = check_onenand_op},
#endif
#ifdef CONFIG_MACH_C1
	{.name = "gps uart",	.check_operation = check_gps_uart_op},
#endif /* CONFIG_MACH_C1 */
#ifdef CONFIG_SAMSUNG_LTE
	{.name = "idpram",	.check_operation = check_idpram_op},
#endif
#ifdef CONFIG_USB_EHCI_HCD
	/* usb host should be checked at the end */
	{.name = "usb host",	.check_operation = check_usb_host_op},
#endif
	{.name = "",		.check_operation = NULL},
};

enum hc_type {
	HC_SDHC,
	HC_MSHC,
};

struct check_dev_mmc_op {
	void __iomem		*base;
	struct platform_device	*pdev;
	enum hc_type type;
};

/* Array of mmc device list */
static struct check_dev_mmc_op chk_dev_mmc_op[] = {
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

static struct sleep_save s5pv310_lpa_save[] = {
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
};

static struct sleep_save s5pv310_aftr[] = {
	{ .reg = S5P_ARM_CORE0_LOWPWR			, .val = 0x0, },
	{ .reg = S5P_ARM_CORE1_LOWPWR			, .val = 0x0, },
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
	{ .reg = S5P_ARM_CORE1_LOWPWR			, .val = 0x0, },
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
	{ .reg = S5P_TOP_RETENTION_LOWPWR		, .val = 0x0, },
	{ .reg = S5P_TOP_PWR_LOWPWR			, .val = 0x0, },
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

static int s5pv310_enter_idle(struct cpuidle_device *dev,
			      struct cpuidle_state *state);

static int s5pv310_enter_aftr(struct cpuidle_device *dev,
			      struct cpuidle_state *state);

static int s5pv310_enter_lowpower(struct cpuidle_device *dev,
			      struct cpuidle_state *state);

static struct cpuidle_state s5pv310_cpuidle_set[] = {
	[0] = {
		.enter			= s5pv310_enter_idle,
		.exit_latency		= 1,
		.target_residency	= 4000,
		.flags			= CPUIDLE_FLAG_TIME_VALID,
		.name			= "IDLE",
		.desc			= "ARM clock gating(WFI)",
	},
	[1] = {
		.enter			= s5pv310_enter_aftr,
		.exit_latency		= 5,
		.target_residency	= 10000,
		.flags			= CPUIDLE_FLAG_TIME_VALID,
		.name			= "AFTR",
		.desc			= "ARM power down",
	},
	[2] = {
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

/*
 * This function is called by s3c_cpu_save() in sleep.S. The contents of
 * the SVC mode stack area and s3c_sleep_save_phys variable should be
 * updated to physical memory until entering AFTR mode. The memory contents
 * are used by s3c_cpu_resume() and resume_with_mmu before enabling L2 cache.
 */
void s5p_aftr_cache_clean(void *stack_addr)
{
	/* SVC mode stack area is cleaned from L1 cache */
	clean_dcache_area(stack_addr, 0x100);
	/* The variable is cleaned from L1 cache */
	clean_dcache_area(&s3c_sleep_save_phys, 0x4);

#ifdef CONFIG_OUTER_CACHE
#ifdef L2_CLEAN_ALL
	dsb();
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


	s3c_sleep_save_phys = phy_regs_save;

	local_irq_disable();
	do_gettimeofday(&before);

	s5pv310_set_wakeupmask();

	/* ensure at least INFORM0 has the resume address */
	__raw_writel(virt_to_phys(s3c_cpu_resume), S5P_INFORM0);
	__raw_writel(virt_to_phys(s3c_cpu_resume), REG_DIRECTGO_ADDR);
	__raw_writel(0xfcba0d10, REG_DIRECTGO_FLAG);

	/* Set value of power down register for aftr mode */
	s3c_pm_do_restore(s5pv310_aftr, ARRAY_SIZE(s5pv310_aftr));

	__raw_writel(S5P_CHECK_DIDLE, S5P_INFORM1);
	if (s3c_cpu_save(regs_save) == 0) {
		/*
		 * Clear Central Sequence Register in exiting early wakeup
		 */
		tmp = __raw_readl(S5P_CENTRAL_SEQ_CONFIGURATION);
		tmp |= (S5P_CENTRAL_LOWPWR_CFG);
		__raw_writel(tmp, S5P_CENTRAL_SEQ_CONFIGURATION);

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

early_wakeup:

	/* Clear wakeup state register */
	__raw_writel(0x0, S5P_WAKEUP_STAT);

	do_gettimeofday(&after);

	local_irq_enable();
	idle_time = (after.tv_sec - before.tv_sec) * USEC_PER_SEC +
		    (after.tv_usec - before.tv_usec);

	return idle_time;
}

extern void bt_uart_rts_ctrl(int flag);

static int s5pv310_enter_core0_lpa(struct cpuidle_device *dev,
				   struct cpuidle_state *state)
{
	struct timeval before, after;
	int idle_time;
	unsigned long tmp;

	//	pr_info("++%s\n", __func__);
#ifdef CONFIG_SAMSUNG_LTE
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

	__raw_writel(virt_to_phys(s3c_cpu_resume), REG_DIRECTGO_ADDR);
	__raw_writel(0xfcba0d10, REG_DIRECTGO_FLAG);

	__raw_writel(S5P_CHECK_LPA, S5P_INFORM1);
	s3c_pm_do_restore(s5pv310_lpa, ARRAY_SIZE(s5pv310_lpa));

	__raw_writel(0xfcba0d10, S5P_VA_SYSRAM + 0x20);

	if (s3c_cpu_save(regs_save) == 0) {

		tmp = __raw_readl(S5P_CENTRAL_SEQ_CONFIGURATION);
		tmp |= (S5P_CENTRAL_LOWPWR_CFG);
		__raw_writel(tmp, S5P_CENTRAL_SEQ_CONFIGURATION);

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

#ifdef CONFIG_SAMSUNG_LTE
	gpio_set_value(GPIO_PDA_ACTIVE, 1);
#endif

	local_irq_enable();
	idle_time = (after.tv_sec - before.tv_sec) * USEC_PER_SEC +
		    (after.tv_usec - before.tv_usec);

#ifdef CONFIG_RFKILL
	/* BT-UART RTS Control (RTS Low) */
	bt_uart_rts_ctrl(0);
#endif

	//	pr_info("--%s\n", __func__);
	return idle_time;
}

static int s5pv310_enter_idle(struct cpuidle_device *dev,
			      struct cpuidle_state *state)
{
	struct timeval before, after;
	int idle_time;
	int cpu;
	unsigned long flags;
	unsigned int tmp, nr_cores;

	local_irq_disable();
	do_gettimeofday(&before);

	cpu = get_cpu();

	spin_lock_irqsave(&idle_lock, flags);
	cpu_core |= (1 << cpu);

	if ((__raw_readl(S5P_ARM_CORE1_STATUS)
			& S5P_CORE_LOCAL_PWR_EN) == S5P_CORE_LOCAL_PWR_EN)
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
	spin_unlock_irqrestore(&idle_lock, flags);

	cpu_do_idle();

	spin_lock_irqsave(&idle_lock, flags);

	if (cpu_core == nr_cores) {
		__raw_writel(old_div, S5P_CLKDIV_CPU);
		do {
			tmp = __raw_readl(S5P_CLKDIV_STATCPU);
		} while (tmp & 0x10000001);
	}

	cpu_core &= ~(1 << cpu);
	spin_unlock_irqrestore(&idle_lock, flags);

	put_cpu();

	do_gettimeofday(&after);
	local_irq_enable();
	idle_time = (after.tv_sec - before.tv_sec) * USEC_PER_SEC +
		    (after.tv_usec - before.tv_usec);

	return idle_time;
}

static int check_power_domain(void)
{
	unsigned long tmp;

	tmp = __raw_readl(S5P_PMU_LCD0_CONF);
	if ((tmp & S5P_INT_LOCAL_PWR_EN) == S5P_INT_LOCAL_PWR_EN)
		return IS_OP;

	tmp = __raw_readl(S5P_PMU_MFC_CONF);
	if ((tmp & S5P_INT_LOCAL_PWR_EN) == S5P_INT_LOCAL_PWR_EN)
		return IS_OP;

	tmp = __raw_readl(S5P_PMU_G3D_CONF);
	if ((tmp & S5P_INT_LOCAL_PWR_EN) == S5P_INT_LOCAL_PWR_EN)
		return IS_OP;

	tmp = __raw_readl(S5P_PMU_CAM_CONF);
	if ((tmp & S5P_INT_LOCAL_PWR_EN) == S5P_INT_LOCAL_PWR_EN)
		return IS_OP;

	tmp = __raw_readl(S5P_PMU_TV_CONF);
	if ((tmp & S5P_INT_LOCAL_PWR_EN) == S5P_INT_LOCAL_PWR_EN)
		return IS_OP;

	tmp = __raw_readl(S5P_PMU_GPS_CONF);
	if ((tmp & S5P_INT_LOCAL_PWR_EN) == S5P_INT_LOCAL_PWR_EN)
		return IS_OP;

	return NO_OP;
}

static int check_clock_gating(void)
{
	unsigned long tmp;

	tmp = __raw_readl(S5P_CLKGATE_IP_IMAGE);
	if (tmp & (S5P_CLKGATE_IP_IMAGE_MDMA | S5P_CLKGATE_IP_IMAGE_SMMUMDMA
		| S5P_CLKGATE_IP_IMAGE_QEMDMA))
		return IS_OP;

	tmp = __raw_readl(S5P_CLKGATE_IP_FSYS);
	if (tmp & (S5P_CLKGATE_IP_FSYS_PDMA0 | S5P_CLKGATE_IP_FSYS_PDMA1))
		return IS_OP;

	tmp = __raw_readl(S5P_CLKGATE_IP_PERIL);
	if (tmp & S5P_CLKGATE_IP_PERIL_I2C0_8)
		return IS_OP;

	return NO_OP;
}

#ifdef CONFIG_MTD_ONENAND
static int check_onenand_op(void)
{
	unsigned int val;

	val = __raw_readl(onenandctl_vbase + 0x10C);
	if (val & 0x1) {
		printk(KERN_ERR "onenand IF status = %x", val);
		return IS_OP;
	}

	val = __raw_readl(onenandctl_vbase + 0x41C);
	if (val & 0x70000) {
		printk(KERN_ERR "onenand dma status = %x\n", val);
		return IS_OP;
	}

	val = __raw_readl(onenandctl_vbase + 0x1064);
	if (val & 0x1010000) {
		printk(KERN_ERR "onenand intc dma status = %x\n", val);
		return IS_OP;
	}

	return NO_OP;
}
#endif

#ifdef CONFIG_MACH_C1
static int check_gps_uart_op(void)
{
	extern int gps_is_running;

	if (gps_is_running)
		return IS_OP;

	return NO_OP;
}
#endif /* CONFIG_MACH_C1 */

static int check_mfc_op(void)
{
        extern int mfc_is_running;

        if (mfc_is_running)
	        return IS_OP;

	return NO_OP;
}

#ifdef CONFIG_SAMSUNG_LTE
static int check_idpram_op(void)
{
	/* This pin is high when CP might be accessing dpram */
	int cp_int = gpio_get_value(GPIO_CP_AP_DPRAM_INT);
	pr_err("%s cp_int is %s\n", __func__, cp_int ? "high" : "low");
	return cp_int;
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

	if (chk_dev_mmc_op[ch].type == HC_SDHC) {
		base_addr = chk_dev_mmc_op[ch].base;
		/* Check CMDINHDAT[1] and CMDINHCMD [0] */
		reg1 = readl(base_addr + S3C_HSMMC_PRNSTS);
		/* Check CLKCON [2]: ENSDCLK */
		reg2 = readl(base_addr + S3C_HSMMC_CLKCON);

		return (reg1 & (S3C_HSMMC_CMD_INHIBIT | S3C_HSMMC_DATA_INHIBIT))
			|| (reg2 & (S3C_HSMMC_CLOCK_CARD_EN));
	} else if (chk_dev_mmc_op[ch].type == HC_MSHC) {
		base_addr = chk_dev_mmc_op[ch].base;
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
			return IS_OP;
	}
	return NO_OP;
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

	if (val & (A_SESSION_VALID | B_SESSION_VALID))
		return IS_OP;

	return NO_OP;
}

#ifdef CONFIG_USB_EHCI_HCD
static int check_usb_host_op(void)
{
	extern int is_usb_host_phy_suspend(void);

	if (is_usb_host_phy_suspend())
		return NO_OP;

	return IS_OP;
}
#endif

#ifdef CONFIG_SND_S5P_RP
static int check_audio_rp_op(void)
{
	extern int s5p_rp_get_op_level(void);	/* By srp driver */

	if (s5p_rp_get_op_level())
		return IS_OP;

	return NO_OP;
}
#endif

#ifdef CONFIG_RFKILL
static int check_bt_op(void)
{
	extern int bt_is_running;

	if (bt_is_running)
		return IS_OP;

	return NO_OP;
}
#endif

static void check_uart_op(void)
{
	unsigned int check_val = 0;
	unsigned int val;
	unsigned int timeout;

	timeout = 1000;

	do {
		/* Check UART for console is empty */
		val = __raw_readl(S5P_VA_UART(CONFIG_S3C_LOWLEVEL_UART_PORT)
				+ 0x18);
		check_val = ((val >> 16) & 0xff);
		if (timeout == 0)
			break;
		timeout--;
	} while (check_val);

}

static int chk_dev_num;
static int s5pv310_check_operation(void)
{
	int i;

	for (i = 0; i < chk_dev_num; i++) {
		if (chk_device_op[i].check_operation == NULL)
			break;

		if (chk_device_op[i].check_operation() == IS_OP) {
			/* For debugging and optimizing LPA mode */
			/* printk(KERN_DEBUG "%s is operating\n",
					chk_device_op[i].name); */
			return IS_OP;
		}
	}
	/* To prevent broken uart data for console */
	check_uart_op();

	return NO_OP;
}

static int s5pv310_enter_lowpower(struct cpuidle_device *dev,
				  struct cpuidle_state *state)
{
	struct cpuidle_state *new_state = state;

	/* This mode only can be entered when Core1 is offline */
	if ((__raw_readl(S5P_ARM_CORE1_STATUS)
			& S5P_CORE_LOCAL_PWR_EN) == S5P_CORE_LOCAL_PWR_EN) {
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

static int s5pv310_enter_aftr(struct cpuidle_device *dev,
				  struct cpuidle_state *state)
{
	struct cpuidle_state *new_state = state;

	/* This mode only can be entered when Core1 is offline */
	if ((__raw_readl(S5P_ARM_CORE1_STATUS)
			& S5P_CORE_LOCAL_PWR_EN) == S5P_CORE_LOCAL_PWR_EN) {
		BUG_ON(!dev->safe_state);
		new_state = dev->safe_state;
	}
	dev->last_state = new_state;

	if (new_state == &dev->states[0])
		return s5pv310_enter_idle(dev, new_state);

	/*
	 * Lock out AFTR when MFC is active - some people have video decoding issues here
	 */

	if (check_mfc_op())
	        return s5pv310_enter_idle(dev, new_state);

	return (enable_mask & ENABLE_AFTR)
		? s5pv310_enter_core0_aftr(dev, new_state)
		: s5pv310_enter_idle(dev, new_state);
}

static int s5pv310_init_cpuidle(void)
{
	int i, max_cpuidle_state, cpu_id, ret;
	struct cpuidle_device *device;
	struct platform_device *pdev;
	struct resource *res;

#ifdef CONFIG_MTD_ONENAND
	static phys_addr_t onenand_pbase;
	static int onenand_psize;
#endif

	cpuidle_register_driver(&s5pv310_idle_driver);

	for_each_cpu(cpu_id, cpu_online_mask) {
		device = &per_cpu(s5pv310_cpuidle_device, cpu_id);
		device->cpu = cpu_id;

		if (cpu_id == 0)
			device->state_count = ARRAY_SIZE(s5pv310_cpuidle_set);
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

	/* set number of devices which need to check before LPA mode */
	chk_dev_num = ARRAY_SIZE(chk_device_op);

	/* Allocate memory region to access IP's directly */
	for (i = 0 ; i < MAX_CHK_DEV ; i++) {

		pdev = chk_dev_mmc_op[i].pdev;

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
		chk_dev_mmc_op[i].base = ioremap_nocache(res->start, 4096);

		if (!chk_dev_mmc_op[i].base) {
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

	return 0;

err_alloc:
	while (--i >= 0)
		iounmap(chk_dev_mmc_op[i].base);

#ifdef CONFIG_MTD_ONENAND
	iounmap(onenandctl_vbase);
#endif
	return ret;
}

device_initcall(s5pv310_init_cpuidle);
