/* linux/arch/arm/mach-s5pv310/dev-audio.c
 *
 * Copyright (c) 2010 Samsung Electronics Co. Ltd
 *	Jaswinder Singh <jassi.brar@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/platform_device.h>
#include <linux/dma-mapping.h>

#include <plat/gpio-cfg.h>
#include <plat/audio.h>

#include <mach/gpio.h>
#include <mach/map.h>
#include <mach/dma.h>
#include <mach/irqs.h>

static int s5pv310_cfg_i2s(struct platform_device *pdev)
{
	/* configure GPIO for i2s port */
	switch (pdev->id) {
	case 1:
		s3c_gpio_cfgpin(S5PV310_GPC0(0), S3C_GPIO_SFN(2));
#ifndef CONFIG_TDMB
		s3c_gpio_cfgpin(S5PV310_GPC0(1), S3C_GPIO_SFN(2));
#endif
		s3c_gpio_cfgpin(S5PV310_GPC0(2), S3C_GPIO_SFN(2));
		s3c_gpio_cfgpin(S5PV310_GPC0(3), S3C_GPIO_SFN(2));
		s3c_gpio_cfgpin(S5PV310_GPC0(4), S3C_GPIO_SFN(2));
		break;
	case 2:
		s3c_gpio_cfgpin(S5PV310_GPC1(0), S3C_GPIO_SFN(2));
		s3c_gpio_cfgpin(S5PV310_GPC1(1), S3C_GPIO_SFN(2));
		s3c_gpio_cfgpin(S5PV310_GPC1(2), S3C_GPIO_SFN(2));
		s3c_gpio_cfgpin(S5PV310_GPC1(3), S3C_GPIO_SFN(2));
		s3c_gpio_cfgpin(S5PV310_GPC1(4), S3C_GPIO_SFN(2));
		break;
	case -1:
		s3c_gpio_cfgpin(S5PV310_GPZ(0), S3C_GPIO_SFN(2));
		s3c_gpio_cfgpin(S5PV310_GPZ(1), S3C_GPIO_SFN(2));
		s3c_gpio_cfgpin(S5PV310_GPZ(2), S3C_GPIO_SFN(2));
		s3c_gpio_cfgpin(S5PV310_GPZ(3), S3C_GPIO_SFN(2));
		s3c_gpio_cfgpin(S5PV310_GPZ(4), S3C_GPIO_SFN(2));
		s3c_gpio_cfgpin(S5PV310_GPZ(5), S3C_GPIO_SFN(2));
		s3c_gpio_cfgpin(S5PV310_GPZ(6), S3C_GPIO_SFN(2));
		break;

	default:
		printk(KERN_ERR "Invalid Device %d\n", pdev->id);
		return -EINVAL;
	}

	return 0;
}

static struct s3c_audio_pdata s3c_i2s_pdata = {
	.cfg_gpio = s5pv310_cfg_i2s,
};

static struct resource s5pv310_iis0_resource[] = {
	[0] = {
	       .start = S5PV310_PA_I2S0,
	       .end = S5PV310_PA_I2S0 + 0x100 - 1,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = DMACH_I2S0_TX,
	       .end = DMACH_I2S0_TX,
	       .flags = IORESOURCE_DMA,
	       },
	[2] = {
	       .start = DMACH_I2S0_RX,
	       .end = DMACH_I2S0_RX,
	       .flags = IORESOURCE_DMA,
	       },
};

struct platform_device s5pv310_device_iis0 = {
	.name = "s3c64xx-iis-v4",
	.id = -1,
	.num_resources = ARRAY_SIZE(s5pv310_iis0_resource),
	.resource = s5pv310_iis0_resource,
	.dev = {
		.platform_data = &s3c_i2s_pdata,
		},
};

