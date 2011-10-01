/*
 * Copyright (C) 2010 Samsung Electronics Co. Ltd.
 *	Jaswinder Singh <jassi.brar@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/clk.h>
#include <linux/gfp.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/dma-mapping.h>
#include <linux/ahci_platform.h>
#include <linux/platform_device.h>

#include <mach/irqs.h>
#include <mach/map.h>
#include <mach/regs-clock.h>

#define LANE0		0x200
#define COM_LANE	0xA00

#define SB_CMU(off, val)	__raw_writeb(val, phy_base + (off) * 4)
#define SB_LANE(off, val)	__raw_writeb(val, phy_base + (LANE0 + (off)) * 4)
#define SB_COM_LANE(off, val)	__raw_writeb(val, phy_base + (COM_LANE + (off)) * 4)

#define SATA_PHYCTRL	0x0
#define SATA_LINKCTRL	0x4
#define SATA_PHYSTATUS	0x8
#define SATA_LINKSTATUS	0xc

#define	LINKCTRLb_RST_PMALIVE_N_CTRL	(1 << 8)
#define	LINKCTRLb_RST_RXOOB_N_CTRL	(1 << 7)
#define	LINKCTRLb_RST_RX_N_CTRL		(1 << 6)
#define	LINKCTRLb_RST_TX_N_CTRL		(1 << 5)

#define	PHYCTRLb_RX_DATA_VALID(x)	(x << 27)
#define	PHYCTRLb_SPEED_MODE		(1 << 26)
#define	PHYCTRLb_PHY_CALIBRATED		(1 << 19)
#define	PHYCTRLb_PHY_CMU_RESET_N	(1 << 10)
#define	PHYCTRLb_PHY_LANE_RESET_N	(1 << 9)
#define	PHYCTRLb_PHY_POR_N		(1 << 8)

#define TRGT_CLK	67000000

static void __iomem *phy_base, *phy_ctrl;

/* Voodoo. Don't Ask Can't Tell */
int ahci_phy_init(void __iomem *mmio)
{
	int i;
	u32 val;

	SB_CMU(0, 0x06);
	SB_CMU(2, 0x80);
	SB_CMU(34, 0xA0);
	SB_CMU(35, 0x42);
	SB_CMU(46, 0x04);
	SB_CMU(47, 0x50);
	SB_CMU(48, 0x70);
	SB_CMU(49, 0x02);
	SB_CMU(50, 0x25);
	SB_CMU(51, 0x40);
	SB_CMU(52, 0x01);
	SB_CMU(53, 0x40);
	SB_CMU(97, 0x2E);
	SB_CMU(99, 0x5E);
	SB_CMU(101, 0x42);
	SB_CMU(102, 0xD1);
	SB_CMU(103, 0x20);
	SB_CMU(104, 0x28);
	SB_CMU(105, 0x78);
	SB_CMU(106, 0x04);
	SB_CMU(107, 0xC8);
	SB_CMU(108, 0x06);

	SB_LANE(0, 0x02);
	SB_LANE(5, 0x10);
	SB_LANE(6, 0x84);
	SB_LANE(7, 0x04);
	SB_LANE(8, 0xE0);
	SB_LANE(16, 0x23);

	SB_LANE(19, 0x05);

	SB_LANE(20, 0x30);
	SB_LANE(21, 0x00);
	SB_LANE(23, 0x70);
	SB_LANE(24, 0xF2);
	SB_LANE(25, 0x1E);
	SB_LANE(26, 0x18);
	SB_LANE(27, 0x0D);
	SB_LANE(28, 0x08);
	SB_LANE(80, 0x60);
	SB_LANE(81, 0x0F);

	SB_COM_LANE(1, 0x20);
	SB_COM_LANE(3, 0x40);
	SB_COM_LANE(4, 0x3C);
	SB_COM_LANE(5, 0x7D);
	SB_COM_LANE(6, 0x1D);
	SB_COM_LANE(7, 0xCF);
	SB_COM_LANE(8, 0x05);

	SB_COM_LANE(9, 0x63);
	SB_COM_LANE(10, 0x29);
	SB_COM_LANE(11, 0xC4);
	SB_COM_LANE(12, 0x01);
	SB_COM_LANE(13, 0x03);
	SB_COM_LANE(14, 0x28);
	SB_COM_LANE(15, 0x98);
	SB_COM_LANE(16, 0x19);
	SB_COM_LANE(19, 0x80);
	SB_COM_LANE(20, 0xF0);
	SB_COM_LANE(21, 0xD0);
	SB_COM_LANE(57, 0xA0);
	SB_COM_LANE(58, 0xA0);
	SB_COM_LANE(59, 0xA0);
	SB_COM_LANE(60, 0xA0);
	SB_COM_LANE(61, 0xA0);
	SB_COM_LANE(62, 0xA0);
	SB_COM_LANE(63, 0xA0);
	SB_COM_LANE(64, 0x42);
	SB_COM_LANE(66, 0x80);
	SB_COM_LANE(67, 0x58);
	SB_COM_LANE(69, 0x44);
	SB_COM_LANE(70, 0x5C);
	SB_COM_LANE(71, 0x86);
	SB_COM_LANE(72, 0x8D);
	SB_COM_LANE(73, 0xD0);
	SB_COM_LANE(74, 0x09);
	SB_COM_LANE(75, 0x90);
	SB_COM_LANE(76, 0x07);
	SB_COM_LANE(77, 0x40);
	SB_COM_LANE(81, 0x20);
	SB_COM_LANE(82, 0x32);
	SB_COM_LANE(127, 0xD8);
	SB_COM_LANE(128, 0x1A);
	SB_COM_LANE(129, 0xFF);
	SB_COM_LANE(130, 0x11);
	SB_COM_LANE(131, 0x00);
	SB_COM_LANE(135, 0xF0);
	SB_COM_LANE(135, 0xFF);
	SB_COM_LANE(135, 0xFF);
	SB_COM_LANE(135, 0xFF);
	SB_COM_LANE(135, 0xFF);
	SB_COM_LANE(140, 0x1C);
	SB_COM_LANE(141, 0xC2);
	SB_COM_LANE(142, 0xC3);
	SB_COM_LANE(143, 0x3F);
	SB_COM_LANE(144, 0x0A);
	SB_COM_LANE(150, 0xF8);

	SB_CMU(0, 0x07);

	val = __raw_readl(phy_ctrl +  SATA_PHYCTRL);
	val |= PHYCTRLb_PHY_CMU_RESET_N;
	__raw_writel(val, phy_ctrl +  SATA_PHYCTRL);

	i = 30000;
	while (i--) {
		val = __raw_readl(phy_ctrl + SATA_PHYSTATUS);
		if (val & 0x40000)
			break;
		udelay(100);
		cpu_relax();
	}
	if (!i)
		return -ENODEV;

	SB_COM_LANE(0, 3);

	val = __raw_readl(phy_ctrl +  SATA_PHYCTRL);
	val |= PHYCTRLb_PHY_LANE_RESET_N;
	__raw_writel(val, phy_ctrl +  SATA_PHYCTRL);

	i = 30000;
	while (i--) {
		val = __raw_readl(phy_ctrl + SATA_PHYSTATUS);
		if (val & 0x10000)
			break;
		udelay(100);
		cpu_relax();
	}
	if (!i)
		return -ENODEV;

	val = __raw_readl(phy_ctrl +  SATA_PHYCTRL);
	val |= PHYCTRLb_PHY_CALIBRATED;
	__raw_writel(val, phy_ctrl +  SATA_PHYCTRL);

	return 0;
}

