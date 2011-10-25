/* linux/arch/arm/mach-s5pv210/setup-sdhci.c
 *
 * Copyright (c) 2009-2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S5PV210 - Helper functions for settign up SDHCI device(s) (HSMMC)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/io.h>

#include <linux/mmc/card.h>
#include <linux/mmc/host.h>

#include <plat/mshci.h>

char *s5p6450_mshc_clksrcs[1] = {
	[0] = "sclk_hsmmc44",	/* mmc_bus */
};

void s5p6450_setup_mshci_cfg_card(struct platform_device *dev,
				    void __iomem *r,
				    struct mmc_ios *ios,
				    struct mmc_card *card)
{
/* should be implemented later*/
/*
	u32 ctrl2, ctrl3;

	ctrl2 = __raw_readl(r + S3C_SDHCI_CONTROL2);
	ctrl2 &= S3C_SDHCI_CTRL2_SELBASECLK_MASK;
	ctrl2 |= (S3C64XX_SDHCI_CTRL2_ENSTAASYNCCLR |
		  S3C64XX_SDHCI_CTRL2_ENCMDCNFMSK |
		  S3C_SDHCI_CTRL2_DFCNT_NONE |
		  S3C_SDHCI_CTRL2_ENCLKOUTHOLD);

	if (ios)
		if (ios->clock > 400000)
			ctrl2 |= S3C_SDHCI_CTRL2_ENFBCLKRX;
	ctrl3 = 0;

	__raw_writel(ctrl2, r + S3C_SDHCI_CONTROL2);
	__raw_writel(ctrl3, r + S3C_SDHCI_CONTROL3);
*/
}
