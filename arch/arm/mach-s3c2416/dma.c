/* linux/arch/arm/mach-s3c2416/dma.c
 *
 * Copyright (c) 2007 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * S3C2416 DMA selection
 *
 * http://armlinux.simtec.co.uk/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/io.h>

#include <mach/dma.h>
#include <mach/irqs.h>

#include <plat/dma-s3c24xx.h>
#include <plat/cpu.h>
#include <plat/regs-dma.h>
#include <plat/regs-iis.h>

#define MAP(x) { \
		[0]	= (x) | DMA_CH_VALID,	\
		[1]	= (x) | DMA_CH_VALID,	\
		[2]	= (x) | DMA_CH_VALID,	\
		[3]	= (x) | DMA_CH_VALID,	\
		[4]	= (x) | DMA_CH_VALID,	\
		[5]	= (x) | DMA_CH_VALID,	\
	}


static struct s3c24xx_dma_map __initdata s3c2416_dma_mappings[] = {
	[DMACH_I2S_IN] = {
		.name		= "i2s-sdi",
		.channels	= MAP(S3C2443_DMAREQSEL_I2SRX),
		.hw_addr.from	= S3C2410_PA_IIS + S3C2410_IISFIFO,
	},
	[DMACH_I2S_OUT] = {
		.name		= "i2s-sdo",
		.channels	= MAP(S3C2443_DMAREQSEL_I2STX),
		.hw_addr.to	= S3C2410_PA_IIS + S3C2410_IISFIFO,
	},
};

static void s3c2416_dma_select(struct s3c2410_dma_chan *chan,
				struct s3c24xx_dma_map *map)
{
	writel(map->channels[0] | S3C2443_DMAREQSEL_HW,
		chan->regs + S3C2443_DMA_DMAREQSEL);
}

static struct s3c24xx_dma_selection __initdata s3c2416_dma_sel = {
	.select		= s3c2416_dma_select,
	.dcon_mask	= 0,
	.map		= s3c2416_dma_mappings,
	.map_size	= ARRAY_SIZE(s3c2416_dma_mappings),
};

static int __init s3c2416_dma_add(struct sys_device *sysdev)
{
	s3c24xx_dma_init(6, IRQ_S3C2443_DMA0, 0x100);
	return s3c24xx_dma_init_map(&s3c2416_dma_sel);
}

static struct sysdev_driver s3c2416_dma_driver = {
	.add	= s3c2416_dma_add,
};

static int __init s3c2416_dma_init(void)
{
	return sysdev_driver_register(&s3c2416_sysclass, &s3c2416_dma_driver);
}

arch_initcall(s3c2416_dma_init);