static int s5pv310_ahci_init(struct device *dev, void __iomem *mmio)
{
	struct clk *clk_sata, *clk_sataphy, *clk_sclk_sata;
	u32 val;

	phy_base = ioremap(S5PV310_PA_SATA_PHY, 0x20000);
	if (!phy_base) {
		dev_err(dev, "Unable to ioremap SATA PHY\n");
		return -ENOMEM;
	}

	phy_ctrl = ioremap(S5PV310_PA_SATA_PHYCTRL, 0x100);
	if (!phy_ctrl) {
		dev_err(dev, "Unable to ioremap SATA PHY\n");
		return -ENOMEM;
	}

	clk_sata = clk_get(dev, "sata");
	if (IS_ERR(clk_sata))
		dev_err(dev, "Unable to get SATA clock\n");
	clk_enable(clk_sata);

	clk_sataphy = clk_get(dev, "sataphy");
	if (IS_ERR(clk_sataphy))
		dev_err(dev, "Unable to get SATAPHY clock\n");
	clk_enable(clk_sataphy);

	clk_sclk_sata = clk_get(dev, "sclk_sata");
	if (IS_ERR(clk_sclk_sata))
		dev_err(dev, "Unable to get SCLK_SATA\n");
	clk_enable(clk_sclk_sata);
	clk_set_rate(clk_sclk_sata, TRGT_CLK);

	/* Enable SATA */
	__raw_writel(1, S5PV310_VA_PMU + 0x720);

	/* UnDocumented Reg */
	__raw_writel(3, S5PV310_VA_PMU + 0x11e4);

	__raw_writel(0, phy_ctrl + SATA_LINKCTRL);
	__raw_writel(0, phy_ctrl + SATA_PHYCTRL);
	cpu_relax();
	udelay(10);
	val = LINKCTRLb_RST_PMALIVE_N_CTRL | LINKCTRLb_RST_RXOOB_N_CTRL
			| LINKCTRLb_RST_RX_N_CTRL | LINKCTRLb_RST_TX_N_CTRL;
	__raw_writel(val, phy_ctrl + SATA_LINKCTRL);

	val = PHYCTRLb_RX_DATA_VALID(3) | PHYCTRLb_SPEED_MODE;
	__raw_writel(val, phy_ctrl +  SATA_PHYCTRL);

	val |= PHYCTRLb_PHY_POR_N;
	__raw_writel(val, phy_ctrl +  SATA_PHYCTRL);

	/* Only 1 Port */
	__raw_writel(1, mmio + 0xc/*HOST_PORTS_IMPL*/);

	return ahci_phy_init(mmio);
}

static struct ahci_platform_data s5pv310_ahci_pdata = {
	.init = s5pv310_ahci_init,
};

static struct resource s5pv310_ahci_resource[] = {
	[0] = {
		.start	= S5PV310_PA_AHCI,
		.end	= S5PV310_PA_AHCI + 0x20000 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_SATA,
		.end	= IRQ_SATA,
		.flags	= IORESOURCE_IRQ,
	},
};

static u64 s5pv310_ahci_dmamask = DMA_BIT_MASK(32);

struct platform_device s5pv310_device_sata = {
	.name		= "ahci",
	.id		= 0,
	.resource	= s5pv310_ahci_resource,
	.num_resources	= ARRAY_SIZE(s5pv310_ahci_resource),
	.dev		= {
		.dma_mask		= &s5pv310_ahci_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
		.platform_data		= &s5pv310_ahci_pdata,
	},
};