static int s5pv310_pcm_cfg_gpio(struct platform_device *pdev)
{
	switch (pdev->id) {
	case 1:
		s3c_gpio_cfgpin(S5PV310_GPC0(0), S3C_GPIO_SFN(3));
#ifndef CONFIG_TDMB
		s3c_gpio_cfgpin(S5PV310_GPC0(1), S3C_GPIO_SFN(3));
#endif
		s3c_gpio_cfgpin(S5PV310_GPC0(2), S3C_GPIO_SFN(3));
		s3c_gpio_cfgpin(S5PV310_GPC0(3), S3C_GPIO_SFN(3));
		s3c_gpio_cfgpin(S5PV310_GPC0(4), S3C_GPIO_SFN(3));
		break;
	case 2:
		s3c_gpio_cfgpin(S5PV310_GPC1(0), S3C_GPIO_SFN(3));
		s3c_gpio_cfgpin(S5PV310_GPC1(1), S3C_GPIO_SFN(3));
		s3c_gpio_cfgpin(S5PV310_GPC1(2), S3C_GPIO_SFN(3));
		s3c_gpio_cfgpin(S5PV310_GPC1(3), S3C_GPIO_SFN(3));
		s3c_gpio_cfgpin(S5PV310_GPC1(4), S3C_GPIO_SFN(3));
		break;
	case 0:
		s3c_gpio_cfgpin(S5PV310_GPZ(0), S3C_GPIO_SFN(3));
		s3c_gpio_cfgpin(S5PV310_GPZ(1), S3C_GPIO_SFN(3));
		s3c_gpio_cfgpin(S5PV310_GPZ(2), S3C_GPIO_SFN(3));
		s3c_gpio_cfgpin(S5PV310_GPZ(3), S3C_GPIO_SFN(3));
		s3c_gpio_cfgpin(S5PV310_GPZ(4), S3C_GPIO_SFN(3));
		s3c_gpio_cfgpin(S5PV310_GPZ(5), S3C_GPIO_SFN(3));
		s3c_gpio_cfgpin(S5PV310_GPZ(6), S3C_GPIO_SFN(3));
		break;
	default:
		printk(KERN_DEBUG "Invalid PCM Controller number!");
		return -EINVAL;
	}

	return 0;
}

static struct s3c_audio_pdata s3c_pcm_pdata = {
	.cfg_gpio = s5pv310_pcm_cfg_gpio,
};

static struct resource s5pv310_pcm1_resource[] = {
	[0] = {
		.start = S5PV310_PA_PCM1,
		.end   = S5PV310_PA_PCM1 + 0x100 - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = DMACH_PCM1_TX,
		.end   = DMACH_PCM1_TX,
		.flags = IORESOURCE_DMA,
	},
	[2] = {
		.start = DMACH_PCM1_RX,
		.end   = DMACH_PCM1_RX,
		.flags = IORESOURCE_DMA,
	},
};

struct platform_device s5pv310_device_pcm1 = {
	.name		  = "samsung-pcm",
	.id		  = 1,
	.num_resources	  = ARRAY_SIZE(s5pv310_pcm1_resource),
	.resource	  = s5pv310_pcm1_resource,
	.dev = {
		.platform_data = &s3c_pcm_pdata,
	},
};


static int s5pv310_ac97_cfg_gpio(struct platform_device *pdev)
{
	s3c_gpio_cfgpin(S5PV310_GPC0(0), S3C_GPIO_SFN(4));
#ifndef CONFIG_TDMB
	s3c_gpio_cfgpin(S5PV310_GPC0(1), S3C_GPIO_SFN(4));
#endif
	s3c_gpio_cfgpin(S5PV310_GPC0(2), S3C_GPIO_SFN(4));
	s3c_gpio_cfgpin(S5PV310_GPC0(3), S3C_GPIO_SFN(4));
	s3c_gpio_cfgpin(S5PV310_GPC0(4), S3C_GPIO_SFN(4));

	return 0;
}

static struct resource s5pv310_ac97_resource[] = {
	[0] = {
		.start = S5PV310_PA_AC97,
		.end   = S5PV310_PA_AC97 + 0x100 - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = DMACH_AC97_PCMOUT,
		.end   = DMACH_AC97_PCMOUT,
		.flags = IORESOURCE_DMA,
	},
	[2] = {
		.start = DMACH_AC97_PCMIN,
		.end   = DMACH_AC97_PCMIN,
		.flags = IORESOURCE_DMA,
	},
	[3] = {
		.start = DMACH_AC97_MICIN,
		.end   = DMACH_AC97_MICIN,
		.flags = IORESOURCE_DMA,
	},
	[4] = {
		.start = IRQ_AC97,
		.end   = IRQ_AC97,
		.flags = IORESOURCE_IRQ,
	},
};

static struct s3c_audio_pdata s3c_ac97_pdata = {
	.cfg_gpio = s5pv310_ac97_cfg_gpio,
};

static u64 s5pv310_ac97_dmamask = DMA_BIT_MASK(32);

struct platform_device s5pv310_device_ac97 = {
	.name             = "s3c-ac97",
	.id               = -1,
	.num_resources    = ARRAY_SIZE(s5pv310_ac97_resource),
	.resource         = s5pv310_ac97_resource,
	.dev = {
		.platform_data = &s3c_ac97_pdata,
		.dma_mask = &s5pv310_ac97_dmamask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	},
};

static struct resource s5pv310_rp_resource[] = {
};

struct platform_device s5pv310_device_rp = {
	.name             = "s5p-rp",
	.id               = -1,
	.num_resources    = ARRAY_SIZE(s5pv310_rp_resource),
	.resource         = s5pv310_rp_resource,
};
EXPORT_SYMBOL(s5pv310_device_rp);
